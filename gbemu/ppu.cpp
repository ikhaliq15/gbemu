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

                std::cout << "Drew frame #" << ++frameCount_ << std::endl;

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
        bool lcd_enabled = getBit(lcdc, 7) == 1;
        uint16_t window_tilemap = (getBit(lcdc, 6) == 1) ? 0x9C00 : 0x9800;
        bool window_enabled = getBit(lcdc, 5) == 1;
        uint16_t tile_data = (getBit(lcdc, 4) == 1) ? 0x8000 : 0x8800;
        uint16_t bg_tile_map = (getBit(lcdc, 3) == 1) ? 0x9C00 : 0x9800;
        bool sprite_eight_by_eight_mode = getBit(lcdc, 2) == 0;
        bool sprite_enabled = getBit(lcdc, 1) == 1;
        bool bg_enabled = getBit(lcdc, 0) == 1; // TODO: is bit 0 actually bg enable?

        /* Draw nothing if lcd is turned off. */
        if (!lcd_enabled) {
            for (int i = 0; i < pixels_.size(); i++)
                pixels_[i] = 0;
            return;
        }

        /* Setup background color palette. */
        uint32_t bg_palette[4];
        uint8_t bgp = cpu_->ram()->get(RAM::BGP);
        for (int i = 0; i < 4; i++) {
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

        /* Draw background for current scan line, if background is enabled. */
        if (bg_enabled) {
            uint8_t y = cpu_->ram()->get(RAM::LY);

            for (int x = 0; x < WINDOW_WIDTH; x++) {
                int tile_x = x / 8;
                int tile_y = y / 8;
                int tile_index = tile_y * 32 + tile_x;

                uint16_t tile;
                if (tile_data == 0x8000) {
                    tile = tile_data + 16 * cpu_->ram()->get(bg_tile_map + tile_index);
                } else {
                    tile = tile_data + 16 * (int8_t) cpu_->ram()->get(bg_tile_map + tile_index);
                }

                int inner_tile_x = x % 8;
                int inner_tile_y = y % 8;

                uint8_t lsb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y)), 7 - inner_tile_x);
                uint8_t msb = getBit(cpu_->ram()->get(tile + (2 * inner_tile_y) + 1), 7 - inner_tile_x);

                pixels_[y * WINDOW_WIDTH + x] = bg_palette[(msb << 1) | lsb];
            }
        }
    }

} // gbemu