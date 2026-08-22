#include "shader.hpp"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace render {
    bool Shader::compile_shader(unsigned int type, const std::string& source, unsigned int& id) {
        id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int success;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(id, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
            return false;
        }
        return true;
    }

    bool Shader::load(const std::string& vert_path, const std::string& frag_path) {
        std::ifstream vShaderFile(vert_path);
        std::ifstream fShaderFile(frag_path);
        if (!vShaderFile.is_open() || !fShaderFile.is_open()) {
            std::cerr << "Failed to open shader files!" << std::endl;
            return false;
        }

        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        unsigned int vertex, fragment;
        if (!compile_shader(GL_VERTEX_SHADER, vShaderStream.str(), vertex)) return false;
        if (!compile_shader(GL_FRAGMENT_SHADER, fShaderStream.str(), fragment)) return false;

        program_id = glCreateProgram();
        glAttachShader(program_id, vertex);
        glAttachShader(program_id, fragment);
        glLinkProgram(program_id);

        int success;
        glGetProgramiv(program_id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program_id, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return success;
    }

    void Shader::use() {
        glUseProgram(program_id);
    }
}
