#include "gl_context.hpp"
#include <iostream>

namespace render {
    bool GLContext::init() {
#if defined(CHESS_ANDROID) && !defined(__ANDROID__)
        std::cerr << "GLContext: CHESS_ANDROID stub (no GLEW)\n";
        return true;
#elif defined(CHESS_ANDROID) && defined(__ANDROID__)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        return true;
#else
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        return true;
#endif
    }

    void GLContext::set_viewport(int width, int height) {
#if defined(CHESS_ANDROID) && !defined(__ANDROID__)
        (void)width; (void)height;
#else
        glViewport(0, 0, width, height);
#endif
    }

    void GLContext::clear() {
#if defined(CHESS_ANDROID) && !defined(__ANDROID__)
#else
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
    }
}
