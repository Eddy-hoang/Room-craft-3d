#version 330 core

// Fragment shader 3D cơ bản cho M1/M2/M3:
// - Hỗ trợ cả màu cơ bản uColor và diffuse texture (uHasTexture == 1) cho M3 Asset Preview.
// - Khi uFaceContrast > 0 sẽ tạo độ tương phản hướng mặt nhẹ giúp nhìn rõ khối 3D (chưa phải Phong Lighting M7).
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec4 uColor;
uniform float uFaceContrast;
uniform sampler2D uDiffuseMap;
uniform int uHasTexture;

out vec4 FragColor;

void main()
{
    vec3 baseRgb = uColor.rgb;
    if (uHasTexture != 0) {
        vec4 texSample = texture(uDiffuseMap, vTexCoord);
        baseRgb *= texSample.rgb;
    }

    vec3 n = normalize(vNormal);
    float shade = 1.0;
    if (uFaceContrast > 0.0) {
        float dirFactor = clamp(dot(n, normalize(vec3(0.45, 0.80, 0.35))) * 0.5 + 0.5, 0.0, 1.0);
        shade = mix(1.0, 0.65 + 0.35 * dirFactor, clamp(uFaceContrast, 0.0, 1.0));
    }

    FragColor = vec4(baseRgb * shade, uColor.a);
}
