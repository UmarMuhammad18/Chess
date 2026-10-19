#include "gl_context.hpp"
#include <iostream>

namespace render {
    bool GLContext::init() {
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        bool ok = err == GLEW_OK;
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
        // GLEW built for GLX reports "No GLX display" under Wayland even though GL works
        ok = ok || err == GLEW_ERROR_NO_GLX_DISPLAY;
#endif
        if (!ok) {
            std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
            return false;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glClearColor(0.13f, 0.13f, 0.15f, 1.0f);
        return true;
    }

    void GLContext::set_viewport(int width, int height) {
        glViewport(0, 0, width, height);
    }

    void GLContext::clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}
