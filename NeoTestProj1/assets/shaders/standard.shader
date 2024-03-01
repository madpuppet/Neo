// standard perspective shader

UBO_View View(0,0) : V
UBO_Material Material(1,0) : F
Sampler Albedo(1, 1) : F
UBO_Model Model(2,0) : V

VS_IN Vertex_p3f_t2f_c4b

VS_TO_FS vec4 fragColor
VS_TO_FS vec2 fragTexCoord

PS_OUT vec4 outColor

COMMON_CODE ======================================


VERTEX_SHADER_CODE ===============================

void main()
{
    gl_Position = View.proj * View.view * Model.model * vec4(pos, 1.0);
    fragColor = color;
    fragTexCoord = texCoord;
}

FRAGMENT_SHADER_CODE =============================

void main() {
    outColor = texture(Albedo, fragTexCoord) * fragColor * Material.blendColor;
    if (outColor.a == 0.0)
        discard;
}

