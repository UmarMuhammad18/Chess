#include "renderer.hpp"
#include <GL/glew.h>
#include <cstring>

namespace render {
    // 5x7 glyphs for ASCII 32-126. Each glyph is 7 rows, bits 4..0 = left..right.
    static const unsigned char FONT[95][7] = {
        {0,0,0,0,0,0,0}, // space
        {4,4,4,4,0,4,0}, // !
        {10,10,0,0,0,0,0},
        {10,10,31,10,31,10,10},
        {4,15,20,14,5,30,4},
        {18,18,2,4,8,9,9},
        {8,20,20,8,21,18,13},
        {4,4,0,0,0,0,0},
        {2,4,8,8,8,4,2},
        {8,4,2,2,2,4,8},
        {0,4,21,14,21,4,0},
        {0,4,4,31,4,4,0},
        {0,0,0,0,0,4,8},
        {0,0,0,31,0,0,0},
        {0,0,0,0,0,4,0},
        {1,1,2,4,8,16,16},
        {14,17,19,21,25,17,14}, // 0
        {4,12,4,4,4,4,14},
        {14,17,1,6,8,16,31},
        {14,17,1,6,1,17,14},
        {2,6,10,18,31,2,2},
        {31,16,30,1,1,17,14},
        {6,8,16,30,17,17,14},
        {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14},
        {14,17,17,15,1,2,12},
        {0,4,0,0,0,4,0},
        {0,4,0,0,0,4,8},
        {2,4,8,16,8,4,2},
        {0,0,31,0,31,0,0},
        {8,4,2,1,2,4,8},
        {14,17,1,2,4,0,4},
        {14,17,21,23,22,16,14},
        {14,17,17,31,17,17,17}, // A
        {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},
        {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},
        {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14},
        {1,1,1,1,1,17,14},
        {17,18,20,24,20,18,17},
        {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},
        {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13},
        {30,17,17,30,20,18,17},
        {14,17,16,14,1,17,14},
        {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10},
        {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},
        {31,1,2,4,8,16,31},
        {14,8,8,8,8,8,14},
        {16,16,8,4,2,1,1},
        {14,2,2,2,2,2,14},
        {4,10,17,0,0,0,0},
        {0,0,0,0,0,0,31},
        {8,4,0,0,0,0,0},
        {0,0,14,1,15,17,15}, // a
        {16,16,30,17,17,17,30},
        {0,0,14,17,16,17,14},
        {1,1,15,17,17,17,15},
        {0,0,14,17,31,16,14},
        {6,8,8,30,8,8,8},
        {0,0,15,17,15,1,14},
        {16,16,30,17,17,17,17},
        {4,0,12,4,4,4,14},
        {2,0,2,2,2,18,12},
        {16,16,18,20,24,20,18},
        {12,4,4,4,4,4,14},
        {0,0,26,21,21,21,21},
        {0,0,30,17,17,17,17},
        {0,0,14,17,17,17,14},
        {0,0,30,17,30,16,16},
        {0,0,15,17,15,1,1},
        {0,0,22,24,16,16,16},
        {0,0,15,16,14,1,30},
        {8,8,30,8,8,8,6},
        {0,0,17,17,17,17,15},
        {0,0,17,17,17,10,4},
        {0,0,17,17,21,21,10},
        {0,0,17,10,4,10,17},
        {0,0,17,17,15,1,14},
        {0,0,31,2,4,8,31},
        {2,4,4,8,4,4,2},
        {4,4,4,4,4,4,4},
        {8,4,4,2,4,4,8},
        {0,8,21,2,0,0,0},
    };

    bool Renderer::init() {
        if (!gl.init()) return false;
        if (!basic_shader.load("assets/shaders/basic.vert", "assets/shaders/basic.frag")) return false;
        board_tex.load("assets/textures/chess_board.png");
        pieces_tex.load("assets/textures/pieces.png");

        glGenTextures(1, &white_tex);
        glBindTexture(GL_TEXTURE_2D, white_tex);
        unsigned char px[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        quad.init_quad();
        return true;
    }

    void Renderer::begin_frame(int w, int h) {
        win_w = w;
        win_h = h;
        glViewport(0, 0, w, h);
        gl.clear();
    }

    void Renderer::draw_quad(int x, int y, int w, int h, float u, float v, float uw, float uh,
                             float r, float g, float b, float a, bool use_tex) {
        float gl_y = static_cast<float>(win_h - y - h);
        float ndc_x = (static_cast<float>(x) / win_w) * 2.0f - 1.0f;
        float ndc_y = (gl_y / win_h) * 2.0f - 1.0f;
        float ndc_w = (static_cast<float>(w) / win_w) * 2.0f;
        float ndc_h = (static_cast<float>(h) / win_h) * 2.0f;
        basic_shader.use();
        basic_shader.set_vec4("uNdc", ndc_x, ndc_y, ndc_w, ndc_h);
        basic_shader.set_vec4("uvOffset", u, v, uw, uh);
        basic_shader.set_vec4("uTint", r, g, b, a);
        basic_shader.set_float("uUseTex", use_tex ? 1.0f : 0.0f);
        quad.draw();
    }

    void Renderer::square_to_pixel(int file, int rank, int& x, int& y) const {
        int df = flipped ? 7 - file : file;
        int dr = flipped ? 7 - rank : rank;
        int sq = board_px / 8;
        x = df * sq;
        y = (7 - dr) * sq;
    }

    engine::Square Renderer::pixel_to_square(int mx, int my) const {
        if (mx < 0 || my < 0 || mx >= board_px || my >= board_px) return engine::Square::NONE;
        int sq = board_px / 8;
        int file = mx / sq;
        int rank = 7 - (my / sq);
        if (flipped) {
            file = 7 - file;
            rank = 7 - rank;
        }
        if (file < 0 || file > 7 || rank < 0 || rank > 7) return engine::Square::NONE;
        return static_cast<engine::Square>(rank * 8 + file);
    }

    void Renderer::draw_board() {
        glBindTexture(GL_TEXTURE_2D, board_tex.id());
        draw_quad(0, 0, board_px, board_px, 0, 0, 1, 1, 1, 1, 1, 1, true);
    }

    void Renderer::draw_square_tint(int file, int rank, float r, float g, float b, float a) {
        int x, y;
        square_to_pixel(file, rank, x, y);
        int sq = board_px / 8;
        glBindTexture(GL_TEXTURE_2D, white_tex);
        draw_quad(x, y, sq, sq, 0, 0, 1, 1, r, g, b, a, false);
    }

    void Renderer::draw_dot(int file, int rank, float r, float g, float b) {
        int x, y;
        square_to_pixel(file, rank, x, y);
        int sq = board_px / 8;
        int d = sq / 4;
        glBindTexture(GL_TEXTURE_2D, white_tex);
        draw_quad(x + (sq - d) / 2, y + (sq - d) / 2, d, d, 0, 0, 1, 1, r, g, b, 0.9f, false);
    }

    void Renderer::draw_piece(int piece_type, int color_type, int file, int rank) {
        int cols[] = {5, 3, 2, 4, 1, 0};
        int col = cols[piece_type];
        int row = (color_type == 0) ? 0 : 1;
        float uw = 1.0f / 6.0f;
        float uh = 1.0f / 2.0f;
        int x, y;
        square_to_pixel(file, rank, x, y);
        int sq = board_px / 8;
        glBindTexture(GL_TEXTURE_2D, pieces_tex.id());
        draw_quad(x, y, sq, sq, col * uw, row * uh, uw, uh, 1, 1, 1, 1, true);
    }

    void Renderer::draw_rect(int x, int y, int w, int h, float r, float g, float b, float a) {
        glBindTexture(GL_TEXTURE_2D, white_tex);
        draw_quad(x, y, w, h, 0, 0, 1, 1, r, g, b, a, false);
    }

    void Renderer::draw_text(int x, int y, const char* text, int scale, float r, float g, float b) {
        glBindTexture(GL_TEXTURE_2D, white_tex);
        int cx = x;
        for (const char* p = text; *p; ++p) {
            unsigned char ch = static_cast<unsigned char>(*p);
            if (ch == '\n') { y += 8 * scale + 2; cx = x; continue; }
            if (ch < 32 || ch > 126) { cx += 6 * scale; continue; }
            const unsigned char* glyph = FONT[ch - 32];
            for (int row = 0; row < 7; row++) {
                for (int col = 0; col < 5; col++) {
                    if (glyph[row] & (16 >> col)) {
                        draw_quad(cx + col * scale, y + row * scale, scale, scale,
                                  0, 0, 1, 1, r, g, b, 1, false);
                    }
                }
            }
            cx += 6 * scale;
        }
    }

    int Renderer::text_width(const char* text, int scale) const {
        int w = 0, line = 0;
        for (const char* p = text; *p; ++p) {
            if (*p == '\n') { if (line > w) w = line; line = 0; continue; }
            line += 6 * scale;
        }
        return line > w ? line : w;
    }

    void Renderer::end_frame(platform::SDLContext& sdl) {
        sdl.swap_buffers();
    }
}
