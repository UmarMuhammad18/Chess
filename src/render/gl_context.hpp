#pragma once

#if defined(CHESS_ANDROID)
  #if defined(__ANDROID__)
    #include <GLES3/gl3.h>
  #else
    typedef unsigned int GLuint;
    typedef int GLint;
    typedef float GLfloat;
    typedef unsigned int GLenum;
    typedef unsigned char GLboolean;
    #define GL_TRUE 1
    #define GL_BLEND 0x0BE2
    #define GL_SRC_ALPHA 0x0302
    #define GL_ONE_MINUS_SRC_ALPHA 0x0303
    #define GL_COLOR_BUFFER_BIT 0x00004000
    #define GL_DEPTH_BUFFER_BIT 0x00000100
  #endif
#else
  #include <GL/glew.h>
#endif

namespace render {
    class GLContext {
    public:
        bool init();
        void set_viewport(int width, int height);
        void clear();
    };
}
