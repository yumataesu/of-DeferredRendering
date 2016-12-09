#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec4 position;
in vec3 normal;
in vec2 texcoord;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    Position = (view * model * position).xyz;
    Normal = normalize((view * model * vec4(normal, 0.0)).xyz);
    TexCoord = texcoord;
    
    gl_Position = projection * view * model * position;
}
