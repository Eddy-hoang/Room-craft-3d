#version 330 core
out vec2 vUv;

void main()
{
    // Fullscreen triangle không cần VBO: 3 đỉnh phủ trọn NDC [-1, 1]
    vec2 pos = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                    (gl_VertexID == 2) ? 3.0 : -1.0);
    vUv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.9999, 1.0);
}
