#version 330 core

// Vertex shader 3D chuẩn cho Milestone M1 (Core Graphics Engine):
// Biến đổi vị trí đỉnh từ không gian cục bộ (Local) sang Clip Space:
//   p_clip = Projection * View * Model * p_local
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    // Truyền pháp tuyến cục bộ để phân biệt nhẹ các mặt khối 3D (chưa phải Phong Lighting của M7)
    vNormal = aNormal;
    vTexCoord = aTexCoord;
}
