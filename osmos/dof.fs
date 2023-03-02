#version 400
//naive implmentation of dof
out vec4 color;
in vec2 TexCoords;
uniform sampler2D postBuffer;
uniform vec2 mouseFocus = vec2(0.5, 0.5);
const int mean_step = 10;
const float minDistance = 0.0;
const float maxDistance = 0.707f;

void main() {
    vec4 fragColor = texture(postBuffer, TexCoords);
    color = fragColor;

    vec4 orgColor = vec4(color.rgb, 1.0f);
    vec4 tempColor = vec4(0.0);
    tempColor.rgb = vec3(0.0);

    for(int x = -mean_step; x <= mean_step; ++x) {
        for(int y = -mean_step; y <= mean_step; ++y) {
            vec2 offset = vec2(float(x), float(y)) * 1.0 / vec2(textureSize(postBuffer, 0));
            vec4 tmp = texture(postBuffer, TexCoords + offset);
            tempColor.rgb += tmp.rgb;
        }
    }

    vec4 mean_color = vec4(tempColor.rgb / pow((mean_step * 2 + 1), 2), 1.0);
    orgColor = mix(orgColor, mean_color, 0.8);
    float blur = smoothstep(minDistance, maxDistance, length(TexCoords - mouseFocus));
    color = mix(fragColor, orgColor, blur);
    return;
}