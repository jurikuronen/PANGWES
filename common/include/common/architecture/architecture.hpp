/*
 * architecture.hpp - Simple compile-time architecture requirement check.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <limits>

static_assert(std::numeric_limits<std::size_t>::digits >= 64, "PANGWES requires a 64-bit architecture.");
