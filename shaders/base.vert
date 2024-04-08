in vec4 in_position;
in vec4 in_normal;
in vec4 in_color;

out vec4 v_position;
out vec4 v_normal;
out vec4 v_color;

void main()
{
   mat4 project;
   project[0] = vec4(cos(0.3926991f)/sin(0.3926991f), 0.0f, 0.0f, 0.0f);
   project[1] = vec4(0.0f, cos(0.3926991f)/sin(0.3926991f), 0.0f, 0.0f);
   project[2] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
   project[3] = vec4(0.0f, 0.0f, -1.0f, 0.0f);
   v_position = in_position;
   v_normal = normalize(in_normal);
   v_color = in_color;
   gl_Position = project * in_position;    
}