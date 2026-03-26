#ifndef GBEMU_PPU
#define GBEMU_PPU

#include <SDL2/SDL.h>

#include "gbemu/cpu.h"
#include "gbemu/shutdown_listener.h"
#include "gbemu/timer.h"

// TODO: explore why top of window looks cut off (like in blargg tests)
namespace gbemu
{

#define LCD_WIDTH 160
#define LCD_HEIGHT 144
#define WINDOW_SCALE 3
#define WINDOW_WIDTH (LCD_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (LCD_HEIGHT * WINDOW_SCALE)

// TODO: should technically be ~59.7.
#define DEVICE_FPS (60.0)

class PPU : public Timer::TimerListener, public RAM::Owner, public gbemu::ShutDownListener
{
  public:
    class FrameCompleteListener
    {
      public:
        FrameCompleteListener()
        {
        }
        virtual ~FrameCompleteListener()
        {
        }
        virtual void onFrameComplete() = 0;
    };

    static constexpr uint64_t SCANLINE_FREQUENCY = 9352;
    static constexpr uint64_t CYCLES_PER_SCANLINE = 114;

    PPU(std::shared_ptr<CPU> cpu);
    PPU(std::shared_ptr<CPU> cpu, bool runHeadless = false, std::optional<std::string> displayDumpPath = std::nullopt);
    ~PPU();
    void init();
    void update();

    void timerTriggerHandler();

    uint8_t onReadOwnedByte(uint16_t address);
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

    // ShutDownListener
    void onShutDown() override;

    void subscribeToCompleteFrames(const std::shared_ptr<FrameCompleteListener> frameCompleteListener)
    {
        frameCompleteListeners_.push_back(frameCompleteListener);
    }

  private:
    static constexpr uint16_t MAX_SPRITES_PER_SCANLINE = 10;

    static constexpr uint32_t COLOR_0 = 0xFFFFFFFF;
    static constexpr uint32_t COLOR_1 = 0xFFAAAAAA;
    static constexpr uint32_t COLOR_2 = 0xFF555555;
    static constexpr uint32_t COLOR_3 = 0xFF000000;

    SDL_Renderer *renderer_;
    SDL_Window *window_;
    SDL_Texture *texture_;

    std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> pixels_;

    uint8_t scy_;
    uint8_t scx_;
    uint8_t ly_;
    uint8_t lyc_;
    uint8_t wy_;
    uint8_t wx_;
    uint8_t lcdStatus_;

    uint8_t windowLy_;

    bool lycCoincidenceCalledOnThisLy_;

    std::shared_ptr<CPU> cpu_;

    size_t frameCount_ = 0;

    uint64_t lastFrameTickCount_ = 0;

    std::vector<std::shared_ptr<FrameCompleteListener>> frameCompleteListeners_;

    const bool runHeadless_;
    const std::optional<std::string> displayDumpPath_;

    void drawScanLine();
    std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> scalePixels(uint32_t scaleFactor) const;

    void dumpDisplay(const std::string &path);
};

} // namespace gbemu

#endif // GBEMU_PPU