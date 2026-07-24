#include "sdl_context.hpp"
#include <iostream>

namespace platform {
    SDLContext::SDLContext() {}
    SDLContext::~SDLContext() {
        if (gl_context) SDL_GL_DeleteContext(gl_context);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
    bool SDLContext::init(const std::string& title, int width, int height) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        
        // Anti-aliasing
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                  width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!window) {
            std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }

        gl_context = SDL_GL_CreateContext(window);
        if (!gl_context) {
            std::cerr << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_GL_SetSwapInterval(1); // Enable vsync
        return true;
    }
    void SDLContext::swap_buffers() {
        SDL_GL_SwapWindow(window);
    }
}
