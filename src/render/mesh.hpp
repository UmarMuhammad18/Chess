#pragma once

namespace render {
    class Mesh {
    public:
        Mesh();
        ~Mesh();
        void init_quad();
        void draw();
    private:
        unsigned int VAO{0}, VBO{0}, EBO{0};
    };
}
