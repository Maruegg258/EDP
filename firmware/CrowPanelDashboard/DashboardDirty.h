#pragma once

#include <stdint.h>

using DashboardDirtyMask = uint16_t;

namespace DashboardDirty {

constexpr DashboardDirtyMask NONE = 0;
constexpr DashboardDirtyMask CLOCK = 1u << 0;
constexpr DashboardDirtyMask WEATHER = 1u << 1;
constexpr DashboardDirtyMask WIFI = 1u << 2;
constexpr DashboardDirtyMask BTC = 1u << 3;
constexpr DashboardDirtyMask ETH = 1u << 4;
constexpr DashboardDirtyMask HYPE = 1u << 5;
constexpr DashboardDirtyMask STATUS = 1u << 6;

constexpr DashboardDirtyMask ALL =
    CLOCK |
    WEATHER |
    WIFI |
    BTC |
    ETH |
    HYPE |
    STATUS;

}  // namespace DashboardDirty
