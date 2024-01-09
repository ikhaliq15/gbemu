#include "ppu.h"

namespace gbemu {

    PPU::PPU(std::shared_ptr<CPU> cpu)
    : quit_(false)
    , cpu_(cpu)
    , cycleCount_(0)
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
        window_ = SDL_CreateWindow("GBEmu v3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

        for (int i = 0; i < pixels_.size(); i++)
            pixels_[i] = 0x00000000;
    }

    void PPU::update()
    {
        if (quit_)
            return;

        if (cpu_->cycles() - cycleCount_ >= 114)
        {
            cpu_->ram()->set(RAM::LY, cpu_->ram()->get(RAM::LY) + 1);
            const auto scanLine = cpu_->ram()->get(RAM::LY);
            
            if (scanLine < 144)
            {
                drawScanLine();
            } 
            else if (scanLine == 144)
            {
                // TODO: render frame
                while (SDL_PollEvent(&event_) == 1) {
                    if (event_.type == SDL_QUIT) {
                        quit_ = true;
                    }
                }

                SDL_UpdateTexture(texture_, NULL, pixels_.data(), WINDOW_WIDTH * sizeof(uint32_t));
                SDL_RenderCopy(renderer_, texture_, NULL, NULL);
                SDL_RenderPresent(renderer_);

                /* Sleep to achieve desired FPS. */
                // TODO: this mechanism is causing actual FPS to be a bit lower than desired FPS
                //       probably due to some overhead from SDL_Delay and the thread sleeping and restarting.
                //       from a quick test, i was getting approx 56.9 FPS when desired FPS was 60.
                const auto now = SDL_GetTicks64();
                if (lastFrameTickCount_ > 0)
                {
                    constexpr auto timeBetweenFramesMs = (Uint64) ((1.0 / DEVICE_FPS) * 1000);
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
            else if (scanLine > 153) // TODO: check if 153 is correct value here.
            {
                cpu_->ram()->set(RAM::LY, 0);
            } 

            cycleCount_ = cpu_->cycles();
        }
    }

    bool PPU::hasQuit() { return quit_; }

    void PPU::drawScanLine()
    {
        const auto lcdc = cpu_->ram()->get(RAM::LCDC);
        const auto lcd_enabled = getBit(lcdc, 7) == 1;
        const uint16_t window_tilemap = (getBit(lcdc, 6) == 1) ? 0x9C00 : 0x9800;
        const auto window_enabled = getBit(lcdc, 5) == 1;
        const uint16_t tile_data = (getBit(lcdc, 4) == 1) ? 0x8000 : 0x8800;
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
            switch (bgp & 0x03) {
                case 0:
                    bg_palette[i] = 0xFFFFFFFF;
                    break;
                case 1:
                    bg_palette[i] = 0xFFC0C0C0;
                    break;
                case 2:
                    bg_palette[i] = 0xFF606060;
                    break;
                case 3:
                    bg_palette[i] = 0xFF000000;
                    break;
            }
            bgp >>= 2;
        }

        const uint8_t y = cpu_->ram()->get(RAM::LY);

        /* Draw background for current scan line, if background is enabled. */
        if (bg_enabled)
        {
            for (int x = 0; x < WINDOW_WIDTH; x++) {
                int tile_x = x / 8;
                int tile_y = y / 8;
                int tile_index = tile_y * 32 + tile_x;

                uint16_t tile;
                if (tile_data == 0x8000) {
                    tile = tile_data + 16 * cpu_->ram()->get(bg_tile_map + tile_index);
                } else {
                    // TODO: does cast to signed value need to happen after multiply by 16?
                    tile = tile_data + 16 * (int8_t) cpu_->ram()->get(bg_tile_map + tile_index);
                }

                int inner_tile_x = x % 8;
                int inner_tile_y = y % 8;

                uint8_t lsb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y)), 7 - inner_tile_x);
                uint8_t msb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y) + 1), 7 - inner_tile_x);

                pixels_[y * WINDOW_WIDTH + x] = bg_palette[(msb << 1) | lsb];
            }
        }

        /* Draw sprites for current scan line, if sprites are enabled. */
        if (sprite_enabled)
        {
            // note - 40 = num of sprites in OAM
            for (uint16_t i = 0; i < 40; i++)
            {
                const uint16_t sprite = RAM::OAM + (i * 4);

                const auto sprite_y = cpu_->ram()->get(sprite) - 16;
                const auto sprite_x = cpu_->ram()->get(sprite + 1) - 8;

                /* If sprite does not appear on current scanline, then skip. */
                if (!(sprite_y <= y && y < sprite_y + sprite_height))
                    continue;

                auto sprite_tile_index = cpu_->ram()->get(sprite + 2);
                if (!sprite_eight_by_eight_mode)
                    sprite_tile_index = setBit(sprite_tile_index, 0, 0);

                const auto sprite_attributes = cpu_->ram()->get(sprite + 3);

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
                    switch (obp & 0x03) {
                        case 0:
                            palette[i] = 0xFFFFFFFF;
                            break;
                        case 1:
                            palette[i] = 0xFFC0C0C0;
                            break;
                        case 2:
                            palette[i] = 0xFF606060;
                            break;
                        case 3:
                            palette[i] = 0xFF000000;
                            break;
                    }
                    obp >>= 2;
                }

                for (int x = sprite_x; x < sprite_x + 8; x++)
                {
                    const int inner_tile_x = x % 8;
                    const int inner_tile_y = y % 8;

                    const uint8_t lsb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y)), 7 - inner_tile_x);
                    const uint8_t msb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y) + 1), 7 - inner_tile_x);

                    const uint8_t colorId = (msb << 1) | lsb;

                    if (colorId == 0)
                        continue;

                    pixels_[y * WINDOW_WIDTH + x] = palette[colorId];
                }
            }
        }
    }

} // gbemu