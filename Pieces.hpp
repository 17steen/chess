#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace PieceType {
enum PieceType
{
    rook,
    knight,
    bishop,
    queen,
    king,
    pawn,
    Count,
};
};

namespace LetterColumn {
// y coordinate is upside down and starts at 1
constexpr auto Y{ [](int8_t i) -> int8_t { return 8 - i; } };
enum LetterColumn
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    Count,
};
};

namespace Colour {
enum Colour
{
    white,
    black,
    Count,
};
};

struct Position
{
    int8_t row, col;
};

using MoveVector = std::vector<Position>;

struct Piece
{
    PieceType::PieceType type;
    Position pos;
    bool colour;
    bool special; // to be marked as true when the pawn moves two tiles and to
                  // false at its next move
};

struct GameData
{
    // this array should only be generated once, all other accesses to the data
    // should be done through the pointers(array index).
    std::array<Piece, 32> pieces;
    std::array<std::array<int8_t, 8>, 8> board;

    [[nodiscard]] std::optional<Piece> peek(int8_t x, int8_t y) const
    {
        auto index = board[x][y];
        if (index == -1)
            return {};
        else
            return { pieces[index] };
    }
};

