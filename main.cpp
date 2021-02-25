#include "SDL.h"
#include "SDL_image.h"
#include "SDL_scancode.h"
#include "SDL_video.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>

#include "CustomDeleters.hpp"
#include "Helpers.h"
#include "Logic.h"
#include "Pieces.hpp"

void
check_sdl_failure(bool b, char const* what)
{
    if (b) {
        SDL_Log("Error : %s, %s\n", what, SDL_GetError());
        std::exit(1);
    }
}

void
check_img_failure(bool b, char const* what)
{
    if (b) {
        SDL_Log("Error : %s, %s\n", what, IMG_GetError());
        std::exit(1);
    }
}

void
initialize_sdl()
{
    auto const ec = SDL_Init(SDL_INIT_VIDEO);
    check_sdl_failure(ec != 0, "Video initialization");
}

SurfacePtr
generate_board()
{
    int rmask, gmask, bmask, amask;
    int rshift, gshift, bshift, ashift;

    if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
        rshift = 24;
        gshift = 16;
        bshift = 8;
        ashift = 0;
    } else {
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
        rshift = 0;
        gshift = 8;
        bshift = 16;
        ashift = 24;
    }

    // TODO: probably allow reading that from a config file / settings menu
    constexpr SDL_Color const white_tile_colour{ 230, 204, 171, 0xFF };
    constexpr SDL_Color const black_tile_colour{ 157, 87, 27, 0xFF };

    constexpr auto const colour_switch =
      std::to_array({ white_tile_colour, black_tile_colour });

    // 8 * 8 pixel
    auto board = SDL_CreateRGBSurface(0, 8, 8, 32, rmask, gmask, bmask, amask);

    check_sdl_failure(not board, "Board creation");

    Uint8* const pixel_data = static_cast<Uint8*>(board->pixels);

    auto const pitch = board->pitch;

    for (int y = 0; y < board->h; ++y) {
        // switching from white to black every tile
        bool current_colour = y & 1; // whether we start white, or black;
        for (int x = 0; x < board->w; ++x) {
            Uint32* const target_pixel = reinterpret_cast<Uint32*>(
              pixel_data + y * pitch + x * sizeof(Uint32));

            auto const [r, g, b, a] = colour_switch[current_colour];

            *target_pixel =
              r << rshift | b << bshift | g << gshift | a << ashift;

            current_colour = not current_colour;
        }
    }
    return to_ptr(board);
}

struct Assets
{
    std::array<std::array<SDL_Texture*, PieceType::Count>, 2> pieces;

    SDL_Texture* cursor{};
    SDL_Texture* board{};

    ~Assets()
    {
        for (auto const& arr : pieces) {
            for (auto* const ptr : arr) {
                SDL_Log("Destroying Texture");
                SDL_DestroyTexture(ptr);
            }
        }

        SDL_DestroyTexture(board);
        SDL_DestroyTexture(cursor);
    }
};

Assets
load_assets(RendererPtr const& renderer)
{
    namespace fs = std::filesystem;

    fs::path const assets_dir{ "./assets" };

    auto const flags = IMG_INIT_PNG;

    check_img_failure(not(IMG_Init(flags) & flags), "Error init IMG");

    Assets assets;

    static auto const piece_colour =
      std::to_array({ std::string("piece_white"), std::string("piece_black") });

    // loading pieces
    for (int i = 0; auto const& name : piece_colour) {
        for (int j = 0; auto*& piece : assets.pieces[i]) {
            // create path eg.: ./assets/piece_black5.png
            auto const dir =
              assets_dir / (name + static_cast<char>('0' + j) + ".png");

            auto const surface = to_ptr(IMG_Load(dir.c_str()));
            check_img_failure(not surface.get(), "Image loading");

            auto* const texture =
              SDL_CreateTextureFromSurface(renderer.get(), surface.get());

            check_sdl_failure(not texture, "Texture from image surface");

            // freeing this will be handled by the assets destructor
            piece = texture;

            ++j;
        }
        ++i;
    }

    // loading cursor
    {
        auto const surface =
          to_ptr(IMG_Load((assets_dir / "cursor.png").c_str()));
        check_img_failure(not surface.get(), "Cursor loading");

        auto* const texture =
          SDL_CreateTextureFromSurface(renderer.get(), surface.get());
        check_sdl_failure(not texture, "Texture from cursor surface");

        assets.cursor = texture;
    }

    // generating board
    {
        auto* const texture =
          SDL_CreateTextureFromSurface(renderer.get(), generate_board().get());

        check_sdl_failure(not texture, "Texture from board surface");

        assets.board = texture;
    }

    IMG_Quit();

    return assets;
}

BoardInfo
generate_default_game_data()
{
    BoardInfo data{};
    auto& [pieces, board, turn] = data;

    using namespace PieceType;
    using namespace Colour;
    using namespace LetterColumn;

    for (auto& arr : board)
        for (auto& val : arr)
            val = -1;

    for (auto const [colour, column, add] :
         { std::tuple{ black, 8, 0 }, { white, 1, 8 } }) {

        for (int8_t i = 0;
             auto const type :
             { rook, knight, bishop, queen, king, bishop, knight, rook }) {

            int8_t const index = add + i;

            pieces[index] = {
                .type = type,
                .pos{ i, Y(column) },
                .colour = colour,
                .special = false,
            };

            board[i][Y(column)] = index;
            ++i;
        }
    }

#if 1

    // TODO: cleanup, tried to generalize but doesn't look good
    for (auto [colour, column, add] :
         { std::tuple{ bool{ black }, 7, 0 }, { white, 2, 8 } }) {

        for (int8_t i = A; i <= H; ++i) {

            int8_t index = 16 + add + i;

            pieces[index] = {
                .type = pawn,
                .pos{ i, Y(column) },
                .colour = colour,
                .special = false,
            };

            board[i][Y(column)] = index;
        }
    }
#endif

    return data;
}

struct WindowData
{
    SDL_Window* win;
    inline std::pair<int, int> size()
    {
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        return { w, h };
    }
    inline auto get() { return win; }
    inline auto renderer() { return SDL_GetRenderer(win); }

    ~WindowData()
    {
        SDL_Log("Destroying window");
        SDL_DestroyWindow(win);
    }
};

// TODO: pass in a struct has all necessary information on the window
void
render_board(Assets const& assets,
             BoardInfo const& board_info,
             WindowData& window_data)
{
    auto const [w, h] = window_data.size();
    // SDL_Log("width : %d, height : %d\n", w, h);
    SDL_Rect tile{ .x = 0, .y = 0, .w = w / 8, .h = h / 8 };

    // TODO: handle screen resize and scale this in a rectangle
    SDL_Rect screen_rect{ .x = 0, .y = 0, .w = w, .h = h };

    auto* renderer = window_data.renderer();

    SDL_RenderCopy(renderer, assets.board, nullptr, &screen_rect);

#if 0
    for (auto const& arr : board_info.board) {
        for (auto const ptr : arr) {
            if (ptr != -1) {
                auto& [type, pos, colour, special, alive] =
                  board_info.pieces[ptr];

                tile.x = pos.x * tile.w;
                tile.y = pos.y * tile.h;

                SDL_RenderCopy(
                  renderer, assets.pieces[colour][type], nullptr, &tile);
            }
        }
    }

#else
    for (auto const pc : board_info.pieces) {
        if (pc.alive) {
            tile.x = pc.pos.x * tile.w;
            tile.y = pc.pos.y * tile.h;

            SDL_RenderCopy(
              renderer, assets.pieces[pc.colour][pc.type], nullptr, &tile);
        }
    }

#endif
}

void
game(Assets const& assets, GameData& game_data, WindowData& window_data)
{
    auto* main_renderer = window_data.renderer();

    auto& board = game_data.current_board;

    int mouse_x, mouse_y;
    uint32_t prev_mouse_state{}, mouse_state{};

    MoveContainer moves{};

    BoardInfo::PeekResult selection{};

    bool run{ true };

    while (run) {
        // SDL_WaitEvent(nullptr);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {
                    run = false;
                } break;

                    // if i press H i should see all the previous
                    // states
                case SDL_KEYUP: {
                    switch (e.key.keysym.scancode) {
                        // debug
                        case SDL_SCANCODE_H: {
                            SDL_Log("Pressed H\n");
                            for (auto const& prev : game_data.history) {
                                render_board(assets, prev, window_data);
                                SDL_RenderPresent(main_renderer);
                                SDL_Delay(500);
                                SDL_Log("frame\n");
                            }
                        } break;
                        case SDL_SCANCODE_ESCAPE: {
                            run = false;
                        } break;

                        default: {
                        } break;
                    }

                } break;
            }
        }
        mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);

        auto const [win_w, win_h] = window_data.size();
        auto const tile_width = win_w / 8;
        auto const tile_height = win_h / 8;

        int8_t const m_x = (mouse_x / tile_width) % 8;
        int8_t const m_y = (mouse_y / tile_height) % 8;

        SDL_Rect tile{ .x{}, .y{}, .w = tile_width, .h = tile_height };

        // released
        if (not(mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) and
            prev_mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {

            // if clicking out of a move allows to recheck
            bool keep_checking;
            do {
                keep_checking = false;

                // if there is a selection and we are the right player
                if (selection.has_value()) {

                    auto const res = std::find_if(
                      std::begin(moves), std::end(moves), [m_x, m_y](Move mv) {
                          return mv.where.operator==({ m_x, m_y });
                      });

                    // didn't click on a move
                    if (res == std::end(moves)) {
                        // in case you click away from a move, and hit another
                        // piece, no need to double click.
                        keep_checking = true;
                        selection.reset();

                        // player clicked on a move
                    } else { // TODO: handle clicked on a valid move
                        SDL_Log("valid move ! %s\n",
                                res->move_type ? "takes" : "doesn't take");
                        auto const& [where, takes] = *res;

                        game_data.save();

                        // MUTATES GAME DATA
                        //  take piece
                        switch (takes) {
                            using namespace MoveType;
                            case move: {
                                board.move(selection, where);
                            } break;
                            case take: {
                                // get piece at that position, then mark it as
                                // dead
                                auto& target_piece = board.get(where).value();
                                target_piece.alive = false;

                                if (target_piece.type == PieceType::king) {
                                    SDL_Log(
                                      "%s won.\n",
                                      Colour::names[selection.value().colour]);
                                }

                                board.move(selection, where);

                            } break;
                            case en_passant: {
                                int8_t y =
                                  where.y + (selection.value().colour ? -1 : 1);

                                auto& target_piece =
                                  board.get(where.x, y).value();
                                target_piece.alive = false;

                                board.move(selection, where);

                            } break;
                            case castle: {
                            } break;
                        }

                        board.switch_turn();
                        selection.reset();
                    }

                    // nothing valid was selected
                } else {
                    auto piece_selected = board.get(m_x, m_y);

                    SDL_Log("clicked on %d : %d\n", m_x, m_y);

                    if (piece_selected.has_value() and
                        piece_selected.value().colour == board.player()) {
                        SDL_Log("Selected : %s %s\n",
                                Colour::names[piece_selected.value().colour],
                                PieceType::names[piece_selected.value().type]);

                        selection = piece_selected;
                        moves = get_moves(piece_selected.value(), game_data);
                    } else {
                        selection.reset();
                    }
                }
            } while (keep_checking);
        }

        SDL_RenderClear(main_renderer);
        render_board(assets, board, window_data);
        if (selection) {
            for (auto const move : moves) {
                tile.x = move.where.x * tile_width;
                tile.y = move.where.y * tile_height;
                if (not move.move_type) {
                    SDL_SetRenderDrawColor(main_renderer, 0, 100, 200, 200);
                } else {
                    SDL_SetRenderDrawColor(main_renderer, 0, 200, 100, 200);
                }
                SDL_RenderFillRect(main_renderer, &tile);
            }
        }

        SDL_RenderPresent(main_renderer);

        prev_mouse_state = mouse_state;
    }
}

int
main(int const argc, char const* const* const argv)
{
    int const width = 800;
    int const height = 800;

    initialize_sdl();

    auto main_window =
      WindowData{ .win = SDL_CreateWindow("Chess game",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          width,
                                          height,
                                          SDL_WINDOW_RESIZABLE) };

    check_sdl_failure(not main_window.get(), "Window creation");

    SDL_SetWindowMinimumSize(main_window.get(), width / 10, height / 10);

    auto const render_flags = SDL_RENDERER_ACCELERATED;

    // TODO: figure out if i need to store the renderer since it is associated
    // to the window
    auto const main_renderer =
      to_ptr(SDL_CreateRenderer(main_window.get(), -1, render_flags));

    assert(main_renderer.get() == SDL_GetRenderer(main_window.get()));

    // data structure containing all textures representing games pieces
    auto const assets = load_assets(main_renderer);

    check_sdl_failure(not main_renderer.get(), "Renderer creation");

    SDL_SetRenderDrawBlendMode(main_renderer.get(), SDL_BLENDMODE_BLEND);

    GameData game_data = { .current_board{ generate_default_game_data() },
                           .history{} };

    game(assets, game_data, main_window);
}
