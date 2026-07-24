#pragma once
#include <string>

namespace render {
    class Shader {
    public:
        bool load(const std::string& vert_path, const std::string& frag_path);
        void use();
    private:
        public: unsigned int program_id{0};
        bool compile_shader(unsigned int type, const std::string& source, unsigned int& id);
    };
}
