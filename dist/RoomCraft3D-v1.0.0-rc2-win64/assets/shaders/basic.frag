#version 330 core

// Fragment shader cơ bản cho Milestone M0:
// Nhận màu nội suy tuyến tính (barycentric interpolation) từ Rasterizer
// và xuất màu RGBA cuối cùng ra framebuffer.
in vec3 vColor;

out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
