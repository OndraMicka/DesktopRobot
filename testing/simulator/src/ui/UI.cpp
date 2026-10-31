#include "UI.h"

#include <cctype>
#include <string>

namespace desktoprobot
{
    namespace
    {
        constexpr int kFontWidth = 5;
        constexpr int kFontHeight = 7;

        const unsigned char fontData[38][5] = {
            {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
            {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
            {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
            {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
            {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
            {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
            {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
            {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
            {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
            {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
            {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
            {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
            {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
            {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
            {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
            {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
            {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
            {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
            {0x46, 0x49, 0x49, 0x49, 0x31}, // S
            {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
            {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
            {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
            {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
            {0x63, 0x14, 0x08, 0x14, 0x63}, // X
            {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
            {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
            {0x08, 0x08, 0x08, 0x08, 0x08}, // -
            {0x40, 0x20, 0x10, 0x08, 0x04}, // /
            {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
            {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
            {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
            {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
            {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
            {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
            {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
            {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
            {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
            {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
        };

        void renderGlyph(SDL_Renderer* renderer, char ch, int x, int y, int scale, SDL_Color color)
        {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

            if (ch == ' ')
            {
                return;
            }

            int glyphIndex = -1;
            if (ch >= 'A' && ch <= 'Z')
            {
                glyphIndex = ch - 'A';
            }
            else if (ch == '-')
            {
                glyphIndex = 26;
            }
            else if (ch == '/')
            {
                glyphIndex = 27;
            }
            else if (ch >= '0' && ch <= '9')
            {
                glyphIndex = 28 + ch - '0';
            }

            if (glyphIndex < 0)
            {
                return;
            }

            const unsigned char* source = fontData[glyphIndex];

            for (int row = 0; row < kFontHeight; ++row)
            {
                for (int col = 0; col < kFontWidth; ++col)
                {
                    if (source[col] & (1 << row))
                    {
                        SDL_Rect rect{ x + col * scale, y + row * scale, scale, scale };
                        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }
        }
    }

    bool pointInRect(int x, int y, const SDL_Rect& rect)
    {
        return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
    }

    void drawText(SDL_Renderer* renderer, const char* text, int x, int y, int scale, SDL_Color color)
    {
        int cursorX = x;
        for (const char* p = text; *p != '\0'; ++p)
        {
            if (*p == ' ')
            {
                cursorX += 6 * scale;
                continue;
            }

            renderGlyph(renderer, *p, cursorX, y, scale, color);
            cursorX += (kFontWidth + 1) * scale;
        }
    }

    void drawButton(SDL_Renderer* renderer, const Button& button)
    {
        const SDL_Color bg = button.active ? SDL_Color{ 28, 110, 220, 255 } : SDL_Color{ 48, 48, 52, 255 };
        const SDL_Color border = button.hovered ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 170, 170, 170, 255 };
        const SDL_Color textColor = { 255, 255, 255, 255 };

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &button.rect);

        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderDrawRect(renderer, &button.rect);

        const int labelLength = static_cast<int>(std::char_traits<char>::length(button.label));
        const int textScale = (labelLength * 12 + 16 > button.rect.w)
            ? 1
            : 2;
        const int textWidth = labelLength * 6 * textScale;
        const int textX = button.rect.x + (button.rect.w - textWidth) / 2;
        const int textY = button.rect.y + (button.rect.h / 2) - 4;
        drawText(renderer, button.label, textX, textY, textScale, textColor);
    }

    void drawPanelHeader(SDL_Renderer* renderer, int x, int y, int w, int h, const char* title)
    {
        SDL_Rect panel{ x, y, w, h };
        SDL_SetRenderDrawColor(renderer, 35, 35, 40, 255);
        SDL_RenderFillRect(renderer, &panel);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &panel);

        drawText(renderer, title, x + 10, y + 6, 2, SDL_Color{ 255, 255, 255, 255 });
    }
}
