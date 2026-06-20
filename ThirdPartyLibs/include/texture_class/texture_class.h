#ifndef TEXTURE_CLASS_H_INCLUDED
#define TEXTURE_CLASS_H_INCLUDED

#include <glad/glad.h>      // include glad to get all the required OpenGL headers
#include <stb/stb_image.h>  // for reading image files into memory
#include <string>
#include <iostream>

class Texture
{
public:
    // Default Constructor
    Texture(const std::string& path, GLenum wrapS = GL_REPEAT, GLenum wrapT = GL_REPEAT,
            GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR,
            bool flipVertically = true)
        : path_(path), ID_(0)
    {
        loadAndCreate(wrapS, wrapT, minFilter, magFilter, flipVertically);
    }

    ~Texture()
    {
        if (ID_ != 0)
        {
            glDeleteTextures(1, &ID_);
            #ifndef NDEBUG
            std::cout << "Texture destroyed: " << path_ << " (ID " << ID_ << ")\n";
            #endif // NDEBUG
        }
    }

    // prohibit use of Copy Constructor (OpenGL texture IDs shouldn't be copied)
    Texture(const Texture&) = delete;

    // prohibit use of Copy Assignment
    Texture& operator=(const Texture&) = delete;

    // Move Constructor (useful if moving textures around)
    Texture(Texture&& other) noexcept
        : ID_(other.ID_), path_(std::move(other.path_))
    {
        other.ID_ = 0;
    }

    // Move Assignment Operator
    Texture& operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            if (ID_ != 0) glDeleteTextures(1, &ID_);
            ID_ = other.ID_;
            path_ = std::move(other.path_);
            other.ID_ = 0;
        }
        return *this;
    }

    // Bind the texture to a specific unit
    void bind(GLenum textureUnit = GL_TEXTURE0) const
    {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, ID_);
    }

    // Get the OpenGL texture ID (if needed directly)
    GLuint getID() const { return ID_; }

    // Get original path (for debugging/logging)
    const std::string& getPath() const { return path_; }

private:
    GLuint ID_;
    std::string path_;

    void loadAndCreate(GLenum wrapS, GLenum wrapT, GLenum minFilter, GLenum magFilter, bool flip)
    {
        stbi_set_flip_vertically_on_load(flip);

        int width, height, channels;
        unsigned char* data{ stbi_load(path_.c_str(), &width, &height, &channels, 0) };
        if (!data)
        {
            #ifndef NDEBUG
            std::cerr << "Failed to load texture: " << path_ << std::endl;
            #endif // NDEBUG
            return;
        }

        GLenum sourceFormat{ GL_RGB };  // default
        GLenum internalFormat{ GL_RGB };// default

        switch(channels)
        {
        case 1:
            sourceFormat = GL_RED;      // 1-channel grayscale
            internalFormat = GL_RED;    // store as single channel to save VRAM (optional)
            break;
        case 3:
            sourceFormat = GL_RGB;
            internalFormat = GL_RGB;
            break;
        case 4:
            sourceFormat = GL_RGBA;
            internalFormat = GL_RGBA;
            break;
        default:
            #ifndef NDEBUG
            std::cerr << "Unsupported channel count " << channels << " in " << path_ << std::endl;
            #endif // NDEBUG
            stbi_image_free(data);
            return;
        }

        glGenTextures(1, &ID_);
        glBindTexture(GL_TEXTURE_2D, ID_);
        // set wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        // set filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        // upload to gpu
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, sourceFormat, GL_UNSIGNED_BYTE, data);
        // set mipmaps - enable only if needed
        // if (minFilter == GL_LINEAR_MIPMAP_LINEAR || minFilter == GL_NEAREST_MIPMAP_NEAREST)
        //     glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        #ifndef NDEBUG
        std::cout << "Loaded texture: " << path_ << " (" << width << 'x' << height << ", " << channels << " channels)\n";
        #endif // NDEBUG
    }
};

#endif // TEXTURE_CLASS_H_INCLUDED
