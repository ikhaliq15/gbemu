#ifndef GBEMU_PPU
#define GBEMU_PPU

#include <SDL2/SDL.h>

#include "cpu.h"
#include "timer.h"

namespace gbemu {

    #define LCD_WIDTH 160
    #define LCD_HEIGHT 144
    #define WINDOW_SCALE 3
    #define WINDOW_WIDTH (LCD_WIDTH * WINDOW_SCALE)
    #define WINDOW_HEIGHT (LCD_HEIGHT * WINDOW_SCALE)

    // TODO: should technically be ~59.7.
    #define DEVICE_FPS (60.0)

    class PPU: public Timer::CycleListener, public RAM::Owner
    {
    public:
        class FrameCompleteListener
        {
        public:
            FrameCompleteListener(){}
            virtual ~FrameCompleteListener(){}
            virtual void onFrameComplete() = 0;
        };

        static constexpr uint64_t CYCLES_PER_SCANLINE = 114;

        PPU(std::shared_ptr<CPU> cpu);
        ~PPU();
        void init();

        void cycleTriggerHandler(uint64_t cycleCount);

        uint8_t onReadOwnedByte(uint16_t address);
        void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

        void subscribeToCompleteFrames(const std::shared_ptr<FrameCompleteListener> frameCompleteListener)
        {
            frameCompleteListeners_.push_back(frameCompleteListener);
        }

    private:
        SDL_Renderer *renderer_;
        SDL_Window *window_;
        SDL_Texture *texture_;

        std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> pixels_;

        uint8_t ly_;

        std::shared_ptr<CPU> cpu_;

        size_t frameCount_ = 0;

        uint64_t lastFrameTickCount_ = 0;

        std::vector<std::shared_ptr<FrameCompleteListener>> frameCompleteListeners_;

        void drawScanLine();
        std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> scalePixels(uint32_t scaleFactor) const;
    };

} // gbemu

#endif // GBEMU_PPU