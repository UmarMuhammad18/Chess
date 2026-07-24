#pragma once
#include <GL/glew.h>

namespace render {
    class GLContext {
    public:
        bool init();
        void set_viewport(int width, int height);
        void clear();
    };
}
