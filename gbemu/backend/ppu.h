#ifndef GBEMU_BACKEND_PPU_H
#define GBEMU_BACKEND_PPU_H

#include "gbemu/backend/interrupt_controller.h"
#include "gbemu/backend/ram.h"
#include "gbemu/backend/timer.h"

#include <array>

namespace gbemu::backend
{

inline constexpr uint16_t LCD_WIDTH = 160;
inline constexpr uint16_t LCD_HEIGHT = 144;

class PPU : public Timer::IListener, public RAM::Owner
{
  public:
    static constexpr uint64_t SCANLINE_FREQUENCY = 9352;
    static constexpr uint64_t CYCLES_PER_SCANLINE = 114;

    PPU(RAM *ram, InterruptController *interruptController);

    void init();
    void update();
    bool consumeCompletedFrame();

    [[nodiscard]] const std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> &getPixels() const;

    // Timer::IListener
    void trigger() override;

    // RAM::Owner
    uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    // Display palette (ARGB)
    static constexpr uint32_t COLOR_WHITE = 0xFFFFFFFF;
    static constexpr uint32_t COLOR_LIGHT_GRAY = 0xFFAAAAAA;
    static constexpr uint32_t COLOR_DARK_GRAY = 0xFF555555;
    static constexpr uint32_t COLOR_BLACK = 0xFF000000;

    static constexpr uint16_t MAX_SPRITES_PER_SCANLINE = 10;

    void drawScanLine();
    void drawBackground(const std::array<uint32_t, 4> &palette, uint16_t tileData, uint16_t tileMap);
    void drawWindow(const std::array<uint32_t, 4> &palette, uint16_t tileData, uint16_t tileMap);
    void drawSprites(uint16_t spriteHeight);

    [[nodiscard]] std::array<uint32_t, 4> buildPalette(uint8_t paletteRegister) const;

    InterruptController *interruptController_;
    RAM *ram_;

    // LCD registers
    uint8_t scy_;
    uint8_t scx_;
    uint8_t ly_;
    uint8_t lyc_;
    uint8_t wy_;
    uint8_t wx_;
    uint8_t lcdStatus_;

    // Internal state
    uint8_t windowLy_;
    bool lycCoincidenceCalledOnThisLy_;
    uint64_t completedFrames_ = 0;

    std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> pixels_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_PPU_H
