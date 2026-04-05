#include "gbemu/backend/ppu.h"
#include "gbemu/backend/ram.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace gbemu::backend
{

PPU::PPU(CPU *cpu, RAM *ram)
    : cpu_(cpu), ram_(ram), scy_(0x00), scx_(0x00), ly_(0x00), lyc_(0x00), wy_(0x00), wx_(0x00), lcdStatus_(0x00),
      windowLy_(0x00), lycCoincidenceCalledOnThisLy_(false)
{}

void PPU::init()
{
    std::ranges::fill(pixels_, 0x00000000);
}

void PPU::update()
{
    const auto lyLycMatch = ly_ == lyc_;
    lcdStatus_ = setBit(lcdStatus_, 2, lyLycMatch);

    if (lyLycMatch && !lycCoincidenceCalledOnThisLy_)
    {
        cpu_->requestInterrupt(CPU::Interrupt::STAT);
        lycCoincidenceCalledOnThisLy_ = true;
    }
}

bool PPU::consumeCompletedFrame()
{
    if (completedFrames_ == 0)
        return false;

    completedFrames_ -= 1;
    return true;
}

void PPU::trigger()
{
    if (ly_ < LCD_HEIGHT)
    {
        drawScanLine();
    }
    else if (ly_ == LCD_HEIGHT)
    {
        completedFrames_ += 1;
        cpu_->requestInterrupt(CPU::Interrupt::VBLANK);
    }

    ly_ += 1;
    lycCoincidenceCalledOnThisLy_ = false;

    if (ly_ > 153)
    {
        ly_ = 0;
        windowLy_ = 0;
    }
}

auto PPU::onReadOwnedByte(uint16_t address) -> uint8_t
{
    switch (address)
    {
    case RAM::SCY:
        return scy_;
    case RAM::SCX:
        return scx_;
    case RAM::LY:
        return ly_;
    case RAM::LYC:
        return lyc_;
    case RAM::STAT:
        return lcdStatus_;
    case RAM::WY:
        return wy_;
    case RAM::WX:
        return wx_;
    }
    return 0x00;
}

void PPU::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    switch (address)
    {
    case RAM::SCY:
        scy_ = newValue;
        break;
    case RAM::SCX:
        scx_ = newValue;
        break;
    case RAM::LY:
        break; // read-only
    case RAM::LYC:
        lyc_ = newValue;
        break;
    case RAM::STAT:
        lcdStatus_ = (newValue & 0xf8) | (lcdStatus_ & 0x03);
        break;
    case RAM::WY:
        wy_ = newValue;
        break;
    case RAM::WX:
        wx_ = newValue;
        break;
    }
}

const std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> &PPU::getPixels() const
{
    return pixels_;
}

std::array<uint32_t, 4> PPU::buildPalette(uint8_t paletteRegister) const
{
    constexpr std::array<uint32_t, 4> COLORS = {COLOR_WHITE, COLOR_LIGHT_GRAY, COLOR_DARK_GRAY, COLOR_BLACK};

    std::array<uint32_t, 4> palette;
    for (auto &entry : palette)
    {
        entry = COLORS[paletteRegister & 0x03];
        paletteRegister >>= 2;
    }
    return palette;
}

void PPU::drawScanLine()
{
    const auto lcdc = ram_->get(RAM::LCDC);

    const auto lcdEnabled = getBit(lcdc, 7);
    const uint16_t windowTileMap = getBit(lcdc, 6) ? 0x9C00 : 0x9800;
    const auto windowEnabled = getBit(lcdc, 5);
    const uint16_t tileData = getBit(lcdc, 4) ? 0x8000 : 0x9000;
    const uint16_t bgTileMap = getBit(lcdc, 3) ? 0x9C00 : 0x9800;
    const auto spriteEightByEight = !getBit(lcdc, 2);
    const uint16_t spriteHeight = spriteEightByEight ? 8 : 16;
    const auto spriteEnabled = getBit(lcdc, 1);
    const auto bgEnabled = getBit(lcdc, 0); // TODO: is bit 0 actually bg enable?

    if (!lcdEnabled)
    {
        std::ranges::fill(pixels_, 0xffffffff);
        return;
    }

    const auto bgPalette = buildPalette(ram_->get(RAM::BGP));

    if (bgEnabled)
        drawBackground(bgPalette, tileData, bgTileMap);
    else
    {
        std::fill_n(&pixels_[ly_ * LCD_WIDTH], LCD_WIDTH, 0xffffffff);
    }

    if (windowEnabled && bgEnabled)
        drawWindow(bgPalette, tileData, windowTileMap);

    if (spriteEnabled)
        drawSprites(spriteHeight);
}

void PPU::drawBackground(const std::array<uint32_t, 4> &palette, uint16_t tileData, uint16_t tileMap)
{
    for (int x = 0; x < LCD_WIDTH; x++)
    {
        const auto viewPortX = (x + scx_) % 256;
        const auto viewPortY = (ly_ + scy_) % 256;

        const int tileIndex = (viewPortY / 8) * 32 + (viewPortX / 8);

        uint16_t tile;
        if (tileData == 0x8000)
            tile = tileData + 16 * ram_->get(tileMap + tileIndex);
        else
            tile = tileData + 16 * static_cast<int8_t>(ram_->get(tileMap + tileIndex));

        const int innerX = viewPortX % 8;
        const int innerY = viewPortY % 8;

        const uint8_t lsb = getBit(ram_->get(tile + (2 * innerY)), 7 - innerX);
        const uint8_t msb = getBit(ram_->get(tile + (2 * innerY) + 1), 7 - innerX);

        pixels_[ly_ * LCD_WIDTH + x] = palette[(msb << 1) | lsb];
    }
}

void PPU::drawWindow(const std::array<uint32_t, 4> &palette, uint16_t tileData, uint16_t tileMap)
{
    const int windowY = ly_ - wy_;
    const int windowPixelY = windowLy_;

    bool windowVisible = false;
    for (int x = 0; x < LCD_WIDTH; x++)
    {
        const int windowX = x - wx_ + 7;

        if (!(0 <= windowX && windowX < LCD_WIDTH && 0 <= windowY && windowY < LCD_HEIGHT))
            continue;
        windowVisible = true;

        const int tileIndex = (windowPixelY / 8) * 32 + (windowX / 8);

        uint16_t tile;
        if (tileData == 0x8000)
            tile = tileData + 16 * ram_->get(tileMap + tileIndex);
        else
            tile = tileData + 16 * static_cast<int8_t>(ram_->get(tileMap + tileIndex));

        const int innerX = windowX % 8;
        const int innerY = windowPixelY % 8;

        const uint8_t lsb = getBit(ram_->get(tile + (2 * innerY)), 7 - innerX);
        const uint8_t msb = getBit(ram_->get(tile + (2 * innerY) + 1), 7 - innerX);

        pixels_[ly_ * LCD_WIDTH + x] = palette[(msb << 1) | lsb];
    }

    if (windowVisible)
        windowLy_ += 1;
}

void PPU::drawSprites(uint16_t spriteHeight)
{
    constexpr uint16_t SPRITE_TILE_DATA = 0x8000;

    std::vector<std::pair<uint8_t, int>> selectedSprites;
    selectedSprites.reserve(MAX_SPRITES_PER_SCANLINE);

    for (uint16_t i = 0; i < 40 && selectedSprites.size() < MAX_SPRITES_PER_SCANLINE; i++)
    {
        const uint16_t addr = RAM::OAM + (i * 4);
        const uint8_t spriteY = ram_->get(addr) - 16;

        if (spriteY <= ly_ && ly_ < spriteY + spriteHeight)
            selectedSprites.emplace_back(ram_->get(addr + 1) - 8, i);
    }

    // Lower x = higher priority; draw high-x first so low-x overwrites
    std::ranges::sort(selectedSprites, std::greater<>{});

    const auto bgPalette = buildPalette(ram_->get(RAM::BGP));

    for (const auto [spriteX, spriteIndex] : selectedSprites)
    {
        const uint16_t addr = RAM::OAM + (spriteIndex * 4);
        const auto yPos = ram_->get(addr) - 16;
        const auto xPos = ram_->get(addr + 1) - 8;

        auto tileIndex = ram_->get(addr + 2);
        if (spriteHeight == 16)
            tileIndex = setBit(tileIndex, 0, 0);

        const auto attributes = ram_->get(addr + 3);
        const auto priority = getBit(attributes, 7);
        const auto yFlip = getBit(attributes, 6);
        const auto xFlip = getBit(attributes, 5);
        const auto paletteAddr = getBit(attributes, 4) ? RAM::OBP1 : RAM::OBP0;

        const uint16_t tile = SPRITE_TILE_DATA + (16 * tileIndex);
        const auto palette = buildPalette(ram_->get(paletteAddr));

        for (int x = xPos; x < xPos + 8; x++)
        {
            if (x < 0 || x >= LCD_WIDTH)
                continue;

            int innerX = (x - xPos) % 8;
            if (xFlip)
                innerX = 7 - innerX;

            int innerY = (ly_ - yPos) % spriteHeight;
            if (yFlip)
                innerY = spriteHeight - innerY - 1;

            const uint8_t lsb = getBit(ram_->get(tile + (2 * innerY)), 7 - innerX);
            const uint8_t msb = getBit(ram_->get(tile + (2 * innerY) + 1), 7 - innerX);
            const uint8_t colorId = (msb << 1) | lsb;

            if (colorId == 0)
                continue;

            if (!priority || pixels_[ly_ * LCD_WIDTH + x] == bgPalette[0])
                pixels_[ly_ * LCD_WIDTH + x] = palette[colorId];
        }
    }
}

} // namespace gbemu::backend
