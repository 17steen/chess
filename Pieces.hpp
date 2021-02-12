#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <string_view>
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
constexpr auto names =
  std::to_array({ "rook", "knight", "bishop", "queen", "king", "pawn" });
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
constexpr auto names = std::to_array({ "white", "black" });
};

struct Position
{
    int8_t x, y;
    constexpr bool operator==(Position other)
    {
        return other.x == x and other.y == y;
    };
};

using MoveVector = std::vector<Position>;

struct Piece
{
    PieceType::PieceType type;
    Position pos;
    bool colour;
    bool special; // to be marked as true when the pawn moves two tiles and to
                  // false at its next move
    bool alive = true;
};

struct GameData
{
    // this array should only be generated once, all other accesses to the data
    // should be done through the pointers(array index).
    std::array<Piece, 32> pieces;
    std::array<std::array<int8_t, 8>, 8> board;

    // TODO: this sucks, neither a reference, neither do i have the index
    [[nodiscard]] std::optional<Piece> peek(int8_t const x,
                                            int8_t const y) const noexcept
    {
        auto const index = board[x][y];
        if (index == -1)
            return {};
        else
            return { pieces[index] };
    }

    inline void log() const
    {
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                auto const pc = peek(x, y);
                if (pc.has_value()) {
                    std::printf("%7s", PieceType::names[pc.value().type]);
                } else {
                    std::printf("%7s", "none");
                }
            }
            std::puts("");
        }
        std::puts("");
    }
};

