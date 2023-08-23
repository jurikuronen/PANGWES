#pragma once

#include <ostream>
#include <type_traits>

enum class OperatingMode {
    DEFAULT                         = 0x00,
    CDBG                            = 0x01,
    SGGS                            = 0x02,
    CDBG_AND_SGGS                   = 0x03,
    SGG_FILTER                      = 0x04, // Not implemented.

    CDBG_AND_SGGS_FILTERED          = 0x07, // Not implemented.
    GENERAL                         = 0x08,

    OUTLIER_TOOLS                   = 0x20

};

using underlying_type = std::underlying_type<OperatingMode>::type;

inline OperatingMode operator|(const OperatingMode lhs, const OperatingMode rhs) {
    return static_cast<OperatingMode>(static_cast<underlying_type>(lhs) | static_cast<underlying_type>(rhs));
}

inline OperatingMode operator|=(OperatingMode& lhs, const OperatingMode rhs) {
    return lhs = lhs | rhs;
}

inline OperatingMode operator&(const OperatingMode lhs, const OperatingMode rhs) {
    return static_cast<OperatingMode>(static_cast<underlying_type>(lhs) & static_cast<underlying_type>(rhs));
}

inline OperatingMode remove_mode(OperatingMode& lhs, OperatingMode rhs) {
    return lhs = static_cast<OperatingMode>(static_cast<underlying_type>(lhs) & ~static_cast<underlying_type>(rhs));
}

inline bool operating_mode_to_bool(const OperatingMode om) {
    return static_cast<bool>(underlying_type(om));
}

inline std::ostream& operator<<(std::ostream& os, OperatingMode om) {
    if (om == OperatingMode::OUTLIER_TOOLS) return os << "OUTLIER_TOOLS";
    switch (remove_mode(om, OperatingMode::OUTLIER_TOOLS)) {
        case OperatingMode::CDBG:                            os << "CDBG"; break;
        case OperatingMode::CDBG_AND_SGGS:                   os << "CDBG_AND_SGGS"; break;
        case OperatingMode::GENERAL:                         os << "GENERAL"; break;
        default:                                             os << "DEFAULT";
    }
    return os;
}



