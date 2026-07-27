/*
 * ConcurrentSet.hpp - Class for concurrently storing unique keys.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/type_traits/type_traits.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"

namespace PANGWES {

/*
 * Stores unique keys in independently locked shards.
 *
 * Only insertion is safe concurrently.
*/
template <typename Key, typename Hash = std::hash<Key>>
class ConcurrentSet {
private:
    static_assert(Traits::has_hash_operator<Key, Hash>::value,
                  "ConcurrentSet<Key, Hash> requires a valid hash operator for Key");
    static_assert(Traits::has_equal_operator<Key>::value, "ConcurrentSet<Key> requires operator== for Key");

    // Independently locked shard storing keys and a multimap mapping key hashes to the corresponding key indices.
    struct Shard {
        std::unordered_multimap<std::size_t, std::size_t> hash_to_key_idx;
        std::vector<Key> keys;
        std::mutex mutex;

        // Removes the stored keys and releases this shard's container memory.
        void clear_and_release_reserved_memory() {
            Memory::clear_and_release_reserved_memory(hash_to_key_idx);
            Memory::clear_and_release_reserved_memory(keys);
        }
    };

    std::vector<Shard> m_shards;

public:
    // Constructs a ConcurrentSet with the given number of shards.
    explicit ConcurrentSet(std::size_t n_shards = 64)
        : m_shards(n_shards)
    {
        if (m_shards.empty()) {
            throw Exception(ErrorCode::INVALID_ARGUMENT, "ConcurrentSet requires at least one shard");
        }
    }

    ConcurrentSet(const ConcurrentSet&) = delete;
    ConcurrentSet& operator=(const ConcurrentSet&) = delete;
    ConcurrentSet(ConcurrentSet&& other) = delete;
    ConcurrentSet& operator=(ConcurrentSet&& other) = delete;

    // Requests enough total capacity in the internal containers to store `capacity` keys.
    void reserve(std::size_t capacity) {
        const auto n_shards = m_shards.size();
        const auto shard_capacity = (capacity + n_shards - 1) / n_shards;

        for (auto& shard : m_shards) {
            shard.hash_to_key_idx.reserve(shard_capacity);
            shard.keys.reserve(shard_capacity);
        }
    }

    // Removes all stored keys and releases the per-shard container memory.
    void clear_and_release_reserved_memory() {
        for (auto& shard : m_shards) {
            shard.clear_and_release_reserved_memory();
        }
    }

    // Returns the number of stored keys.
    std::size_t size() const noexcept {
        std::size_t size = 0;

        for (const auto& shard : m_shards) {
            size += shard.keys.size();
        }

        return size;
    }

    // Returns true if the key exists in this set.
    bool contains(const Key& key) const {
        const auto key_hash = Hash()(key);
        const auto& shard = m_shards[shard_index_for_hash(key_hash)];

        return shard_contains(key, shard, key_hash);
    }

    // Tries to insert a copy of the key and returns true if an insertion took place.
    bool insert(const Key& key) {
        return insert_impl(key);
    }

    // Tries to insert the moved key and returns true if an insertion took place.
    bool insert(Key&& key) {
        return insert_impl(std::move(key));
    }

    // Const iterator over stored keys.
    class const_iterator {
    public:
        using KeyIteratorT = typename std::vector<Key>::const_iterator;

        using iterator_category = std::forward_iterator_tag;
        using value_type = Key;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        // Constructs an iterator pointing to the first non-empty shard starting from `shard_index`.
        const_iterator(const ConcurrentSet* set, std::size_t shard_index)
            : m_set{set},
              m_shard_index{shard_index},
              m_key_iter{}
        {
            move_iterator_to_next_non_empty_shard();
        }

        // Returns the key that the current iterator points to.
        reference operator*() const {
            return *m_key_iter;
        }

        // Returns a pointer to the key that the current iterator points to.
        pointer operator->() const {
            return &(*m_key_iter);
        }

        // Advances the iterator, wrapping around shards (skipping empty shards).
        const_iterator& operator++() {
            ++m_key_iter;

            if (m_key_iter == current_shard_keys().end()) {
                ++m_shard_index;
                move_iterator_to_next_non_empty_shard();
            }

            return *this;
        }

        // Inequality comparison for iterator position.
        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }

        // Equality comparison for iterator position.
        bool operator==(const const_iterator& other) const {
            if (m_set != other.m_set || m_shard_index != other.m_shard_index) {
                return false;
            }

            return is_end() || m_key_iter == other.m_key_iter;
        }

    private:
        const ConcurrentSet* m_set;
        std::size_t m_shard_index;
        KeyIteratorT m_key_iter;

        // Returns true for the "end iterator".
        bool is_end() const noexcept {
            return m_shard_index == m_set->m_shards.size();
        }

        // Returns a reference to the stored keys of the current shard.
        const std::vector<Key>& current_shard_keys() const {
            return m_set->m_shards[m_shard_index].keys;
        }

        // Moves the iterator to the first key in the current shard or the next non-empty shard.
        void move_iterator_to_next_non_empty_shard() {
            while (!is_end()) {
                m_key_iter = current_shard_keys().begin();

                if (m_key_iter != current_shard_keys().end()) {
                    return;
                }

                ++m_shard_index;
            }
        }
    };

    // Returns the first iterator of the stored keys.
    const_iterator begin() noexcept {
        return const_iterator(this, 0);
    }

    // Returns the end iterator of the stored keys.
    const_iterator end() noexcept {
        return const_iterator(this, m_shards.size());
    }

    // Returns the first iterator of the stored keys.
    const_iterator begin() const noexcept {
        return const_iterator(this, 0);
    }

    // Returns the end iterator of the stored keys.
    const_iterator end() const noexcept {
        return const_iterator(this, m_shards.size());
    }

private:
    // Returns the index of the shard responsible for the given key hash.
    std::size_t shard_index_for_hash(std::size_t key_hash) const {
        return key_hash % m_shards.size();
    }

    // Returns true if the given shard contains the key.
    static bool shard_contains(const Key& key, const Shard& shard, std::size_t key_hash) {
        const auto existing_key_ids = shard.hash_to_key_idx.equal_range(key_hash);

        for (auto key_idx_iter = existing_key_ids.first; key_idx_iter != existing_key_ids.second; ++key_idx_iter) {
            if (shard.keys[key_idx_iter->second] == key) {
                return true;
            }
        }

        return false;
    }

    // Implementation shared by the copying and moving overloads.
    template <typename KeyArg>
    bool insert_impl(KeyArg&& key) {
        const auto key_hash = Hash()(key);
        auto& shard = m_shards[shard_index_for_hash(key_hash)];

        std::lock_guard<std::mutex> lock(shard.mutex);

        if (shard_contains(key, shard, key_hash)) {
            return false;
        }

        // Insert the key.
        shard.hash_to_key_idx.emplace(key_hash, shard.keys.size());
        shard.keys.push_back(std::forward<KeyArg>(key));

        return true;
    }
};

} // namespace PANGWES
