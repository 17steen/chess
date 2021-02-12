#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "Logic.h"

MoveContainer
get_moves_rook(Piece const pc, GameData const& board)
{
    MoveContainer moves{};

    auto ways = std::to_array({
      std::pair{ 0, 1 },
      { 0, -1 },
      { 1, 0 },
      { -1, 0 },
    });

    for (auto const [x, y] : ways) {
        auto pos{ pc.pos };

        for (;;) {
            pos.y += y;
            pos.x += x;

            if (out_of_bounds(pos))
                break;

            auto piece = board.peek(pos.x, pos.y);

            bool takes = false;

            if (piece.has_value()) {
                auto& value = piece.value();
                // encountered teammate
                if (value.colour == pc.colour) {
                    break;
                    // encountered enemy
                } else {
                    moves.push_back({ .where = pos, .takes = true });
                    break;
                }
                // nothing
            } else {
                moves.push_back({ .where = pos, .takes = false });
            }
        }
    }

    return moves;
}

MoveContainer
get_moves_knight(Piece const pc, GameData const& board)
{
    MoveContainer moves{};

    auto ways = std::to_array({
      std::pair{ 1, 2 },
      { 2, 1 },
      { 2, -1 },
      { 1, -2 },
      { -1, -2 },
      { -2, -1 },
      { -2, 1 },
      { -1, 2 },
    });

    for (auto const [x, y] : ways) {
        auto pos{ pc.pos };

        pos.x += x;
        pos.y += y;

        if (out_of_bounds(pos))
            continue;

        auto const piece = board.peek(pos.x, pos.y);

        bool takes = false;

        if (piece.has_value()) {
            auto& value = piece.value();
            // encountered enemy
            if (value.colour != pc.colour) {
                board.log();
                std::printf("encountered %s %s at %d,%d\n",
                            Colour::names[value.colour],
                            PieceType::names[value.type],
                            value.pos.x,
                            value.pos.y);
                moves.push_back({ .where = pos, .takes = true });
            }
            // empty tile
        } else {
            moves.push_back({ .where = pos, .takes = false });
        }
    }

    return moves;
}

MoveContainer
get_moves_bishop(Piece const pc, GameData const& board)
{
    MoveContainer moves{};

    auto ways = std::to_array({
      std::pair{ 1, 1 },
      { 1, -1 },
      { -1, -1 },
      { -1, 1 },
    });

    for (auto const [x, y] : ways) {
        auto pos{ pc.pos };

        for (;;) {
            pos.y += y;
            pos.x += x;

            if (out_of_bounds(pos))
                break;

            auto piece = board.peek(pos.x, pos.y);

            bool takes = false;

            if (piece.has_value()) {
                auto& value = piece.value();
                // encountered teammate
                if (value.colour == pc.colour) {
                    break;
                    // encountered enemy
                } else {
                    moves.push_back({ .where = pos, .takes = true });
                    break;
                }
                // nothing
            } else {
                moves.push_back({ .where = pos, .takes = false });
            }
        }
    }

    return moves;
}

MoveContainer
get_moves_queen(Piece const pc, GameData const& board)
{
    MoveContainer moves{};

    // essentially just bishop + rook
    auto ways = std::to_array({
      std::pair{ 1, 1 },
      { 1, -1 },
      { -1, -1 },
      { -1, 1 },
      { 0, 1 },
      { 0, -1 },
      { 1, 0 },
      { -1, 0 },
    });

    for (auto const [x, y] : ways) {
        auto pos{ pc.pos };

        for (;;) {
            pos.y += y;
            pos.x += x;

            if (out_of_bounds(pos))
                break;

            auto piece = board.peek(pos.x, pos.y);

            bool takes = false;

            if (piece.has_value()) {
                auto& value = piece.value();
                // encountered teammate
                if (value.colour == pc.colour) {
                    break;
                    // encountered enemy
                } else {
                    moves.push_back({ .where = pos, .takes = true });
                    break;
                }
                // nothing
            } else {
                moves.push_back({ .where = pos, .takes = false });
            }
        }
    }

    return moves;
}

MoveContainer
get_moves_king(Piece const pc, GameData const& board)
{
    MoveContainer moves{};

    auto ways = std::to_array({
      std::pair{ 1, 1 },
      { 1, -1 },
      { -1, -1 },
      { -1, 1 },
      { 0, 1 },
      { 0, -1 },
      { 1, 0 },
      { -1, 0 },
    });

    for (auto const [x, y] : ways) {
        auto pos{ pc.pos };

        pos.x += x;
        pos.y += y;

        if (out_of_bounds(pos))
            continue;

        auto const piece = board.peek(pos.x, pos.y);
        std::printf("peeking on %d : %d\n", pos.x, pos.y);

        bool takes = false;

        if (piece.has_value()) {
            std::printf("there is someone\n");
            auto const& value = piece.value();
            // encountered enemy
            if (value.colour != pc.colour) {
                moves.push_back({ .where = pos, .takes = true });
            }
        } else { // empty tile
            std::printf("move on %d : %d\n", pos.x, pos.y);
            moves.push_back({ .where = pos, .takes = false });
        }
    }

    return moves;
}

MoveContainer
get_moves_pawn(Piece const pc, GameData const& board)
{
    MoveContainer moves{};

    int8_t dir;
    int8_t requirement; // pos requirement to move 2 upwards

    if (pc.colour == Colour::white) {
        dir = -1; // go up
        requirement = LetterColumn::Y(2);
    } else { // Colour::black
        dir = 1;
        requirement = LetterColumn::Y(7);
    }

    if (pc.pos.y == requirement) {
        auto where{ pc.pos };
        where.y += 2 * dir;

        auto piece = board.peek(where.x, where.y);
        if (piece.has_value()) {
            if (piece.value().colour != pc.colour) {
                moves.push_back({
                  .where = where,
                  .takes = true,
                });
            }
        } else {
            moves.push_back({
              .where = where,
              .takes = false,
            });
        }
    }

    {
        auto where{ pc.pos };
        where.y += dir;

        auto piece = board.peek(where.x, where.y);
        if (piece.has_value()) {
            if (piece.value().colour != pc.colour) {
                moves.push_back({
                  .where = where,
                  .takes = true,
                });
            }
        } else {
            moves.push_back({
              .where = where,
              .takes = false,
            });
        }
    }

    // checking en passant
    auto const ways = std::to_array({ std::pair{ 1, dir }, { -1, dir } });

    for (auto const [x, y] : ways) {
        auto where{ pc.pos };
        where.y += y;
        where.x += x;

        auto piece = board.peek(where.x, where.y);
        // if there is a pawn in diagonal, and that pawn is an enemy and they
        // moved 2 tiles
        if (piece.has_value() and piece.value().colour != pc.colour and
            piece.value().special) {

            moves.push_back({
              .where = where,
              .takes = true,
            });
        }
    }

    return moves;
}

[[nodiscard]] MoveContainer
get_moves(Piece const pc, GameData const& context)
{

    constexpr auto functions = std::to_array({
      &get_moves_rook,
      &get_moves_knight,
      &get_moves_bishop,
      &get_moves_queen,
      &get_moves_king,
      &get_moves_pawn,
    });

    return functions[pc.type](pc, context);
}

