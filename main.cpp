#include "SDL.h"
#include "SDL_image.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>

#include "CustomDeleters.hpp"
#include "Pieces.hpp"

constexpr int const refresh_rate = 144;
constexpr int const width = 800;
constexpr int const height = 800;

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

    return {
        board,
        &SDL_FreeSurface,
    };
}

struct Assets
{
    std::array<std::array<SDL_Texture*, PieceType::Count>, Colour::Count>
      pieces;

    SDL_Texture* cursor{};

    ~Assets()
    {
        for (auto& arr : pieces) {
            for (auto* ptr : arr) {
                SDL_DestroyTexture(ptr);
            }
        }

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

            auto const texture =
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
        auto surface = to_ptr(IMG_Load((assets_dir / "cursor.png").c_str()));
        check_img_failure(not surface.get(), "Cursor loading");

        auto texture =
          SDL_CreateTextureFromSurface(renderer.get(), surface.get());
        check_sdl_failure(not texture, "Texture from cursor surface");

        assets.cursor = texture;
    }

    IMG_Quit();

    return assets;
}

GameData
generate_default_game_data()
{
    GameData data{};
    auto& [pieces, board] = data;

    using namespace PieceType;
    using namespace Colour;
    using namespace LetterColumn;

    for (auto& arr : board)
        for (auto& val : arr)
            val = -1;

    for (auto [colour, column, add] :
         { std::tuple{ bool{ black }, 8, 0 }, { white, 1, 8 } }) {

        for (uint8_t i = 0;
             auto type :
             { rook, knight, bishop, queen, king, bishop, knight, rook }) {

            int8_t index = add + i;

            pieces[index] = {
                .type = type,
                .pos{ Y(column), i },
                .colour = colour,
                .special = false,
            };

            board[Y(column)][i] = index;
            ++i;
        }
    }

    // TODO: cleanup, tried to generalize but doesn't look good
    for (auto [colour, column, add] :
         { std::tuple{ bool{ black }, 7, 0 }, { white, 2, 8 } }) {

        for (uint8_t i = A; i <= H; ++i) {

            int8_t index = 16 + add + i;

            pieces[index] = {
                .type = pawn,
                .pos{ Y(column), i },
                .colour = colour,
                .special = false,
            };

            board[Y(column)][i] = index;
        }
    }

    return data;
}

int
main(int argc, char* argv[])
{
    int const width = 800;
    int const height = 800;

    initialize_sdl();

    auto main_window = to_ptr(SDL_CreateWindow("Chess game",
                                               SDL_WINDOWPOS_CENTERED,
                                               SDL_WINDOWPOS_CENTERED,
                                               width,
                                               height,
                                               SDL_WINDOW_RESIZABLE));

    check_sdl_failure(not main_window.get(), "Window creation");

    auto const render_flags = SDL_RENDERER_ACCELERATED;

    auto main_renderer =
      to_ptr(SDL_CreateRenderer(main_window.get(), -1, render_flags));

    // data structure containing all textures representing games pieces
    auto const assets = load_assets(main_renderer);

    check_sdl_failure(not main_renderer.get(), "Renderer creation");

    SDL_SetRenderDrawBlendMode(main_renderer.get(), SDL_BLENDMODE_BLEND);

    SDL_Rect tile{ .x = 0, .y = 0, .w = width / 8, .h = height / 8 };

    // generate 8 * 8 board and turn it into a texture
    auto const board = to_ptr(SDL_CreateTextureFromSurface(
      main_renderer.get(), generate_board().get()));

    check_sdl_failure(not board.get(), "Board texture");

    // TODO: handle screen resize and scale this in a rectangle
    SDL_Rect screen_rect{ .x = 0, .y = 0, .w = width, .h = height };

    SDL_RenderCopy(main_renderer.get(), board.get(), nullptr, &screen_rect);

    auto game_data = generate_default_game_data();

    for (auto const& arr : game_data.board) {
        for (auto const ptr : arr) {
            if (ptr != -1) {
                auto& [type, pos, colour] = game_data.pieces[ptr];

                tile.x = pos.col * 100;
                tile.y = pos.row * 100;

                SDL_RenderCopy(main_renderer.get(),
                               assets.pieces[colour][type],
                               nullptr,
                               &tile);
            }
        }
    }

    SDL_RenderPresent(main_renderer.get());

    std::cin.get();
}

