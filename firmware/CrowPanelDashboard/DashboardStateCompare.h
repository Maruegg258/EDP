#pragma once

#include "DashboardDirty.h"
#include "DashboardState.h"

DashboardDirtyMask detectDashboardDirty(
    const DashboardState& previous,
    const DashboardState& current
);
