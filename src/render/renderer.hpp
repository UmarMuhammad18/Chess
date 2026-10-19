#pragma once
#include "gl_context.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "../platform/sdl_context.hpp"
#include <string>

namespace render {
    struct Rect  { float x, y, w, h; };   // pixels, origin top-left
    struct Color { float r, g, b, a; };

    class Renderer {
    public:
        // asset_dir must end with a path separator (e.g. ".../assets/")
        bool init(const std::string& asset_dir);
        void begin_frame(int framebuffer_w, int framebuffer_h);
        void end_frame(platform::SDLContext& sdl);

        void fill_rect(const Rect& r, const Color& c);
        void fill_disc(const Rect& r, const Color& c);
        void draw_ring(const Rect& r, const Color& c);
        void draw_board(const Rect& r);
        // Draws a piece sprite centred in the square-sized rect (piece/color use engine enum order)
        void draw_piece(int piece_type, int color_type, const Rect& square, float alpha = 1.0f);
    private:
        void draw_quad(const Rect& r, int mode, const Color& c, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1);

        GLContext gl;
        Shader shader;
        Texture board_tex;
        Texture pieces_tex;
        Mesh quad;
        int fb_w{1}, fb_h{1};
    };
}
