#pragma once

#include "Pieces.hpp"

struct Move
{
    Position where;
    bool takes;
};

using MoveContainer = std::vector<Move>;
