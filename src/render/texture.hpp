#pragma once
#include <string>

namespace render {
    class Texture {
    public:
        Texture() = default;
        ~Texture();
        bool load(const std::string& path);
        void bind(unsigned int slot = 0);
    private:
        unsigned int texture_id{0};
    };
}
