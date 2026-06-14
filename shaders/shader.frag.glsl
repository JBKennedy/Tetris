// shader.frag.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D detailTexture;    // grayscale (1-channel)
uniform vec3  tintColor;            // RGB color for this block
uniform float opacity;              // 0.0 = fully transparent, 1.0 = fully opaque
uniform bool  useAlpha;             // true = sample alpha from texture, false = opaque
uniform bool  useSpriteSheet;       // true = sample RGBA directly, false = luminance * tint
uniform vec2  uvOffset;             // where the cell starts (e.g. 0.25, 0.0)
uniform vec2  uvScale;              // size of the cell (e.g. 0.25, 1.0)

void main()
{
    vec2 uv = uvOffset + TexCoord * uvScale;

    float alpha = useAlpha ? texture(detailTexture, uv).a : 1.0f;

    vec3 color = useSpriteSheet ? tintColor * texture(detailTexture, uv).rgb
                                : tintColor * texture(detailTexture, uv).r;

    FragColor = vec4(color, alpha * opacity);
}

