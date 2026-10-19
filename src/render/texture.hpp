#pragma once
#include <string>

namespace render {
    class Texture {
    public:
        Texture() = default;
        ~Texture();
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        bool load(const std::string& path);
        void bind(unsigned int slot = 0);
        int width() const { return w; }
        int height() const { return h; }
    private:
        unsigned int texture_id{0};
        int w{0}, h{0};
    };
}
