#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D debugText;

void main() {
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(texture(debugText, TexCoords).rgb, 1.0); // orthographic 
}