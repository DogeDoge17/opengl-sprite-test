#version 330 core

uniform sampler2D u_Texture;
in vec2 v_TexCoord;

out vec4 FragColor;

void main() {
    FragColor = texture(u_Texture, v_TexCoord);
}