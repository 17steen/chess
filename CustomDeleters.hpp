#include "SDL.h"
#include "SDL_render.h"
#include <memory>

struct WindowDeleter
{
    void operator()(SDL_Window* w) const noexcept { SDL_DestroyWindow(w); }
};

struct SurfaceDeleter
{
    void operator()(SDL_Surface* s) const noexcept { SDL_FreeSurface(s); }
};

struct TextureDeleter
{
    void operator()(SDL_Texture* t) const noexcept { SDL_DestroyTexture(t); }
};

struct RendererDeleter
{
    void operator()(SDL_Renderer* r) const noexcept { SDL_DestroyRenderer(r); }
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
