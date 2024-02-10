UBO 0, 0 View : VS,PS
{
    mat4 view;
    mat4 proj;
    mat4 shadow;
}

UBO 1, 0 Material : PS
{
    vec4 blend;
}

UBO 2, 0 Model : VS
{
    mat4 model;
}

SAMPLER 1, 1, Albedo

VS_IN  0, vec3, inPosition
VS_IN  1, vec2, inTexCoord
VS_IN  2, vec4, inColor

PS_IN  0, vec4, fragColor
PS_IN  1, vec2, fragTexCoord

PS_OUT 0,outColor

## VERTEX SHADER ##

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

## PIXEL SHADER ##

void main() {
    outColor = texture(texSampler, fragTexCoord) * fragColor * ubo.blend;
}

