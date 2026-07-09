/*
 * SGGNodeMap.hpp - Class for storing single-genome graph (SGG) mapping information.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {

struct NodeMapElement {
    std::size_t node_mapping;
    std::size_t path_mapping;
};

/*
 * Class storing mapping information from nodes in a SGG to the corresponding nodes in the global compacted de Bruijn
 * graph (cdBG).
 *
 * The global cdBG is built across all assemblies, so its nodes (=unitigs) must be consistent across that full set. As a
 * result, cdBG unitigs are not necessarily maximal within individual SGGs, and can often be compacted further.
 *
 * All cdBG nodes that remain maximal unitigs in the SGG are represented as explicit "real nodes". Otherwise, a chain of
 * cdBG nodes is compacted into an SGGPath, and the intermediate nodes are represented as "path nodes" that are mapped
 * to positions along that path.
*/
class SGGNodeMap {
public:
    SGGNodeMap() = default;

    SGGNodeMap(const SGGNodeMap&) = delete;
    SGGNodeMap& operator=(const SGGNodeMap&) = delete;
    SGGNodeMap(SGGNodeMap&&) = default;
    SGGNodeMap& operator=(SGGNodeMap&&) = default;

    // Returns the size of the internal mapping vector.
    std::size_t size() const noexcept;

    // Returns the capacity of the internal mapping vector.
    std::size_t capacity() const noexcept;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    // Resizes the node map to the given size, initializing any new elements to unmapped values.
    void resize(std::size_t size);

    /*
     * Maps a cdBG node into the SGG using this map:
     * - If `path_mapping == SGG_NODE_NOT_MAPPED`, `node` is represented as an explicit real node in the SGG and
     *   `node_mapping` is the corresponding index of the node in the SGG's adjacency list.
     * - Otherwise, `node` is represented as a "path node" as part of the path with the index `path_mapping`. In this
     *   case, `node_mapping` is the position (index) of `node` along that path.
     *
     * Note: the node map must already have been resized to include `node`.
    */
    void map_node(std::size_t node, std::size_t node_mapping, std::size_t path_mapping = SGG_NODE_NOT_MAPPED);

    /*
     * Returns the node mapping of a cdBG node `node` in the SGG:
     * - Real node: returns the index of the explicit node in the SGG's adjacency list.
     * - Path node: returns the node's position (index) along the corresponding SGGPath.
    */
    std::size_t node_mapping(std::size_t node) const noexcept;

    /*
     * Returns the path mapping of a cdBG node `node` in the SGG:
     * - Real node: returns `SGG_NODE_NOT_MAPPED` (indicating "not on a path").
     * - Path node: returns the index of the corresponding SGGPath in the SGG.
    */
    std::size_t path_mapping(std::size_t node) const noexcept;

    // Returns true if the cdBG node `node` has been been mapped in the SGG, either as a real node or a path node.
    bool is_mapped(std::size_t node) const noexcept;

    // Returns true if the cdBG node `node` is a path node in the SGG.
    bool is_on_path(std::size_t node) const noexcept;

private:
    std::vector<NodeMapElement> m_node_map;
};

} // namespace PANGWES
