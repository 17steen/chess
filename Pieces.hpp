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
enum Colour : bool
{
    white,
    black,
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
    Colour::Colour turn = Colour::white;

    struct PeekResult
    {
        int8_t idx;
        Piece* piece;
        // trying to use the same naming as std::optional
        [[nodiscard]] constexpr bool has_value() const { return idx != -1; }
        [[nodiscard]] explicit constexpr operator bool() { return has_value(); }
        [[nodiscard]] constexpr Piece const& value() const { return *piece; }
        [[nodiscard]] constexpr Piece& value() { return *piece; }
        constexpr void reset() { idx = -1; }
    };

    // should only be used to move to an emtpy position
    void move(PeekResult pk, Position to)
    {
        board[to.x][to.y] =
          pk.idx; // new position is overwritten, it was hopefully empty
        auto const [x, y] = pk.value().pos;
        board[x][y] = -1;    // previous position is emptied
        pk.value().pos = to; // make piece aware of the move
    }

    // use alternative, will be used as a const way to get access to
    // pieces
    [[nodiscard]] std::optional<Piece> peek(int8_t const x,
                                            int8_t const y) const noexcept
    {
        auto const index = board[x][y];
        if (index == -1)
            return {};
        else
            return { pieces[index] };
    }

    [[nodiscard]] constexpr PeekResult get(int8_t const x,
                                           int8_t const y) noexcept
    {
        auto const index = board[x][y];
        if (index == -1)
            // access should be prevented by using "has_value()"
            return { index, nullptr };
        else
            return { index, &pieces[index] };
    }

    [[nodiscard]] constexpr PeekResult get(Position p) noexcept
    {
        return get(p.x, p.y);
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

    // switches players and returns the current player
    constexpr bool switch_turn() { return turn = Colour::Colour{ !turn }; }
    constexpr bool opponent() const { return !turn; }
    constexpr bool player() const { return turn; }
};

