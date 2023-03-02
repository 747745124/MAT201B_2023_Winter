#version 330 core
layout(location = 0) in vec3 aPosition;
out vec3 texCoord;
uniform mat4 projection;
uniform mat4 view;
void main() {
    texCoord = aPosition;
    gl_Position = (projection * view * vec4(100.f * aPosition, 1.0f)).xyww;
}
