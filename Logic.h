#pragma once

#include "Pieces.hpp"

struct Move
{
    Position where;
    bool takes;
};

using MoveContainer = std::vector<Move>;

constexpr bool
out_of_bounds(Position p)
{
    return p.x >= 8 or p.y >= 8 or p.x < 0 or p.y < 0;
}

[[nodiscard]] MoveContainer
get_moves(Piece const pc, GameData const& context);
