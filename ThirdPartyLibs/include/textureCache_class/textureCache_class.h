#ifndef TEXTURECACHE_CLASS_H_INCLUDED
#define TEXTURECACHE_CLASS_H_INCLUDED

#include "texture_class/texture_class.h"
#include <unordered_map>
#include <string>

class TextureCache
{
public:
    TextureCache() = default;

    // Get or load texture - returns reference to cached instance
    // Key on path only - params ignored if already cached
    // (If you need different params for same path → use different cache or key on params too)
    const Texture& get(const std::string& path,
                       GLenum wrapS = GL_REPEAT, GLenum wrapT = GL_REPEAT,
                       GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR,
                       bool flipVertically = true)
    {
        // If already cached, return existing Texture
        auto it = cache_.find(path);
        if (it != cache_.end()) { return it->second; }
        // If not cached -> create, cache and return the new Texture
        auto [inserted_it, success] = cache_.emplace
            (path, Texture(path, wrapS, wrapT, minFilter, magFilter, flipVertically));
        return inserted_it->second;
    }

    void clear() { cache_.clear(); }                                // clear all textures
    void remove(const std::string& path) { cache_.erase(path); }    // clear specific texture
    size_t size() const { return cache_.size(); }

private:
    std::unordered_map<std::string, Texture> cache_;
};

#endif // TEXTURECACHE_CLASS_H_INCLUDED
