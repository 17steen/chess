
#include "SDL.h"
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
#include "Pieces.hpp"

using namespace MoveType;

MoveContainer
get_moves_rook(Piece const pc, GameData const& game_data)
{
    auto const& board = game_data.current_board;

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
                    moves.push_back({ .where = pos, .move_type = take });
                    break;
                }
                // nothing
            } else {
                moves.push_back({ .where = pos, .move_type = move });
            }
        }
    }

    return moves;
}

MoveContainer
get_moves_knight(Piece const pc, GameData const& game_data)
{
    MoveContainer moves{};

    auto const& board = game_data.current_board;

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
                std::printf("encountered %s %s at %d,%d\n",
                            Colour::names[value.colour],
                            PieceType::names[value.type],
                            value.pos.x,
                            value.pos.y);
                moves.push_back({ .where = pos, .move_type = take });
            }
            // empty tile
        } else {
            moves.push_back({ .where = pos, .move_type = move });
        }
    }

    return moves;
}

MoveContainer
get_moves_bishop(Piece const pc, GameData const& game_data)
{
    auto const& board = game_data.current_board;
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
                    moves.push_back({ .where = pos, .move_type = take });
                    break;
                }
                // nothing
            } else {
                moves.push_back({ .where = pos, .move_type = move });
            }
        }
    }

    return moves;
}

MoveContainer
get_moves_queen(Piece const pc, GameData const& game_data)
{
    auto const& board = game_data.current_board;
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
                    moves.push_back({ .where = pos, .move_type = take });
                    break;
                }
                // nothing
            } else {
                moves.push_back({ .where = pos, .move_type = move });
            }
        }
    }

    return moves;
}

MoveContainer
get_moves_king(Piece const pc, GameData const& game_data)
{
    auto const& board = game_data.current_board;
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
                moves.push_back({ .where = pos, .move_type = take });
            }
        } else { // empty tile
            std::printf("move on %d : %d\n", pos.x, pos.y);
            moves.push_back({ .where = pos, .move_type = move });
        }
    }

    return moves;
}

MoveContainer
get_moves_pawn(Piece const pc, GameData const& game_data)
{
    auto const& board = game_data.current_board;
    MoveContainer moves{};

    bool is_obstructed = false;

    int8_t dir;
    int8_t requirement_2_steps;    // pos requirement to move 2 upwards
    int8_t requirement_en_passant; // pos requirement to move 2 upwards

    if (pc.colour == Colour::white) {
        dir = -1; // go up
        requirement_2_steps = LetterColumn::Y(2);
        requirement_en_passant = LetterColumn::Y(5);
    } else { // Colour::black
        dir = 1;
        requirement_2_steps = LetterColumn::Y(7);
        requirement_en_passant = LetterColumn::Y(4);
    }

    // check one in front
    {
        auto where{ pc.pos };
        where.y += dir;

        if (out_of_bounds(where)) {
            // TODO promote pawn
        } else {
            auto piece = board.peek(where.x, where.y);
            if (not piece.has_value()) {
                moves.push_back({
                  .where = where,
                  .move_type = move,
                });
            } else {
                is_obstructed = true;
            }
        }
    }

    // check if you can move 2 steps up
    if (not is_obstructed and pc.pos.y == requirement_2_steps) {
        auto where{ pc.pos };
        where.y += 2 * dir;

        auto piece = board.peek(where.x, where.y);
        if (not piece.has_value()) {
            moves.push_back({
              .where = where,
              .move_type = move,
            });
        }
    }

    // for diagonal takes
    for (auto const ways = std::to_array({ Position{ 1, dir }, { -1, dir } });
         auto const [x, y] : ways) {
        auto where{ pc.pos };
        where.y += y;
        where.x += x;

        if (out_of_bounds(where))
            continue;

        auto const piece = board.peek(where.x, where.y);
        if (piece.has_value() and piece.value().colour != pc.colour)
            moves.push_back({
              .where = where,
              .move_type = take,
            });
    }

    // check en_passant
    if (pc.pos.y == requirement_en_passant) {
        SDL_Log("Piece fits requirements for en passant\n");
        for (auto const next_to : { 1, -1 }) {

            auto where{ pc.pos };
            where.x += next_to;

            if (out_of_bounds(where)) {
                SDL_Log("en passant not out of bounds\n");
                continue;
            }

            auto const piece = board.peek(where);

            if (piece.has_value() and piece.value().type == PieceType::pawn and
                piece.value().colour != pc.colour) {
                SDL_Log("There is a pawn next to you, checking if said pawn "
                        "moved 2 steps last turn");
                // check if moved twice
                // this could segfault if no moved happened before getting here
                auto const& past_state = game_data.history.back();

                // go two steps up
                auto const past_piece =
                  past_state.peek(where.x, where.y + dir * 2);

                if (past_piece.has_value() and
                    past_piece.value().type == PieceType::pawn) {

                    // holds destination y
                    int8_t y = where.y + dir;

                    SDL_Log("En passant available\n");
                    moves.push_back({
                      .where = { where.x, y },
                      .move_type = en_passant,
                    });
                }
            }
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

