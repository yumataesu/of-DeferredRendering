#version 330 core

in vec3 position;
in vec2 texcoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(position, 1.0f);
    TexCoord = texcoord;
}
