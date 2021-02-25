#pragma once

#include "Pieces.hpp"

namespace MoveType {
enum MoveType : uint8_t
{
    move,
    take,
    en_passant,
    castle,
};
constexpr auto names = std::to_array({ "no", "take", "en_passant", "castle" });
};

struct Move
{
    Position where;
    MoveType::MoveType move_type;
};

using MoveContainer = std::vector<Move>;

constexpr bool
out_of_bounds(Position p)
{
    return p.x >= 8 or p.y >= 8 or p.x < 0 or p.y < 0;
}

[[nodiscard]] MoveContainer
get_moves(Piece const pc, GameData const& context);
