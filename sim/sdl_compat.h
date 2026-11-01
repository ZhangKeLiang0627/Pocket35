#pragma once

#ifdef __cplusplus

#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2, 0, 18)
extern "C++" int SDL_RenderSetVSync(SDL_Renderer* renderer, int vsync);
#endif

#endif
