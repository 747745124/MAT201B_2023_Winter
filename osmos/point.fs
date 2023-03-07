#version 400 

in Fragment {
  vec4 color;
  vec2 mapping;
  float size;
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
uniform sampler2D color_user;

//these 3 params are used for user-related control
//also a trailing effect is going to be implemented
uniform vec2 direction;
uniform bool is_user;
uniform float time;
uniform float SPEED = 0.01;
uniform float user_size;
uniform bool is_dead;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

void main() {

  if(is_dead) {
    discard;
  }

  float r = dot(fragment.mapping, fragment.mapping);
  vec3 color = vec3(0.f);

  if(r > 1) {
    discard;
  }

  if(is_user) {
    color = texture(color_user, rotate(fragment.mapping, fragment.color.r)).rgb;
    //moving effect
    float sinEffect = sin(time * 10.0) * 0.25;
    vec2 offset_uv = normalize(direction * vec2(1.0, sinEffect)) * SPEED;
    vec3 offset_color = texture(color_user, offset_uv + rotate(fragment.mapping, fragment.color.r)).rgb;
    color = mix(color, offset_color, 0.5);

    vec3 result = color;
    float brightness = dot(result, vec3(0.3, 0.3, 0.3));

    if(brightness > 0.4)
      BrightColor = vec4(result * 1.0, 1 - r * r);
    else
      BrightColor = vec4(0.0, 0.0, 0.0, 1 - r * r);

    if(r > 0.95) {
      FragColor = vec4(result * (sinEffect + 1.0), 1 - r * r);
    }

    return;
  }

  //add rotation to uv
  bool which_text = fragment.size > abs(user_size);

  if(which_text) {
    color = texture(color_tex, rotate(fragment.mapping, fragment.color.r)).rgb;
  } else {
    color = texture(color_tex_alt, rotate(fragment.mapping, fragment.color.r)).rgb;
  }

  vec3 result = color;

  //using per vertex color
 //result = fragment.color.rgb;

  float brightness = dot(result, vec3(0.3, 0.3, 0.3));

  if(brightness > 0.4)
    BrightColor = vec4(result * 1.0, 1 - r * r);
  else
    BrightColor = vec4(0.0, 0.0, 0.0, 1 - r * r);

  FragColor = vec4(result, 1 - r * r);
}