#pragma once
#include <string>
#include <unordered_map>

namespace render {
    class Shader {
    public:
        Shader() = default;
        ~Shader();
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        bool load(const std::string& vert_path, const std::string& frag_path);
        void use();

        void set_int(const char* name, int v);
        void set_vec2(const char* name, float x, float y);
        void set_vec4(const char* name, float x, float y, float z, float w);
    private:
        bool compile_shader(unsigned int type, const std::string& source, unsigned int& id);
        int  location(const char* name);

        unsigned int program_id{0};
        std::unordered_map<std::string, int> uniform_cache;
    };
}
