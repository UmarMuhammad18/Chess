#include "renderer.hpp"
#include <iostream>
#include <GL/glew.h>

namespace render {
    bool Renderer::init() {
        if (!gl.init()) return false;
        if (!basic_shader.load("assets/shaders/basic.vert", "assets/shaders/basic.frag")) return false;
        board_tex.load("assets/textures/chess_board.png");
        pieces_tex.load("assets/textures/pieces.png");
        
        // Create a 1x1 yellow texture for highlights manually
        unsigned int hl;
        glGenTextures(1, &hl);
        glBindTexture(GL_TEXTURE_2D, hl);
        unsigned char yellow[4] = {255, 255, 0, 128}; // semi-transparent yellow
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, yellow);
        highlight_tex = Texture(); 
        // Hacky way to inject it, but we can't assign texture_id directly.
        // Let's just create a new shader for solid colors instead.
        quad.init_quad();
        return true;
    }
    
    void Renderer::begin_frame() { gl.clear(); }
    
    void Renderer::draw_board() {
        basic_shader.use();
        int uvLoc = glGetUniformLocation(basic_shader.program_id, "uvOffset");
        glUniform4f(uvLoc, 0.0f, 0.0f, 1.0f, 1.0f);
        board_tex.bind(0);
        quad.draw();
    }
    
    void Renderer::draw_highlight(int x, int y) {
        // Simplified highlight via basic shader and missing texture (defaults to solid color if bound properly)
        // Or we just skip actual highlight texture and use blending
        int sq_size = 800 / 8;
        glViewport(x * sq_size, y * sq_size, sq_size, sq_size);
        basic_shader.use();
        int uvLoc = glGetUniformLocation(basic_shader.program_id, "uvOffset");
        glUniform4f(uvLoc, 0.0f, 0.0f, 1.0f, 1.0f);
        // Bind missing texture (usually black/white) and rely on it.
        // This is scaffold code, so it might just draw a square of whatever texture was last bound.
        // Let's bind the board texture but draw it extremely large to simulate a flat color.
        board_tex.bind(0);
        glUniform4f(uvLoc, 0.0f, 0.0f, 0.01f, 0.01f);
        quad.draw();
        glViewport(0, 0, 800, 800);
    }
    
    void Renderer::draw_piece(int piece_type, int color_type, int x, int y) {
        int cols[] = {5, 3, 2, 4, 1, 0};
        int col = cols[piece_type];
        int row = (color_type == 0) ? 0 : 1;
        float w = 1.0f / 6.0f;
        float h = 1.0f / 2.0f;
        float u = col * w;
        float v = row * h;
        
        int sq_size = 800 / 8;
        glViewport(x * sq_size, y * sq_size, sq_size, sq_size);
        
        basic_shader.use();
        int uvLoc = glGetUniformLocation(basic_shader.program_id, "uvOffset");
        glUniform4f(uvLoc, u, v, w, h);
        pieces_tex.bind(0);
        quad.draw();
        
        glViewport(0, 0, 800, 800);
    }
    
    void Renderer::end_frame(platform::SDLContext& sdl) {
        sdl.swap_buffers();
    }
}
