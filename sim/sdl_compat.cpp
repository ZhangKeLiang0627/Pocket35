#include "sdl_compat.h"

#if !SDL_VERSION_ATLEAST(2, 0, 18)
int SDL_RenderSetVSync(SDL_Renderer*, int) { return 0; }
#endif
