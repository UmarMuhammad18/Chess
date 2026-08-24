#pragma once
#include "gl_context.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "../engine/types.hpp"
#include "../platform/sdl_context.hpp"

namespace render {
    class Renderer {
    public:
        bool init();
        void begin_frame(int win_w, int win_h);
        void set_flipped(bool f) { flipped = f; }
        void set_board_size(int px) { board_px = px; }
        int  get_board_size() const { return board_px; }

        void draw_board();
        void draw_square_tint(int file, int rank, float r, float g, float b, float a);
        void draw_dot(int file, int rank, float r, float g, float b);
        void draw_piece(int piece_type, int color_type, int file, int rank);

        void draw_rect(int x, int y, int w, int h, float r, float g, float b, float a);
        void draw_text(int x, int y, const char* text, int scale, float r, float g, float b);
        int  text_width(const char* text, int scale) const;

        engine::Square pixel_to_square(int mx, int my) const;
        void square_to_pixel(int file, int rank, int& x, int& y) const;

        void end_frame(platform::SDLContext& sdl);
    private:
        void draw_quad(int x, int y, int w, int h, float u, float v, float uw, float uh,
                       float r, float g, float b, float a, bool use_tex);

        GLContext gl;
        Shader basic_shader;
        Texture board_tex;
        Texture pieces_tex;
        unsigned int white_tex{0};
        Mesh quad;
        int win_w{1120};
        int win_h{800};
        int board_px{800};
        bool flipped{false};
    };
}
