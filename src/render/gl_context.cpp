#include "gl_context.hpp"
#include <iostream>

namespace render {
    bool GLContext::init() {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        return true;
    }
    
    void GLContext::set_viewport(int width, int height) {
        glViewport(0, 0, width, height);
    }
    
    void GLContext::clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}
