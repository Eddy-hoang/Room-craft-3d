#version 330 core

// Vertex shader cho lưới mặt sàn (Grid) và hệ trục tọa độ XYZ (GL_LINES)
layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
