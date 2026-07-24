#pragma once
#include "gl_context.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "../platform/sdl_context.hpp"

namespace render {
    class Renderer {
    public:
        bool init();
        void begin_frame();
        void draw_board();
        void draw_highlight(int x, int y);
        void draw_piece(int piece_type, int color_type, int x, int y);
        void end_frame(platform::SDLContext& sdl);
    private:
        GLContext gl;
        Shader basic_shader;
        Texture board_tex;
        Texture pieces_tex;
        Texture highlight_tex; // Just a 1x1 color pixel
        Mesh quad;
    };
}
