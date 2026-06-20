#ifndef SHADER_CLASS_H_INCLUDED
#define SHADER_CLASS_H_INCLUDED

#include <glad/glad.h>  // include glad to get all the required OpenGL headers

#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include <glm/glm.hpp>                  // core types: vec3, mat4, etc.
#include <glm/gtc/matrix_transform.hpp> // for translate / rotate / scale / perspective / lookAt
#include <glm/gtc/type_ptr.hpp>         // value_ptr() for passing matrices to uniforms

class Shader
{
public:
    // the program ID
    unsigned int ID{ 0 };

    // default constructor (reads + builds shader program, or throws on error)
    explicit Shader(const std::string& vertexPath, const std::string& fragmentPath)
    {
        // 1. retrieve shader source code from filePath
        std::string vertexCode{};
        std::string fragmentCode{};

        try
        {
            vertexCode = readFile(vertexPath);
            fragmentCode = readFile(fragmentPath);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl;
            throw;
        }

        // 2. build and compile shaders
        // ----------------------------
        unsigned int vertexShader{ compileShader(vertexCode.c_str(), GL_VERTEX_SHADER, "VERTEX") };
        unsigned int fragmentShader{ compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER, "FRAGMENT") };
        // link program
        ID = linkProgram(vertexShader, fragmentShader);

        // delete shader objects (no longer needed after linking)
        glDeleteShader( vertexShader );
        glDeleteShader( fragmentShader );
    }

    ~Shader() { glDeleteProgram(ID); }

    Shader(const Shader&) = delete;             // prohibit use of copy constructor
    Shader& operator=(const Shader&) = delete;  // prohibit use of copy assignment
    // move constructor
    Shader(Shader&& other) noexcept
        : ID(other.ID), uniformLocationCache(std::move(other.uniformLocationCache))
    {
        other.ID = 0;
    }
    // move assignment operator
    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            glDeleteProgram(ID);
            ID = other.ID;
            uniformLocationCache = std::move(other.uniformLocationCache);
            other.ID = 0;
        }
        return *this;
    }

    // activate the shader
    void use() const { glUseProgram(ID); }

    // for validation
    bool isValid() const noexcept { return ID != 0; }

    // utility uniform functions (cached locations for performance)
    // -------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(getUniformLocation(name), static_cast<int>(value));
    }
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(getUniformLocation(name), value);
    }
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(getUniformLocation(name), value);
    }
    // -------------------------------------------------------------------
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(getUniformLocation(name), x, y);
    }
    // -------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(getUniformLocation(name), x, y, z);
    }
    // -------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }
    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(getUniformLocation(name), x, y, z, w);
    }
    // -------------------------------------------------------------------
    void setMat2(const std::string& name, const glm::mat2& matrix, bool transpose = false) const
    {
        glUniformMatrix2fv(
                           getUniformLocation(name),      // reuse existing cached lookup
                           1,                             // count: one matrix
                           transpose ? GL_TRUE : GL_FALSE,// usually GL::FALSE (GLM is column-major)
                           glm::value_ptr(matrix)         // pointer to the 4 floats
                           );
    }
    // -------------------------------------------------------------------
    void setMat3(const std::string& name, const glm::mat3& matrix, bool transpose = false) const
    {
        glUniformMatrix3fv(
                           getUniformLocation(name),      // reuse existing cached lookup
                           1,                             // count: one matrix
                           transpose ? GL_TRUE : GL_FALSE,// usually GL::FALSE (GLM is column-major)
                           glm::value_ptr(matrix)         // pointer to the 9 floats
                           );
    }
    // -------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4& matrix, bool transpose = false) const
    {
        glUniformMatrix4fv(
                           getUniformLocation(name),      // reuse existing cached lookup
                           1,                             // count: one matrix
                           transpose ? GL_TRUE : GL_FALSE,// usually GL::FALSE (GLM is column-major)
                           glm::value_ptr(matrix)         // pointer to the 16 floats
                           );
    }

private:

    static std::string readFile( const std::string& path )
    {
        std::ifstream file(path, std::ios::in);
        if (!file.is_open())
        {
            throw std::runtime_error( "ERROR::SHADER::FILE_NOT_FOUND: " + path );
        }

        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        std::stringstream stream;
        stream << file.rdbuf();
        return stream.str();
    }

    static unsigned int compileShader( const char* source, GLenum type, std::string_view typeName )
    {
        // create VERTEX SHADER object and assign reference ID
        unsigned int shader{ glCreateShader( type ) };
        if (shader == 0) throw std::runtime_error("Failed to create " + std::string(typeName) + " shader object");
        // attach source code and compile
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        // log and throw exception if compilation fails
        int success{};
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[1024]{};
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::" << typeName << "::COMPILATION_FAILED\n"
                      << infoLog << "\nSource:\n" << source << std::endl;
            glDeleteShader(shader);
            throw std::runtime_error( "Shader compilation failed: " + std::string(typeName) );
        }

        return shader;
    }

    static unsigned int linkProgram( unsigned int vertex, unsigned int fragment )
    {
        // create SHADER PROGRAM object
        auto program{ glCreateProgram() };
        if (program == 0) throw std::runtime_error("Failed to create program object");
        // attach shaders and link
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        // log and throw exception if linking failed
        int success{};
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[1024]{};
            glGetProgramInfoLog(program, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            glDeleteProgram(program);
            throw std::runtime_error("Shader program linking failed");
        }

        return program;
    }

    mutable std::unordered_map<std::string, int> uniformLocationCache;

    int getUniformLocation(const std::string& name) const
    {
        // return the cached uniform location if has been looked up before
        if (auto it{ uniformLocationCache.find(name) }; it != uniformLocationCache.end())
            return it->second;

        int location{ glGetUniformLocation(ID, name.c_str()) };
        if (location == -1)
            std::cerr << "Warning: Uniform " << name << " not found or not used." << std::endl;

        uniformLocationCache[name] = location;
        return location;
    }
};

#endif // SHADER_CLASS_H_INCLUDED
