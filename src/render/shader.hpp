#pragma once
#include <string>

namespace render {
    class Shader {
    public:
        bool load(const std::string& vert_path, const std::string& frag_path);
        void use();
        void set_vec4(const char* name, float x, float y, float z, float w);
        void set_float(const char* name, float v);
        unsigned int program_id{0};
    private:
        bool compile_shader(unsigned int type, const std::string& source, unsigned int& id);
    };
}
