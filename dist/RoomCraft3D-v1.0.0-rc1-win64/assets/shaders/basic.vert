#version 330 core

// Vertex shader cơ bản cho Milestone M0:
// Nhận vị trí 3D và màu RGB của từng đỉnh trực tiếp từ VBO thông qua VAO.
// Trong M0, tọa độ đầu vào đã nằm trong không gian Normalized Device Coordinates (NDC [-1, 1]),
// chưa áp dụng ma trận Model-View-Projection (sẽ triển khai ở Milestone M1).
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
