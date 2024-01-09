#ifndef GBEMU_PPU
#define GBEMU_PPU

#include <SDL2/SDL.h>

#include "cpu.h"

namespace gbemu {

    #define WINDOW_WIDTH 160
    #define WINDOW_HEIGHT 144

    // TODO: should technically be ~59.7.
    #define DEVICE_FPS (60.0)

    class PPU
    {
    public:
        PPU(std::shared_ptr<CPU> cpu);
        ~PPU();
        void init();
        void update();
        bool hasQuit();

    private:
        SDL_Event event_;
        SDL_Renderer *renderer_;
        SDL_Window *window_;
        SDL_Texture *texture_;

        std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> pixels_;

        bool quit_;

        std::shared_ptr<CPU> cpu_;
        size_t cycleCount_ = 0;

        size_t frameCount_ = 0;

        uint64_t lastFrameTickCount_ = 0;

        void drawScanLine();
    };

} // gbemu

#endif // GBEMU_PPU