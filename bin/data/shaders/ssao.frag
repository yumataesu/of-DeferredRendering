#version 330 core

struct Sample
{
    vec3 offset;
};

const int kernelSize = 64;
const float radius = 0.5;
const vec2 noiseScale = vec2(1280.0f / 4.0f, 720.0f / 4.0f);

uniform sampler2D gNormal;
uniform sampler2D gPosition;
uniform sampler2D gDepth;
uniform sampler2D noiseTexture;
uniform mat4 projection;
uniform Sample samples[kernelSize];

in vec2 TexCoord;

out vec4 color;

void main()
{
    vec4 position = texture(gPosition, TexCoord);
    vec3 normal = texture(gNormal, TexCoord).xyz;
    vec3 randomVec = texture(noiseTexture, TexCoord * noiseScale).xyz;
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    
    mat3 TBN = mat3(tangent, bitangent, normal);
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        vec3 sample = TBN * samples[i].offset; // From tangent to view-space
        sample = position.xyz + sample * radius;
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(sample, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = -texture(gDepth, offset.xy).r; // Get depth value of kernel sample
        
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= sample.z ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    color = vec4(occlusion, occlusion, occlusion, 1.0);
}
