#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

// constant uniforms
uniform vec4 ka = vec4(0.1, 0.1, 0.1, 0.9);
uniform vec4 kd = vec4(0.5, 0.5, 0.5, 0.9);
uniform vec4 ks = vec4(0.5, 0.5, 0.5, 0.9);
uniform float p = 100;
uniform float steps = 5;
uniform float steps_spec = 50;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

// extra credit explanation
// The cel shader uses the phong model but discretely quantizes the
// output value into some number of steps. This is done for both the diffuse
// coefficent and the secular coefficent by taking the original calculated value
// and multiplying by the steps, ceiling the value and dividing my the steps again.
// This effectively makes it so there only exist steps number of possible gradient values
// which results in distinct regions of brightness.

void main() {
  // light vector
  vec3 light_dir = u_light_pos - vec3(v_position);
  float dist = dot(light_dir, light_dir);

  // view vector
  vec3 cam_dir = u_cam_pos - vec3(v_position);
  
  // norm vectors
  light_dir = normalize(light_dir);
  cam_dir = normalize(cam_dir);
  vec3 norm_dir = vec3(normalize(v_normal));
 
  // bisector
  vec3 bis_dir = normalize(light_dir + cam_dir);

  float diff = max(dot(light_dir, norm_dir), 0.0);
  float diff_cel = max(ceil(diff * steps) / steps, 0.0);
  float spec = max(dot(bis_dir, norm_dir), 0.0);
  float spec_cel = max(ceil(spec * steps_spec) / steps_spec, 0.0);
  
  vec4 light_id = vec4(u_light_intensity, 1.0) / dist;
  
  vec4 light_r = ka;
  light_r += (kd * light_id) * diff_cel;
  light_r += (ks * light_id) * pow(spec_cel, p);
  
  out_color = light_r * u_color; 
  out_color.a = 0.9;  
}