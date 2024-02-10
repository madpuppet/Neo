#version 450

layout(set = 0, binding = 0) uniform UBO_View
{
    mat4 view;
    mat4 proj;
} view;

layout(set = 1, binding = 0) uniform UBO_Material
{
    vec4 blend;
} material;

layout(set = 2, binding = 0) uniform UBO_Model
{
    mat4 model;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() 
{
    gl_Position = view.proj * view.view * model.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
