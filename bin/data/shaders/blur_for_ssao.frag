#version 330 core

uniform sampler2D ssao;

in vec2 TexCoord;

out vec4 color;

void main()
{
    vec4 c = texture(ssao, TexCoord);
    
    vec2 texelSize = 1.0 / vec2(textureSize(ssao, 0));
    float result = 0.0;
    
    for(int x = -2; x < 2; x++)
    {
        for(int y = -2; y < 2; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssao, TexCoord + offset).r;
        }
    }
    float blur = result / (4.0 * 4.0);
    
    color = vec4(blur, blur, blur, 1.0);
}
