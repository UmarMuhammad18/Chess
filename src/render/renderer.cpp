#include "renderer.hpp"
#include <GL/glew.h>

namespace render {
    namespace {
        enum Mode { SOLID = 0, TEXTURE = 1, KEYED_TEXTURE = 2, DISC = 3, RING = 4 };

        // Tight pixel bounds of each sprite in pieces.png (1024x1024), indexed [color][piece]
        // in engine order P, N, B, R, Q, K. The atlas grid is irregular: black N/R/P sit in
        // half-height cells, so a uniform UV grid can't be used.
        struct Sprite { int x0, y0, x1, y1; };
        constexpr Sprite SPRITES[2][6] = {
            { {880, 242, 996, 441}, {529, 214, 665, 441}, {363, 166, 490, 441},
              {702, 222, 833, 441}, {190, 125, 322, 441}, { 19,  76, 151, 441} },
            { {888, 822, 988, 987}, {534, 789, 659, 988}, {361, 644, 492, 984},
              {708, 810, 827, 987}, {190, 640, 322, 983}, { 19, 611, 151, 984} },
        };
        // Rendered height as a fraction of the square, so both colours match in size
        constexpr float PIECE_HEIGHT[6] = { 0.58f, 0.68f, 0.74f, 0.64f, 0.80f, 0.86f };
        constexpr int SPRITE_PAD = 3;
        constexpr float ATLAS_SIZE = 1024.0f;
    }

    bool Renderer::init(const std::string& asset_dir) {
        if (!gl.init()) return false;
        if (!shader.load(asset_dir + "shaders/basic.vert", asset_dir + "shaders/basic.frag")) return false;
        if (!board_tex.load(asset_dir + "textures/chess_board.png")) return false;
        if (!pieces_tex.load(asset_dir + "textures/pieces.png")) return false;
        quad.init_quad();

        shader.use();
        shader.set_int("uTexture", 0);
        return true;
    }

    void Renderer::begin_frame(int framebuffer_w, int framebuffer_h) {
        fb_w = framebuffer_w > 0 ? framebuffer_w : 1;
        fb_h = framebuffer_h > 0 ? framebuffer_h : 1;
        gl.set_viewport(fb_w, fb_h);
        gl.clear();
        // Dear ImGui changes GL state each frame; restore what we rely on
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
    }

    void Renderer::draw_quad(const Rect& r, int mode, const Color& c, float u0, float v0, float u1, float v1) {
        shader.use();
        shader.set_vec2("uScreen", static_cast<float>(fb_w), static_cast<float>(fb_h));
        shader.set_vec4("uRect", r.x, r.y, r.w, r.h);
        shader.set_vec4("uUV", u0, v0, u1, v1);
        shader.set_vec4("uColor", c.r, c.g, c.b, c.a);
        shader.set_int("uMode", mode);
        quad.draw();
    }

    void Renderer::fill_rect(const Rect& r, const Color& c) { draw_quad(r, SOLID, c); }
    void Renderer::fill_disc(const Rect& r, const Color& c) { draw_quad(r, DISC, c); }
    void Renderer::draw_ring(const Rect& r, const Color& c) { draw_quad(r, RING, c); }

    void Renderer::draw_board(const Rect& r) {
        board_tex.bind(0);
        draw_quad(r, TEXTURE, {1, 1, 1, 1});
    }

    void Renderer::draw_piece(int piece_type, int color_type, const Rect& square, float alpha) {
        if (piece_type < 0 || piece_type > 5 || color_type < 0 || color_type > 1) return;
        const Sprite& s = SPRITES[color_type][piece_type];
        float sx0 = static_cast<float>(s.x0 - SPRITE_PAD), sy0 = static_cast<float>(s.y0 - SPRITE_PAD);
        float sx1 = static_cast<float>(s.x1 + SPRITE_PAD), sy1 = static_cast<float>(s.y1 + SPRITE_PAD);

        // Preserve the sprite's aspect ratio; centre it in the square
        float h = square.h * PIECE_HEIGHT[piece_type];
        float w = h * (sx1 - sx0) / (sy1 - sy0);
        Rect dst{square.x + (square.w - w) * 0.5f, square.y + (square.h - h) * 0.5f, w, h};

        pieces_tex.bind(0);
        draw_quad(dst, KEYED_TEXTURE, {1, 1, 1, alpha},
                  sx0 / ATLAS_SIZE, sy0 / ATLAS_SIZE, sx1 / ATLAS_SIZE, sy1 / ATLAS_SIZE);
    }

    void Renderer::end_frame(platform::SDLContext& sdl) {
        sdl.swap_buffers();
    }
}
