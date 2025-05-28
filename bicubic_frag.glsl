#version 330 core

uniform sampler2D u_Texture;
uniform vec2 u_TextureSize; // Size of the texture
in vec2 v_TexCoord;

out vec4 FragColor;

float cubicWeight(float x) {
    if (x < 1.0) {
        return (1.5 * x - 2.5) * x * x + 1.0;
    } else if (x < 2.0) {
        return ((-0.5 * x + 2.5) * x - 4.0) * x + 2.0;
    } else {
        return 0.0;
    }
}

vec4 textureBicubic(sampler2D tex, vec2 texCoord, vec2 texSize) {
    vec2 texelSize = 1.0 / texSize;
    vec2 coord = texCoord * texSize - 0.5;

    vec2 f = fract(coord);
    coord -= f;

    vec4 result = vec4(0.0);

    for (int j = -1; j <= 2; j++) {
        for (int i = -1; i <= 2; i++) {
            vec2 offset = vec2(float(i), float(j));
            vec2 sampleCoord = (coord + offset) * texelSize;
            vec4 samplez = texture(tex, sampleCoord);

            float wx = cubicWeight(abs(f.x - float(i)));
            float wy = cubicWeight(abs(f.y - float(j)));

            result += samplez * wx * wy;
        }
    }

    return result;
}

void main() {
    FragColor = textureBicubic(u_Texture, v_TexCoord, u_TextureSize);
}