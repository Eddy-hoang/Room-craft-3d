#version 330 core
in vec2 vUv;
out vec4 FragColor;

uniform vec3 uTopColor = vec3(0.115, 0.130, 0.162);
uniform vec3 uBottomColor = vec3(0.082, 0.094, 0.118);
uniform vec3 uCenterGlow = vec3(0.172, 0.194, 0.238);

void main()
{
    // Gradient studio trung tính tinh tế: sáng nhẹ tâm nhìn, tối dần về mép viewport
    vec2 centered = (vUv - vec2(0.5, 0.52)) * vec2(1.15, 1.25);
    float radial = clamp(dot(centered, centered), 0.0, 1.0);
    vec3 baseGrad = mix(uBottomColor, uTopColor, smoothstep(0.0, 1.0, vUv.y));
    vec3 col = mix(uCenterGlow, baseGrad, smoothstep(0.0, 0.92, radial));

    // Dither rất nhỏ (1/255) triệt tiêu phân bậc 8-bit trên nền tối
    float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
    col = clamp(col + vec3(dither / 255.0), 0.0, 1.0);
    FragColor = vec4(col, 1.0);
}
