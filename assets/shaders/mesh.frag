#version 330 core

// Fragment shader 3D cơ bản cho Milestone M1:
// Xuất màu cơ bản uColor; khi uFaceContrast > 0 sẽ tạo độ tương phản nhẹ giữa các mặt hộp
// để dễ quan sát hình khối 3D (khi uFaceContrast == 0 sẽ xuất màu chính xác tuyệt đối uColor).
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec4 uColor;
uniform float uFaceContrast;

out vec4 FragColor;

void main()
{
    vec3 n = normalize(vNormal);
    // Hệ số độ sáng đơn giản theo hướng pháp tuyến giúp phân biệt 6 mặt lập phương trong M1
    float shade = 1.0;
    if (uFaceContrast > 0.0) {
        float dirFactor = clamp(dot(n, normalize(vec3(0.45, 0.80, 0.35))) * 0.5 + 0.5, 0.0, 1.0);
        shade = mix(1.0, 0.65 + 0.35 * dirFactor, clamp(uFaceContrast, 0.0, 1.0));
    }

    FragColor = vec4(uColor.rgb * shade, uColor.a);
}
