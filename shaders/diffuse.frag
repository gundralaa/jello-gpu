
layout(location = 0) uniform float u_light_pos[4];

in vec4 v_normal;
in vec4 v_position;
in vec4 v_color;

// constant uniforms
uniform float ka = 0.4;
uniform float kd = 10;
uniform vec4 ks = vec4(0.5, 0.5, 0.5, 1);

out vec4 out_color;

void main()
{
    vec4 light_dir = vec4(u_light_pos[0], u_light_pos[1], u_light_pos[2], u_light_pos[3]) - v_position;
    float dist = dot(light_dir, light_dir);
    
    float diff = max(dot(light_dir, v_normal), 0) * inversesqrt(dist);

    vec4 light_r = v_color * (ka + kd * diff / dist);
    
    out_color = light_r;
    out_color.a = 1;
} 