// FILE: DebugDraw implementation - see DebugDraw.hpp for the contract/EXTEND note.
#include "DebugDraw.hpp"
#include "../core/renderer.hpp"
#include <cstdlib>

namespace DebugDraw
{
    void marker(int x, int y, int size, SDL_Color color)
    {
        SDL_Renderer *r = Renderer::Instance()->getRenderer();
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        // Draw a diamond (isometric-friendly) by connecting four points.
        SDL_Point pts[5] = {
            { x,        y - size },
            { x + size, y        },
            { x,        y + size },
            { x - size, y        },
            { x,        y - size },
        };
        SDL_RenderDrawLines(r, pts, 5);
    }

    void line(int x1, int y1, int x2, int y2, SDL_Color color)
    {
        SDL_Renderer *r = Renderer::Instance()->getRenderer();
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(r, x1, y1, x2, y2);
    }

    void filledDiamond(int cx, int cy, int halfW, int halfH, SDL_Color color)
    {
        if (halfH <= 0) {
            return;
        }
        SDL_Renderer *r = Renderer::Instance()->getRenderer();
        // WATCH OUT: blend mode is turned on then back off around the fill -
        // if you add an early return in here, restore SDL_BLENDMODE_NONE
        // first, or every draw call after this one starts blending too.
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        // One horizontal scanline per pixel row, narrowing linearly towards
        // the top/bottom vertices - a cheap diamond fill with only line draws.
        for (int dy = -halfH; dy <= halfH; ++dy) {
            float t = 1.0f - (float)std::abs(dy) / (float)halfH;
            int rowHalfW = (int)(halfW * t);
            SDL_RenderDrawLine(r, cx - rowHalfW, cy + dy, cx + rowHalfW, cy + dy);
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}
