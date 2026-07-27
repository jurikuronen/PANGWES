/*
 * GFAFormat.cpp - Enumeration for GFA formats.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>

#include "common/type_traits/type_traits.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/GFAFormat.hpp"

namespace PANGWES {

GFAFormat to_gfa_format(std::uint64_t gfa_format_uint) {
    switch (gfa_format_uint) {
        case 1: return GFAFormat::GFA1;
        case 2: return GFAFormat::GFA2;
        default: throw Exception(ErrorCode::INVALID_GFA_FORMAT, "value: ", gfa_format_uint);
    }
}

std::string to_string(GFAFormat gfa_format) {
    assert(Traits::to_underlying(gfa_format) <= Traits::to_underlying(GFAFormat::GFA2) && "Unknown GFA format");

    if (gfa_format == GFAFormat::GFA1) {
        return "GFA 1.0";
    }

    if (gfa_format == GFAFormat::GFA2) {
        return "GFA 2.0";
    }

    throw Exception(ErrorCode::INVALID_GFA_FORMAT, "underlying value: ",
                    Traits::to_underlying(gfa_format));
}

} // namespace PANGWES
