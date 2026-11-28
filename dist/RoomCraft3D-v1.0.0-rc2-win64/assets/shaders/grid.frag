#version 330 core

// Fragment shader cho lưới mặt sàn (Grid) và hệ trục tọa độ XYZ
uniform vec4 uColor;

out vec4 FragColor;

void main()
{
    FragColor = uColor;
}
