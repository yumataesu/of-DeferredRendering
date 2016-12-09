#version 330 core

in vec3 position;
in vec2 texcoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(position, 1.0f);
    // We swap the y-axis by substracing our coordinates from 1. This is done because most images have the top y-axis inversed with OpenGL's top y-axis.
    TexCoord = texcoord;
}
