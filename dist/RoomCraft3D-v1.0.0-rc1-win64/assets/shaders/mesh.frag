#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vLightSpacePos;
out vec4 FragColor;

uniform vec4 uColor;
uniform sampler2D uDiffuseMap;
uniform int uHasTexture;
uniform bool uLit;
uniform vec3 uEye, uOrthoView;
uniform bool uOrthographic;
uniform vec3 uSunDirection = vec3(-0.6,-1.0,-0.45);
uniform vec3 uSunRadiance = vec3(0.62);
uniform vec3 uPointPosition, uPointRadiance;
uniform vec3 uAttenuation = vec3(1.0,0.18,0.12);
uniform float uAmbient = 0.5;
uniform float uShininess;
uniform vec3 uSpecular;
uniform bool uFloor;
uniform vec3 uGrid; // nửa chiều rộng, nửa chiều dài, bước lưới (0 = tắt)

// Directional Shadow Mapping & Architectural Corner Shading (M9.4 — mặc định tắt uShadowEnabled=0 để bảo toàn M7 GPU tests)
uniform sampler2D uShadowMap;
uniform int uShadowEnabled = 0;
uniform float uShadowBias = 0.0022;
uniform vec3 uRoomHalfSize = vec3(0.0); // (halfW, height, halfL); z > 0 khi đang vẽ cảnh Room Editor

vec3 unitVector(vec3 v) { return v * inversesqrt(max(dot(v,v), 1e-12)); }
vec3 linearColor(vec3 c) {
    return mix(pow((c + 0.055) / 1.055, vec3(2.4)), c / 12.92, lessThanEqual(c, vec3(0.04045)));
}
vec3 displayColor(vec3 c) {
    return mix(1.055 * pow(c, vec3(1.0/2.4)) - 0.055, 12.92*c, lessThanEqual(c, vec3(0.0031308)));
}
vec3 illuminate(vec3 albedo, vec3 n, vec3 v, vec3 l, vec3 radiance) {
    float diffuse = dot(n,l);
    if (diffuse <= 0.0) return vec3(0);
    float highlight = pow(max(dot(n,unitVector(l+v)),0.0), max(uShininess,1.0));
    return radiance * (albedo * diffuse + uSpecular * highlight);
}

// Lọc mềm 3x3 PCF cho Directional Shadow Map (giảm răng cưa bóng và triệt tiêu shadow acne bằng slope-scaled bias)
float computeSunVisibility(vec4 lightSpacePos, vec3 n, vec3 sunDir) {
    vec3 proj = lightSpacePos.xyz / max(lightSpacePos.w, 1e-6);
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) {
        return 1.0;
    }
    float ndotl = max(dot(n, sunDir), 0.0);
    float bias = max(uShadowBias * (1.0 - ndotl) * 2.2, uShadowBias * 0.45);
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float litSum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth = texture(uShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            litSum += (proj.z - bias <= closestDepth) ? 1.0 : 0.0;
        }
    }
    return litSum / 9.0;
}

// Khối sáng góc chân tường & góc phòng nhẹ giúp căn phòng có chiều sâu kiến trúc tự nhiên
float computeRoomCornerAO(vec3 p, vec3 n, vec3 halfSize) {
    if (halfSize.z <= 0.0) return 1.0;
    float dWest  = max(p.x + halfSize.x, 0.0);
    float dEast  = max(halfSize.x - p.x, 0.0);
    float dNorth = max(p.z + halfSize.z, 0.0);
    float dSouth = max(halfSize.z - p.z, 0.0);
    float dFloor = max(p.y, 0.0);
    float ao = 1.0;
    if (n.y > 0.7) {
        float wallDist = min(min(dWest, dEast), min(dNorth, dSouth));
        ao *= mix(0.86, 1.0, smoothstep(0.0, 0.32, wallDist));
    } else if (abs(n.y) < 0.5) {
        float cornerX = min(dWest, dEast);
        float cornerZ = min(dNorth, dSouth);
        float verticalCorner = max(cornerX, cornerZ);
        ao *= mix(0.86, 1.0, smoothstep(0.0, 0.28, dFloor));
        ao *= mix(0.90, 1.0, smoothstep(0.0, 0.26, verticalCorner));
    }
    return ao;
}

void main() {
    vec3 base = linearColor(uColor.rgb);
    if (uHasTexture != 0) base *= texture(uDiffuseMap,vTexCoord).rgb;
    vec3 n = unitVector(vNormal);
    if (uFloor) {
        // Vân sồi tự nhiên trong tọa độ mét, không bị kéo dãn khi đổi kích thước phòng.
        float grain = 0.96 + 0.04*cos(18.0*vWorld.x + 0.7*sin(2.0*vWorld.z));
        if (uShadowEnabled != 0) {
            // Mạch ghép ván gỗ tinh tế khi bật chế độ render đầy đủ
            float plankIdx = floor(vWorld.x / 0.20);
            float plankHash = fract(sin(plankIdx * 127.1) * 43758.5453);
            float seamX = smoothstep(0.0, 0.009, abs(fract(vWorld.x / 0.20 + 0.5) - 0.5) * 0.20);
            float staggeredZ = vWorld.z + plankHash * 1.15;
            float seamZ = smoothstep(0.0, 0.008, abs(fract(staggeredZ / 1.25 + 0.5) - 0.5) * 1.25);
            float plankTone = 0.94 + 0.11 * plankHash;
            float plankSeam = mix(0.82, 1.0, min(seamX, seamZ));
            base *= grain * plankTone * plankSeam;
        } else {
            base *= grain;
        }
    }
    vec3 rgb = base;
    if (uLit) {
        vec3 v = unitVector(uOrthographic ? uOrthoView : uEye-vWorld);
        vec3 sunDir = unitVector(-uSunDirection);
        float sunVis = (uShadowEnabled != 0) ? computeSunVisibility(vLightSpacePos, n, sunDir) : 1.0;
        float cornerAo = (uShadowEnabled != 0) ? computeRoomCornerAO(vWorld, n, uRoomHalfSize) : 1.0;
        // Khi bật Shadow Mapping, dùng ambient bán cầu nhẹ (trần sáng hơn đáy) để tăng độ nổi khối 3D
        float hemiFactor = (uShadowEnabled != 0) ? mix(0.84, 1.06, 0.5 * (n.y + 1.0)) : 1.0;
        rgb = (uAmbient * hemiFactor * cornerAo) * base
            + sunVis * illuminate(base, n, v, sunDir, uSunRadiance);
        vec3 delta = uPointPosition-vWorld;
        float d = length(delta);
        float divisor = max(dot(uAttenuation,vec3(1,d,d*d)),0.001);
        rgb += illuminate(base,n,v,unitVector(delta),uPointRadiance/divisor);
    }
    rgb = displayColor(max(rgb,vec3(0)));
    if (uFloor && uGrid.z > 0.0) {
        vec2 cell = vWorld.xz/uGrid.z;
        vec2 footprint = max(fwidth(cell),vec2(0.0001));
        vec2 distanceInCells = min(abs(fract(cell+0.5)-0.5), abs(abs(vWorld.xz)-uGrid.xy)/uGrid.z);
        vec2 coverage = (1.0-smoothstep(vec2(0.0),footprint,distanceInCells))
                      * (1.0-smoothstep(vec2(0.25),vec2(1.0),footprint));
        if (n.y > 0.9 && all(lessThanEqual(abs(vWorld.xz),uGrid.xy)))
            rgb = mix(rgb,vec3(0.32,0.36,0.44),0.59*max(coverage.x,coverage.y));
    }
    FragColor = vec4(rgb,uColor.a);
}
