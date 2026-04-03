#ifndef GBEMU_PPU
#define GBEMU_PPU

#include "gbemu/cpu.h"
#include "gbemu/shutdown_listener.h"
#include "gbemu/timer.h"

#include <SDL2/SDL.h>

#include <optional>

// TODO: explore why top of window looks cut off (like in blargg tests)
namespace gbemu
{

static constexpr uint16_t LCD_WIDTH = 160;
static constexpr uint16_t LCD_HEIGHT = 144;
static constexpr uint32_t WINDOW_SCALE = 3;
static constexpr uint32_t WINDOW_WIDTH = LCD_WIDTH * WINDOW_SCALE;
static constexpr uint32_t WINDOW_HEIGHT = LCD_HEIGHT * WINDOW_SCALE;

// TODO: should technically be ~59.7.
#define DEVICE_FPS (60.0)

class PPU : public Timer::TimerListener, public RAM::Owner, public gbemu::ShutDownListener
{
  public:
    class FrameCompleteListener
    {
      public:
        FrameCompleteListener() = default;
        virtual ~FrameCompleteListener() = default;
        virtual void onFrameComplete() = 0;
    };

    static constexpr uint64_t SCANLINE_FREQUENCY = 9352;
    static constexpr uint64_t CYCLES_PER_SCANLINE = 114;

    PPU(CPU *cpu);
    PPU(CPU *cpu, bool runHeadless = false, std::optional<std::string> displayDumpPath = std::nullopt);
    ~PPU();

    void init();
    void update();

  public:
    // Timer::TimerListener
    void timerTriggerHandler() override;

    // RAM::Owner
    [[nodiscard]] uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

    // ShutDownListener
    void onShutDown() override;

    void subscribeToCompleteFrames(FrameCompleteListener *frameCompleteListener)
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

    CPU *cpu_;

    size_t frameCount_ = 0;

    uint64_t lastFrameTickCount_ = 0;

    std::vector<FrameCompleteListener *> frameCompleteListeners_;

    const bool runHeadless_;
    const std::optional<std::string> displayDumpPath_;

    void drawScanLine();
    [[nodiscard]] std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> scalePixels(uint32_t scaleFactor) const;

    void dumpDisplay(const std::string &path);
};

} // namespace gbemu

#endif // GBEMU_PPU
