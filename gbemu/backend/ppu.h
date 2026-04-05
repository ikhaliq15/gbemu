#ifndef GBEMU_BACKEND_PPU_H
#define GBEMU_BACKEND_PPU_H

#include "gbemu/backend/cpu.h"
#include "gbemu/backend/timer.h"

// TODO: explore why top of window looks cut off (like in blargg tests)
namespace gbemu::backend
{

static constexpr uint16_t LCD_WIDTH = 160;
static constexpr uint16_t LCD_HEIGHT = 144;

// TODO: should technically be ~59.7.
#define DEVICE_FPS (60.0)

class PPU : public Timer::IListener, public RAM::Owner
{
  public:
    static constexpr uint64_t SCANLINE_FREQUENCY = 9352;
    static constexpr uint64_t CYCLES_PER_SCANLINE = 114;

    PPU(CPU *cpu);

    void init();
    void update();

  public:
    bool consumeCompletedFrame();

  public:
    // Timer::IListener
    void trigger() override;

    // RAM::Owner
    [[nodiscard]] uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  public:
    const std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> &getPixels() const;

  private:
    static constexpr uint16_t MAX_SPRITES_PER_SCANLINE = 10;

    static constexpr uint32_t COLOR_0 = 0xFFFFFFFF;
    static constexpr uint32_t COLOR_1 = 0xFFAAAAAA;
    static constexpr uint32_t COLOR_2 = 0xFF555555;
    static constexpr uint32_t COLOR_3 = 0xFF000000;

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

    uint64_t completedFrames_ = 0;

    CPU *cpu_;

    void drawScanLine();
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_PPU_H
