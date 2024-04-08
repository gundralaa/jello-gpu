
layout(location = 0) uniform float u_light_pos[4];

in vec4 v_normal;
in vec4 v_position;
in vec4 v_color;

// constant uniforms
uniform float ka = 0.4;
uniform float kd = 10;
uniform vec4 ks = vec4(0.5, 0.5, 0.5, 1);

uniform float steps = 15;

out vec4 out_color;

void main()
{
  vec3 path_in = vec3(u_light_pos[0], u_light_pos[1], u_light_pos[2])-v_position.xyz;
  vec3 path_out = -v_position.xyz;
  float dist = dot(path_in, path_in);
  vec3 mid = (normalize(path_out)+normalize(path_in));

  float diff =(5.0 * max(0, dot(v_normal.xyz, normalize(path_in))) + 10.0 * pow(max(0, dot(v_normal.xyz, normalize(mid))), 50))/dist;
  float diff_cel = ceil(sqrt(diff) * steps) / steps;

  diff_cel *= diff_cel;

  out_color = v_color * (0.4 + diff_cel);
  out_color.a = 1;
}