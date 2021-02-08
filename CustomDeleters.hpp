#include "SDL.h"
#include "SDL_render.h"
#include <memory>

using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
using RendererPtr =
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

inline WindowPtr
to_ptr(SDL_Window* w)
{
    return {
        w,
        &SDL_DestroyWindow,
    };
}

inline SurfacePtr
to_ptr(SDL_Surface* s)
{
    return {
        s,
        &SDL_FreeSurface,
    };
}

inline TexturePtr
to_ptr(SDL_Texture* t)
{
    return {
        t,
        &SDL_DestroyTexture,
    };
}

inline RendererPtr
to_ptr(SDL_Renderer* r)
{
    return {
        r,
        &SDL_DestroyRenderer,
    };
}
