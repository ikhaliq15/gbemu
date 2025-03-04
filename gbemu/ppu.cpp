#include "ppu.h"

#include <queue>

namespace gbemu
{

PPU::PPU(std::shared_ptr<CPU> cpu)
    : scy_(0x00), scx_(0x00), ly_(0x00), lyc_(0x00), wy_(0x00), wx_(0x00), windowLy_(0x00),
      lycCoincidenceCalledOnThisLy_(false), cpu_(cpu)
{
}

PPU::~PPU()
{
    if (renderer_)
        SDL_DestroyRenderer(renderer_);
    if (window_)
        SDL_DestroyWindow(window_);
    if (texture_)
        SDL_DestroyTexture(texture_);
    SDL_Quit();
}

void PPU::init()
{
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    window_ =
        SDL_CreateWindow("GBEmu v3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH,
                                 WINDOW_HEIGHT);

    for (int i = 0; i < pixels_.size(); i++)
        pixels_[i] = 0x00000000;
}

void PPU::update()
{
    const auto lyLycMatch = ly_ == lyc_;
    lcdStatus_ = setBit(lcdStatus_, 2, (lyLycMatch) ? 1 : 0);
    if (lyLycMatch && !lycCoincidenceCalledOnThisLy_)
    {
        cpu_->requestInterupt(CPU::Interrupt::STAT);
        lycCoincidenceCalledOnThisLy_ = true;
    }
}

void PPU::timerTriggerHandler()
{
    if (ly_ < 144)
    {
        drawScanLine();
    }
    else if (ly_ == 144)
    {
        // TODO: render frame
        const auto scaledPixels = scalePixels(WINDOW_SCALE);
        SDL_UpdateTexture(texture_, NULL, scaledPixels.data(), WINDOW_WIDTH * sizeof(uint32_t));
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);

        for (const auto &frameCompleteListener : frameCompleteListeners_)
            frameCompleteListener->onFrameComplete();

        /* Sleep to achieve desired FPS. */
        // TODO: this mechanism is causing actual FPS to be a bit lower than desired FPS
        //       probably due to some overhead from SDL_Delay and the thread sleeping and restarting.
        //       from a quick test, i was getting approx 56.9 FPS when desired FPS was 60.
        const auto now = SDL_GetTicks64();
        if (lastFrameTickCount_ > 0)
        {
            constexpr auto timeBetweenFramesMs = (Uint64)((1.0 / DEVICE_FPS) * 1000);
            const auto timeToSleepUntil = lastFrameTickCount_ + timeBetweenFramesMs;

            // TODO: add warning once we keep a moving average of the FPS (otherwise errors are too jiterry)
            // if (timeToSleepUntil < now)
            //     std::cout << "[WARNING] Rendering unable to keep up with desired framerate." << std::endl;

            const auto delayAmount = (timeToSleepUntil > now) ? timeToSleepUntil - now : 0ull;
            SDL_Delay(delayAmount);
        }
        lastFrameTickCount_ = SDL_GetTicks64();

        // std::cout << "Drew frame #" << ++frameCount_ << std::endl;

        // TODO: request VBLANK interrupt
        cpu_->requestInterupt(CPU::Interrupt::VBLANK);
    }

    ly_ += 1;
    lycCoincidenceCalledOnThisLy_ = false;

    if (ly_ > 153) // TODO: check if 153 is correct value here.
    {
        ly_ = 0;
        windowLy_ = 0;
    }
}

uint8_t PPU::onReadOwnedByte(uint16_t address)
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
    // TODO: exception?
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
        break;
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
    // TODO: exception?
}

void PPU::drawScanLine()
{
    const auto lcdc = cpu_->ram()->get(RAM::LCDC);
    const auto lcd_enabled = getBit(lcdc, 7) == 1;
    const uint16_t window_tilemap = (getBit(lcdc, 6) == 1) ? 0x9C00 : 0x9800;
    const auto window_enabled = getBit(lcdc, 5) == 1;
    const uint16_t tile_data = (getBit(lcdc, 4) == 1) ? 0x8000 : 0x9000;
    const uint16_t bg_tile_map = (getBit(lcdc, 3) == 1) ? 0x9C00 : 0x9800;
    const auto sprite_eight_by_eight_mode = getBit(lcdc, 2) == 0;
    const uint16_t sprite_height = (sprite_eight_by_eight_mode) ? 8 : 16;
    const auto sprite_enabled = getBit(lcdc, 1) == 1;
    const uint16_t sprite_tile_data = 0x8000;
    const auto bg_enabled = getBit(lcdc, 0) == 1; // TODO: is bit 0 actually bg enable?

    /* Draw nothing if lcd is turned off. */
    if (!lcd_enabled)
    {
        for (int i = 0; i < pixels_.size(); i++)
            pixels_[i] = 0;
        return;
    }

    /* Setup background color palette. */
    uint32_t bg_palette[4];
    uint8_t bgp = cpu_->ram()->get(RAM::BGP);
    for (int i = 0; i < 4; i++)
    {
        switch (bgp & 0x03)
        {
        case 0:
            bg_palette[i] = COLOR_0;
            break;
        case 1:
            bg_palette[i] = COLOR_1;
            break;
        case 2:
            bg_palette[i] = COLOR_2;
            break;
        case 3:
            bg_palette[i] = COLOR_3;
            break;
        }
        bgp >>= 2;
    }

    const uint8_t y = ly_;

    /* Draw background for current scan line, if background is enabled. */
    // TODO: if not enabled, background is white
    if (bg_enabled)
    {
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            const auto viewPortX = (x + scx_) % 256;
            const auto viewPortY = (y + scy_) % 256;

            int tile_x = viewPortX / 8;
            int tile_y = viewPortY / 8;
            int tile_index = tile_y * 32 + tile_x;

            uint16_t tile;
            if (tile_data == 0x8000)
            {
                tile = tile_data + 16 * cpu_->ram()->get(bg_tile_map + tile_index);
            }
            else
            {
                // TODO: does cast to signed value need to happen after multiply by 16?
                tile = tile_data + 16 * (int8_t)cpu_->ram()->get(bg_tile_map + tile_index);
            }

            int inner_tile_x = viewPortX % 8;
            int inner_tile_y = viewPortY % 8;

            uint8_t lsb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y)), 7 - inner_tile_x);
            uint8_t msb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y) + 1), 7 - inner_tile_x);

            pixels_[y * LCD_WIDTH + x] = bg_palette[(msb << 1) | lsb];
        }
    }
    else
    {
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            pixels_[y * LCD_WIDTH + x] = 0xffffffff;
        }
    }

    /* Draw window for current scan line, if window is enabled. */
    // TODO: if not enabled, window is white
    if (window_enabled && bg_enabled)
    {
        const int window_y = y - wy_;
        const int window_pixel_y = windowLy_;

        bool windowVisible = false;
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            const int window_x = x - wx_ + 7;

            if (!(0 <= window_x && window_x < LCD_WIDTH && 0 <= window_y && window_y < LCD_HEIGHT))
                continue;

            int tile_x = window_x / 8;
            int tile_y = window_pixel_y / 8;
            int tile_index = tile_y * 32 + tile_x;

            uint16_t tile;
            if (tile_data == 0x8000)
            {
                tile = tile_data + 16 * cpu_->ram()->get(window_tilemap + tile_index);
            }
            else
            {
                // TODO: does cast to signed value need to happen after multiply by 16?
                tile = tile_data + 16 * (int8_t)cpu_->ram()->get(window_tilemap + tile_index);
            }

            int inner_tile_x = window_x % 8;
            int inner_tile_y = window_pixel_y % 8;

            uint8_t lsb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y)), 7 - inner_tile_x);
            uint8_t msb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y) + 1), 7 - inner_tile_x);

            windowVisible = true;
            pixels_[y * LCD_WIDTH + x] = bg_palette[(msb << 1) | lsb];
        }
        if (windowVisible)
            windowLy_ += 1;
    }

    /* Draw sprites for current scan line, if sprites are enabled. */
    if (sprite_enabled)
    {
        // TODO: handle sprite drawing priority case where sprites overlap at same x,y but starting x are not the same.
        std::priority_queue<std::pair<uint8_t, int>> selectedSprites;
        for (uint16_t i = 0; i < 40 && selectedSprites.size() < MAX_SPRITES_PER_SCANLINE; i++)
        {
            const uint16_t spriteAddress = RAM::OAM + (i * 4);
            const uint8_t spriteY = cpu_->ram()->get(spriteAddress) - 16;
            const uint8_t spriteX = cpu_->ram()->get(spriteAddress + 1) - 8;

            /* If sprite does not appear on current scanline, then skip. */
            if (!(spriteY <= y && y < spriteY + sprite_height))
                continue;

            selectedSprites.push(std::make_pair(spriteX, i));
        }

        while (!selectedSprites.empty())
        {
            const auto selectedSprite = selectedSprites.top();
            selectedSprites.pop();

            const uint16_t spriteAddress = RAM::OAM + (std::get<1>(selectedSprite) * 4);

            const auto sprite_y = cpu_->ram()->get(spriteAddress) - 16;
            const auto sprite_x = cpu_->ram()->get(spriteAddress + 1) - 8;

            auto sprite_tile_index = cpu_->ram()->get(spriteAddress + 2);
            if (!sprite_eight_by_eight_mode)
                sprite_tile_index = setBit(sprite_tile_index, 0, 0);

            const auto sprite_attributes = cpu_->ram()->get(spriteAddress + 3);

            const auto sprite_priority = getBit(sprite_attributes, 7) == 1;
            const auto sprite_y_flip = getBit(sprite_attributes, 6) == 1;
            const auto sprite_x_flip = getBit(sprite_attributes, 5) == 1;
            const auto sprite_palette_flag = getBit(sprite_attributes, 4);
            const auto sprite_palette = (sprite_palette_flag == 0) ? RAM::OBP0 : RAM::OBP1;

            const uint16_t tile = sprite_tile_data + (16 * sprite_tile_index);

            /* Setup object color palette. */
            uint32_t palette[4];
            uint8_t obp = cpu_->ram()->get(sprite_palette);
            for (int i = 0; i < 4; i++)
            {
                switch (obp & 0x03)
                {
                case 0:
                    palette[i] = COLOR_0;
                    break;
                case 1:
                    palette[i] = COLOR_1;
                    break;
                case 2:
                    palette[i] = COLOR_2;
                    break;
                case 3:
                    palette[i] = COLOR_3;
                    break;
                }
                obp >>= 2;
            }

            for (int x = sprite_x; x < sprite_x + 8; x++)
            {
                int inner_tile_x = (x - sprite_x) % 8;
                if (sprite_x_flip)
                    inner_tile_x = 7 - inner_tile_x;
                int inner_tile_y = (y - sprite_y) % sprite_height; // TODO: is this 8 or sprite_height?
                if (sprite_y_flip)
                    inner_tile_y = sprite_height - inner_tile_y - 1;

                if (x < 0 || x >= LCD_WIDTH)
                    continue;

                const uint8_t lsb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y)), 7 - inner_tile_x);
                const uint8_t msb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y) + 1), 7 - inner_tile_x);

                const uint8_t colorId = (msb << 1) | lsb;

                if (colorId == 0)
                    continue;

                if (!sprite_priority || pixels_[y * LCD_WIDTH + x] == bg_palette[0])
                    pixels_[y * LCD_WIDTH + x] = palette[colorId];
            }
        }
    }
}

std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> PPU::scalePixels(uint32_t scaleFactor) const
{
    // TODO: optimze this function.
    std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> scaledPixels;
    for (size_t i = 0; i < LCD_WIDTH; i++)
    {
        for (size_t j = 0; j < LCD_HEIGHT; j++)
        {
            const auto pixelValue = pixels_[j * LCD_WIDTH + i];
            for (int scaledI = i * scaleFactor; scaledI < (i + 1) * scaleFactor; scaledI++)
            {
                for (int scaledJ = j * scaleFactor; scaledJ < (j + 1) * scaleFactor; scaledJ++)
                {
                    scaledPixels[scaledJ * WINDOW_WIDTH + scaledI] = pixelValue;
                }
            }
        }
    }
    return scaledPixels;
}

} // namespace gbemu