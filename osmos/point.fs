#version 400 

in Fragment {
  vec4 color;
  vec2 mapping;
} fragment;

//Rotate a 2D vector
vec2 rotate(vec2 v, float a) {
  float s = sin(a);
  float c = cos(a);
  mat2 m = mat2(c, -s, s, c);
  return m * v;
}

float random(vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

uniform sampler2D color_tex;
uniform sampler2D color_tex_alt;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

void main() {
  float r = dot(fragment.mapping, fragment.mapping);

  if(r > 1) {
    discard;
  }

  //add rotation to uv
  bool which_text = true;
  vec3 color = vec3(0.f);

  if(which_text) {
    color = texture(color_tex, rotate(fragment.mapping, fragment.color.r)).rgb;
  } else {
    color = texture(color_tex_alt, rotate(fragment.mapping, fragment.color.r)).rgb;
  }
  vec3 result = color;

  //using per vertex color
  //result = fragment.color.rgb;

  float brightness = dot(result, vec3(0.3, 0.3, 0.3));

  if(brightness > 0.5)
    BrightColor = vec4(result * 1.0, 1.0);
  else
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

  FragColor = vec4(result, 1 - r * r);
}