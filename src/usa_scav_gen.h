#pragma once
#include <utility>
#include <vector>

namespace ScavGen
{
    // Scans map elevation, passability, valleys, and chokepoints to build
    // campaign-grade Scavenger bases and fortified oil derricks.
    void generateAll(int beachheadLzX, int beachheadLzY, const std::vector<std::pair<int, int>>& seedValleys);
}
