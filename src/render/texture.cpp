#include "texture.hpp"
#include <GL/glew.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/stb_image.h"

namespace render {
    Texture::~Texture() {
        if (texture_id > 0) {
            glDeleteTextures(1, &texture_id);
        }
    }
    
    bool Texture::load(const std::string& path) {
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // good for pixel art/chess

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            return true;
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
            stbi_image_free(data);
            return false;
        }
    }

    void Texture::bind(unsigned int slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture_id);
    }
}
