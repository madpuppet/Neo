// standard perspective shader

UBO View(0, 0) : V
UBO Material(1, 0) : F
SAMPLER Albedo(1, 1) : F
UBOD Model(2, 0) : V

VS_IN vec3 inPosition
VS_IN vec2 inTexCoord
VS_IN vec4 inColor : R8G8B8A8_UNORM

VS_TO_FS vec4 fragColor
VS_TO_FS vec2 fragTexCoord

PS_OUT vec4 outColor

COMMON_CODE ======================================


VERTEX_SHADER_CODE ============================== =

void main()
{
    gl_Position = View.ortho * Model.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

FRAGMENT_SHADER_CODE ============================ =

void main() {
    outColor = texture(Albedo, fragTexCoord) * fragColor * Material.blend;
    if (outColor.a == 0.0)
        discard;
}

