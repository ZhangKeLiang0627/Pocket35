#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

#if defined(SDL_h_)

void setup(void);
void loop(void);

namespace
{
    int runFrames(bool* running)
    {
        setup();
        do
        {
            loop();
        } while (*running);
        return 0;
    }
} // namespace

int main(int, char**) { return lgfx::Panel_sdl::main(runFrames, 1); }

#endif
