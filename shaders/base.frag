#version 460

uniform vec4 u_light_pos;

in vec4 v_normal;
in vec4 v_position;

// constant uniforms
uniform float ka = 0.4;
uniform float kd = 10;
uniform vec4 ks = vec4(0.5, 0.5, 0.5, 1);

uniform vec4 u_light_intensity_const = vec4(1, 1, 1, 0);

uniform float p = 100;
uniform float steps = 3;
uniform float steps_spec = 50;

out vec4 out_color;

void main()
{
    // light vector
    vec4 light_dir = u_light_pos - v_position;
    float dist = dot(light_dir, light_dir);

    // norm vectors
    light_dir = normalize(light_dir);
    vec4 norm_dir = v_normal;
    
    float diff = max(dot(light_dir, norm_dir), 0.0);
    float diff_cel = max(ceil(diff * steps) / steps, 0.0);
    
    vec4 light_id = u_light_intensity_const / dist;
    
    vec4 light_r = ka * u_light_intensity_const;
    light_r += (kd * light_id) * diff_cel;
    
    out_color = light_r; 
    out_color.a = 1;

    //out_color = norm_dir;  
} 