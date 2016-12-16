#version 330 core

struct Light
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
const int LightNUM = 3;

uniform Light light[LightNUM];
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;
uniform vec3 viewPos;

uniform float constant;
uniform float linear;
uniform float quadratic;

uniform int DebugMode;


in vec2 TexCoord;

out vec4 color;

void main()
{
    vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
    vec3 albedo = texture(gAlbedo, TexCoord).rgb;
    
    float AmbientOcculusion = texture(ssao, TexCoord).r * 1.5;
    
    vec3 lighting  = vec3(0.0, 0.0, 0.0);
    
    for(int i = 0; i < LightNUM; i++)
    {
        float distance = length(light[i].position - position);
        if(distance < 900)
        {
            //Ambient
            vec3 ambient = albedo * AmbientOcculusion;
            
            //Diffuse
            vec3 lightDir = normalize(light[i].position - position);
            vec3 diffuse = max(dot(normal, lightDir), 0.0) * light[i].diffuse;
            
            //Specular
            vec3 viewDir  = normalize(viewPos - position);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
            vec3 specular = light[i].specular * spec;
            
            //Attenuation
            float dist = length(light[i].position - position) * 1.0;
            float attenuation = 1.0 + (0.1 * dist) + (0.01 * dist * dist);
            
            ambient /= attenuation;
            diffuse /= attenuation;
            specular /= attenuation;
            lighting += ambient + diffuse + specular;
        }
    }
    
    
    if(DebugMode == 0)
    {
        color = vec4(lighting, 1.0);
    }
    
    else if(DebugMode == 1)
    {
        color = vec4(position, 1.0);
    }
    
    else if(DebugMode == 2)
    {
        color = vec4(normal, 1.0);
    }
    
    else if(DebugMode == 3)
    {
        color = vec4(albedo, 1.0);
    }
    
    else if(DebugMode == 4)
    {
        color = vec4(AmbientOcculusion, AmbientOcculusion, AmbientOcculusion, 1.0);
    }
    
}
