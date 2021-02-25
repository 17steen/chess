#include "SDL.h"
#include "SDL_image.h"
#include "SDL_render.h"
#include <memory>

inline void
check_sdl_failure(bool b, char const* what)
{
    if (b) {
        SDL_Log("Error : %s, %s\n", what, SDL_GetError());
        std::exit(1);
    }
}

inline void
check_img_failure(bool b, char const* what)
{
    if (b) {
        SDL_Log("Error : %s, %s\n", what, IMG_GetError());
        std::exit(1);
    }
}

struct SDL2
{
    [[nodiscard]] inline SDL2(unsigned flags)
    {
        auto const ec = SDL_Init(flags);
        check_sdl_failure(ec != 0, "SDL2 initialization");
    }

    [[nodiscard]] inline SDL2()
      : SDL2(SDL_INIT_EVERYTHING)
    {}

    inline ~SDL2() { SDL_Quit(); }
};

struct WindowDeleter
{
    void operator()(SDL_Window* w) const noexcept
    {
        SDL_Log("Destroying Window.");
        SDL_DestroyWindow(w);
    }
};

struct SurfaceDeleter
{
    void operator()(SDL_Surface* s) const noexcept
    {
        SDL_Log("Destroying Surface");
        SDL_FreeSurface(s);
    }
};

struct TextureDeleter
{
    void operator()(SDL_Texture* t) const noexcept
    {
        SDL_Log("Destroying Texture");
        SDL_DestroyTexture(t);
    }
};

struct RendererDeleter
{
    void operator()(SDL_Renderer* r) const noexcept
    {
        SDL_Log("Destroying Renderer");
        SDL_DestroyRenderer(r);
    }
};

using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

static_assert(sizeof(RendererPtr) == sizeof(SDL_Renderer*));

inline WindowPtr
to_ptr(SDL_Window* w)
{
    return WindowPtr{ w };
}

inline SurfacePtr
to_ptr(SDL_Surface* s)
{
    return SurfacePtr{ s };
}

inline TexturePtr
to_ptr(SDL_Texture* t)
{
    return TexturePtr{ t };
}

inline RendererPtr
to_ptr(SDL_Renderer* r)
{
    return RendererPtr{ r };
}
