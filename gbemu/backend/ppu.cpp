#include "gbemu/backend/ppu.h"

#include "gbemu/backend/interrupt_controller.h"
#include "gbemu/backend/ram.h"

#include <algorithm>
#include <span>

namespace gbemu::backend
{

PPU::PPU(RAM *ram, InterruptController *interruptController)
    : interruptController_(interruptController), ram_(ram), scy_(0x00), scx_(0x00), ly_(0x00), lyc_(0x00), wy_(0x00),
      wx_(0x00), lcdStatus_(0x00), windowLy_(0x00), lycCoincidenceCalledOnThisLy_(false)
{}

void PPU::init() { std::ranges::fill(pixels_, 0x00000000); }

void PPU::update()
{
    const auto lyLycMatch = ly_ == lyc_;
    lcdStatus_ = setBit(lcdStatus_, 2, lyLycMatch);

    if (lyLycMatch && !lycCoincidenceCalledOnThisLy_)
    {
        interruptController_->requestInterrupt(InterruptType::Stat);
        lycCoincidenceCalledOnThisLy_ = true;
    }
}

auto PPU::consumeCompletedFrame() -> bool
{
    if (completedFrames_ == 0)
    {
        return false;
    }

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
        interruptController_->requestInterrupt(InterruptType::VBlank);
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
    case RAM::SCY: return scy_;
    case RAM::SCX: return scx_;
    case RAM::LY: return ly_;
    case RAM::LYC: return lyc_;
    case RAM::STAT: return lcdStatus_;
    case RAM::WY: return wy_;
    case RAM::WX: return wx_;
    }
    return 0x00;
}

void PPU::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    switch (address)
    {
    case RAM::SCY: scy_ = newValue; break;
    case RAM::SCX: scx_ = newValue; break;
    case RAM::LY: break; // read-only
    case RAM::LYC: lyc_ = newValue; break;
    case RAM::STAT: lcdStatus_ = (newValue & 0xf8) | (lcdStatus_ & 0x03); break;
    case RAM::WY: wy_ = newValue; break;
    case RAM::WX: wx_ = newValue; break;
    }
}

const std::array<uint32_t, LCD_WIDTH * LCD_HEIGHT> &PPU::getPixels() const { return pixels_; }

constexpr auto PPU::buildPalette(uint8_t paletteRegister) const -> std::array<uint32_t, 4>
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
    {
        drawBackground(bgPalette, tileData, bgTileMap);
    }
    else
    {
        std::fill_n(&pixels_[ly_ * LCD_WIDTH], LCD_WIDTH, 0xffffffff);
    }

    if (windowEnabled && bgEnabled)
    {
        drawWindow(bgPalette, tileData, windowTileMap);
    }

    if (spriteEnabled)
    {
        drawSprites(spriteHeight);
    }
}

void PPU::drawBackground(const std::array<uint32_t, 4> &palette, uint16_t tileData, uint16_t tileMap)
{
    const auto viewPortY = (ly_ + scy_) % 256;
    const int tileRow = viewPortY / 8;
    const int innerY = viewPortY % 8;
    const int rowBase = tileRow * 32;
    auto *scanline = &pixels_[ly_ * LCD_WIDTH];

    int x = 0;
    while (x < LCD_WIDTH)
    {
        const auto viewPortX = (x + scx_) % 256;
        const int tileCol = viewPortX / 8;
        const int startInnerX = viewPortX % 8;
        const int pixelsInTile = std::min(8 - startInnerX, LCD_WIDTH - x);

        const int tileIndex = rowBase + tileCol;
        uint16_t tile;
        if (tileData == 0x8000)
        {
            tile = tileData + 16 * ram_->get(tileMap + tileIndex);
        }
        else
        {
            tile = tileData + 16 * static_cast<int8_t>(ram_->get(tileMap + tileIndex));
        }

        const uint8_t lo = ram_->get(tile + 2 * innerY);
        const uint8_t hi = ram_->get(tile + 2 * innerY + 1);

        for (int i = 0; i < pixelsInTile; i++)
        {
            const int bit = 7 - (startInnerX + i);
            const uint8_t colorId = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
            scanline[x + i] = palette[colorId];
        }

        x += pixelsInTile;
    }
}

void PPU::drawWindow(const std::array<uint32_t, 4> &palette, uint16_t tileData, uint16_t tileMap)
{
    const int windowY = ly_ - wy_;
    if (windowY < 0 || windowY >= LCD_HEIGHT)
    {
        return;
    }

    const int windowPixelY = windowLy_;
    const int tileRow = windowPixelY / 8;
    const int innerY = windowPixelY % 8;
    const int rowBase = tileRow * 32;
    auto *scanline = &pixels_[ly_ * LCD_WIDTH];

    // The window starts at screen x = wx_ - 7
    const int screenStart = std::max(0, static_cast<int>(wx_) - 7);
    if (screenStart >= LCD_WIDTH)
    {
        return;
    }

    bool windowVisible = false;
    int x = screenStart;
    while (x < LCD_WIDTH)
    {
        const int windowX = x - wx_ + 7;
        if (windowX < 0 || windowX >= LCD_WIDTH)
        {
            x++;
            continue;
        }
        windowVisible = true;

        const int tileCol = windowX / 8;
        const int startInnerX = windowX % 8;
        const int pixelsInTile = std::min(8 - startInnerX, LCD_WIDTH - x);

        const int tileIndex = rowBase + tileCol;
        uint16_t tile;
        if (tileData == 0x8000)
        {
            tile = tileData + 16 * ram_->get(tileMap + tileIndex);
        }
        else
        {
            tile = tileData + 16 * static_cast<int8_t>(ram_->get(tileMap + tileIndex));
        }

        const uint8_t lo = ram_->get(tile + 2 * innerY);
        const uint8_t hi = ram_->get(tile + 2 * innerY + 1);

        for (int i = 0; i < pixelsInTile; i++)
        {
            const int bit = 7 - (startInnerX + i);
            const uint8_t colorId = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
            scanline[x + i] = palette[colorId];
        }

        x += pixelsInTile;
    }

    if (windowVisible)
    {
        windowLy_ += 1;
    }
}

void PPU::drawSprites(uint16_t spriteHeight)
{
    struct SpriteInfo final
    {
        int yPos;
        int xPos;
        uint8_t tileIndex;
        uint8_t attributes;
        uint16_t oamPriority;
    };

    constexpr uint16_t SPRITE_TILE_DATA = 0x8000;

    std::array<SpriteInfo, MAX_SPRITES_PER_SCANLINE> selectedSprites;
    uint16_t spriteCount = 0;

    uint16_t oamIndex = 0;
    while (spriteCount < MAX_SPRITES_PER_SCANLINE && oamIndex < 40)
    {
        const uint16_t addr = RAM::OAM + (oamIndex * 4);
        const auto yPos = ram_->get(addr) - 16;

        if (yPos <= ly_ && ly_ < yPos + spriteHeight)
        {
            const auto xPos = ram_->get(addr + 1) - 8;

            selectedSprites[spriteCount++] = SpriteInfo{
                .yPos = yPos,
                .xPos = xPos,
                .tileIndex = ram_->get(addr + 2),
                .attributes = ram_->get(addr + 3),
                .oamPriority = oamIndex,
            };
        }
        oamIndex += 1;
    }

    // Lower x = higher priority; draw high-x first so low-x overwrites
    auto spriteSpan = std::span(selectedSprites.data(), spriteCount);
    std::ranges::sort(spriteSpan, [](const auto &a, const auto &b) {
        if (a.xPos == b.xPos)
        {
            return a.oamPriority > b.oamPriority;
        }
        return a.xPos > b.xPos;
    });

    const auto bgPalette = buildPalette(ram_->get(RAM::BGP));
    const auto objPalette0 = buildPalette(ram_->get(RAM::OBP0));
    const auto objPalette1 = buildPalette(ram_->get(RAM::OBP1));

    for (const auto &sprite : spriteSpan)
    {
        const auto yPos = sprite.yPos;
        const auto xPos = sprite.xPos;

        auto tileIndex = sprite.tileIndex;
        if (spriteHeight == 16)
        {
            tileIndex = setBit(tileIndex, 0, 0);
        }

        const auto attributes = sprite.attributes;
        const auto priority = getBit(attributes, 7);
        const auto yFlip = getBit(attributes, 6);
        const auto xFlip = getBit(attributes, 5);
        const auto &palette = getBit(attributes, 4) ? objPalette1 : objPalette0;

        const uint16_t tile = SPRITE_TILE_DATA + (16 * tileIndex);

        const auto innerY = (yFlip) ? (spriteHeight - 1) - (ly_ - yPos) : (ly_ - yPos);
        const auto tile1 = ram_->get(tile + (2 * innerY));
        const auto tile2 = ram_->get(tile + (2 * innerY) + 1);

        for (int x = xPos; x < xPos + 8; ++x)
        {
            if (x < 0 || x >= LCD_WIDTH)
            {
                continue;
            }

            int innerX = (x - xPos) % 8;
            if (xFlip)
            {
                innerX = 7 - innerX;
            }

            const uint8_t lsb = getBit(tile1, 7 - innerX);
            const uint8_t msb = getBit(tile2, 7 - innerX);
            const uint8_t colorId = (msb << 1) | lsb;

            if (colorId == 0)
            {
                continue;
            }

            if (!priority || pixels_[ly_ * LCD_WIDTH + x] == bgPalette[0])
            {
                pixels_[ly_ * LCD_WIDTH + x] = palette[colorId];
            }
        }
    }
}

} // namespace gbemu::backend
