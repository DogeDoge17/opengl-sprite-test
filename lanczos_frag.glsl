#version 120 // OpenGL 2.1 compatible version

// Uniforms
uniform sampler2D u_Texture;  // Input texture
uniform vec2 u_TextureSize;  // Size of the texture
uniform float u_ScaleFactor; // Scale factor for resizing (e.g., 2.0, 0.5)

// Lanczos kernel function
float lanczos(float x, int a) {
    if (x == 0.0) return 1.0; // Proper normalization
    if (abs(x) > float(a)) return 0.0; // Outside the kernel radius
    float pix = 3.14159265359 * x;
    return sin(pix) * sin(pix / float(a)) / (pix * pix / float(a));
}

// Main shader function
void main() {
    vec2 coord = gl_TexCoord[0].xy; // Input texture coordinate
    vec2 texCoord = coord * u_TextureSize / u_ScaleFactor;

    vec2 texelSize = 1.0 / u_TextureSize; // Size of one texel

    // The Lanczos kernel radius ("a"): typically 2 or 3 for good results
    int a = 3;

    vec4 color = vec4(0.0);
    float totalWeight = 0.0;

    // Iterate in a window for the Lanczos kernel
    for (int i = -a; i <= a; ++i) {
        for (int j = -a; j <= a; ++j) {
            vec2 offset = vec2(float(i), float(j)) * texelSize;
            vec2 sampleCoord = texCoord + offset;

            // Sample the texture
            vec4 samplez = texture2D(u_Texture, sampleCoord);

            // Calculate the Lanczos weight for this sample
            float weight = lanczos(float(i), a) * lanczos(float(j), a);

            color += samplez * weight; // Accumulate the weighted color
            totalWeight += weight;    // Accumulate the weights
        }
    }

    // Normalize the color by the total weight to avoid darkening/brightening
    color /= totalWeight;

    // Output the final scaled color
    gl_FragColor = color;
}