/*
 * MockQueriesData.hpp - Mocked queries data type with integration to MockFileReader.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cassert>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "mocks/MockData.hpp"

namespace PANGWES {
namespace Mocks {

// Stores unitig IDs for mocked queries testing.
struct MockQueryData {
    std::size_t unitig1_id;
    std::size_t unitig2_id;
    std::string other_fields;

    // Constructs with unitig IDs; other fields set to any value.
    MockQueryData(std::size_t id1, std::size_t id2)
        : MockQueryData(id1, id2, "a a a")
    { }

    // Constructs with unitig IDs and explicit other fields.
    MockQueryData(std::size_t id1, std::size_t id2, std::string fields)
        : unitig1_id{id1},
          unitig2_id{id2},
          other_fields{std::move(fields)}
    { }
};

class MockQueriesData {
    public:
        explicit MockQueriesData(std::vector<MockQueryData> mock_data)
            : m_mock_queries_data(std::move(mock_data))
        { }

        std::size_t size() const noexcept {
            return m_mock_queries_data.size();
        }

        std::size_t unitig1_id(std::size_t idx) const noexcept {
            assert(idx < m_mock_queries_data.size());

            return m_mock_queries_data[idx].unitig1_id;
        }

        std::size_t unitig2_id(std::size_t idx) const noexcept {
            assert(idx < m_mock_queries_data.size());

            return m_mock_queries_data[idx].unitig2_id;
        }

        std::string aracne_flag_and_mi_score_fields(std::size_t idx) const noexcept {
            assert(idx < m_mock_queries_data.size());

            // Extract and return the fourth and fifth fields.
            auto other_fields = m_mock_queries_data[idx].other_fields;
            return other_fields.substr(other_fields.find(' ') + 1);
        }

        // Return a mock query line as it would appear in the input file.
        std::string to_string(std::size_t idx) const {
            assert(idx < m_mock_queries_data.size());

            std::ostringstream oss;

            oss << unitig1_id(idx) << " " << unitig2_id(idx) << " " << m_mock_queries_data[idx].other_fields;

            return oss.str();
        }

        // Returns mock queries file contents (one line per query) for MockFileReader.
        std::vector<std::string> to_contents() const {
            std::vector<std::string> contents;

            for (std::size_t idx = 0; idx < size(); ++idx) {
                contents.push_back(to_string(idx));
            }

            return contents;
        }

    private:
        std::vector<MockQueryData> m_mock_queries_data;
};

} // namespace Mocks
} // namespace PANGWES
