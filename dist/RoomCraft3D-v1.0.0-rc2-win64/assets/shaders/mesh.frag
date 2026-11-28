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
uniform vec3 uSunDirection = vec3(-0.58, -0.95, -0.42);
uniform vec3 uSunRadiance = vec3(0.92);
uniform vec3 uPointPosition, uPointRadiance;
uniform vec3 uAttenuation = vec3(1.0, 0.34, 0.24);
uniform float uAmbient = 0.30;
uniform float uShininess;
uniform vec3 uSpecular;
uniform bool uFloor;
uniform vec3 uGrid; // nửa chiều rộng, nửa chiều dài, bước lưới (0 = tắt)

// Directional Shadow Mapping, ACES Tone Mapping & Architectural Corner AO (M9 RC2)
// Mặc định uShadowEnabled=0 và uToneMapEnabled=0 để bảo toàn 100% đường đo tuyến tính của M7 GPU tests.
uniform sampler2DShadow uShadowMap;
uniform int uShadowEnabled = 0;
uniform float uShadowBias = 0.0012;
uniform float uShadowTexelSize = 0.008;
uniform mat4 uLightSpaceMatrix = mat4(1.0);

uniform int uToneMapEnabled = 0;
uniform float uExposure = 0.88;
uniform int uIsRoomSurface = 0;
uniform vec3 uRoomHalfSize = vec3(0.0); // (halfW, height, halfL)
uniform vec4 uVisibleWalls = vec4(1.0, 0.0, 1.0, 0.0); // x=North, y=South, z=West, w=East

// Soft Contact Grounding Shadows trên sàn dưới đồ nội thất đứng sàn (RC2.4 — chỉ ảnh hưởng ambient)
uniform int uContactCount = 0;
uniform vec4 uContactCenterExt[16]; // (cx, cz, hx, hz)
uniform vec4 uContactBasis[16];     // (cosYaw, sinYaw, radiusSoft, strength)

vec3 unitVector(vec3 v) { return v * inversesqrt(max(dot(v, v), 1e-12)); }

vec3 linearColor(vec3 c) {
    return mix(pow((c + 0.055) / 1.055, vec3(2.4)), c / 12.92, lessThanEqual(c, vec3(0.04045)));
}

vec3 displayColor(vec3 c) {
    return mix(1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, 12.92 * c, lessThanEqual(c, vec3(0.0031308)));
}

// ACES Filmic Tone Mapping (Narkowicz 2015) chạy trên dữ liệu tuyến tính HDR trước khi chuyển sRGB
vec3 acesFilmicToneMap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 illuminate(vec3 albedo, vec3 n, vec3 v, vec3 l, vec3 radiance) {
    float diffuse = dot(n, l);
    if (diffuse <= 0.0) return vec3(0.0);
    vec3 h = unitVector(l + v);
    float ndoth = max(dot(n, h), 0.0);
    float shin = max(uShininess, 1.0);
    float highlight = pow(ndoth, shin);
    if (uToneMapEnabled != 0) {
        // Phiên bản Blinn-Phong cân bằng năng lượng + Schlick Fresnel nhẹ cho cảnh Studio/Room Editor (RC2)
        float normFactor = (shin + 8.0) / 25.132741; // (Ns + 8) / (8 * pi)
        float vdotH = clamp(dot(v, h), 0.0, 1.0);
        vec3 f0 = clamp(uSpecular, vec3(0.0), vec3(0.95));
        float fPeak = max(f0.r, max(f0.g, f0.b));
        vec3 fresnel = f0 + (vec3(min(0.35, fPeak * 1.4 + 0.03)) - f0) * pow(1.0 - vdotH, 5.0);
        vec3 kD = vec3(1.0) - clamp(fresnel * 0.5, vec3(0.0), vec3(0.45));
        return radiance * (kD * albedo * diffuse + fresnel * (highlight * min(normFactor, 3.5) * 0.42));
    }
    return radiance * (albedo * diffuse + uSpecular * highlight);
}

// Hardware Bilinear PCF (sampler2DShadow) kết hợp Normal-Offset Bias & 9-tap tent kernel (chống acne & răng cưa)
float computeSunVisibility(vec3 worldPos, vec3 n, vec3 sunDir) {
    float ndotl = dot(n, sunDir);
    if (ndotl <= 0.0005) {
        return 0.0;
    }

    // Dịch điểm lấy mẫu dọc pháp tuyến theo kích thước texel thực tế để triệt tiêu self-acne mà không tách bóng chân đồ
    float slope = 1.0 - clamp(ndotl, 0.0, 1.0);
    float normalOffset = uShadowTexelSize * (0.40 + 1.10 * slope);
    vec4 biasedLs = uLightSpaceMatrix * vec4(worldPos + n * normalOffset, 1.0);
    vec3 proj = biasedLs.xyz / max(biasedLs.w, 1e-6);
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) {
        return 1.0;
    }

    float depthBias = uShadowBias * (0.25 + 0.50 * slope);
    float refDepth = clamp(proj.z - depthBias, 0.0, 1.0);
    vec2 texelStep = 1.0 / vec2(textureSize(uShadowMap, 0));

    // 5-tap rotated hardware bilinear PCF: mỗi lần texture(sampler2DShadow) lọc 2x2 bilinear trên GPU
    // -> 20 mẫu thực tế, cạnh bóng mượt không bậc thang với chi phí băng thông tối ưu cho Intel UHD.
    vec2 o = texelStep * 0.90;
    float litSum = texture(uShadowMap, vec3(proj.xy,                        refDepth)) * 2.0
                 + texture(uShadowMap, vec3(proj.xy + vec2(-o.x, -o.y),     refDepth))
                 + texture(uShadowMap, vec3(proj.xy + vec2( o.x, -o.y),     refDepth))
                 + texture(uShadowMap, vec3(proj.xy + vec2(-o.x,  o.y),     refDepth))
                 + texture(uShadowMap, vec3(proj.xy + vec2( o.x,  o.y),     refDepth));
    return litSum / 6.0;
}

// Xấp xỉ phân tích góc chân tường & góc tường-tường (CHỈ áp dụng lên bề mặt phòng đang hiển thị,
// không làm tối đồ nội thất và không làm tối mép sàn Cutaway trống — khắc phục BUG-07).
float computeRoomCornerAO(vec3 p, vec3 n, vec3 halfSize, vec4 visWalls) {
    if (uIsRoomSurface == 0 || halfSize.z <= 0.0) return 1.0;
    float dNorth = (visWalls.x > 0.5) ? max(p.z + halfSize.z, 0.0) : 99.0;
    float dSouth = (visWalls.y > 0.5) ? max(halfSize.z - p.z, 0.0) : 99.0;
    float dWest  = (visWalls.z > 0.5) ? max(p.x + halfSize.x, 0.0) : 99.0;
    float dEast  = (visWalls.w > 0.5) ? max(halfSize.x - p.x, 0.0) : 99.0;
    float dFloor = max(p.y, 0.0);

    float ao = 1.0;
    if (n.y > 0.7) {
        // Mặt sàn: chỉ làm tối dịu chân các bức tường THẬT SỰ đang hiển thị
        float wallDist = min(min(dWest, dEast), min(dNorth, dSouth));
        ao *= mix(0.84, 1.0, smoothstep(0.0, 0.30, wallDist));
    } else if (abs(n.y) < 0.5) {
        // Mặt tường đứng: tối nhẹ sát sàn và tại góc giao giữa 2 tường đang hiển thị
        float cornerX = min(dWest, dEast);
        float cornerZ = min(dNorth, dSouth);
        float verticalCorner = max(cornerX, cornerZ);
        ao *= mix(0.86, 1.0, smoothstep(0.0, 0.24, dFloor));
        ao *= mix(0.89, 1.0, smoothstep(0.0, 0.26, verticalCorner));
    }
    return ao;
}

// Soft Contact Grounding Shadow (RC2.4): mô phỏng bóng tiếp xúc nhẹ dưới đáy đồ nội thất đứng sàn
// bằng hộp xoay OBB thu gọn trên mặt phẳng XZ; chỉ tác động vào ambient để tránh tối đúp với shadow map.
float computeFloorContactShadow(vec3 p) {
    if (!uFloor || uContactCount <= 0) return 1.0;
    float occ = 1.0;
    int count = min(uContactCount, 16);
    for (int i = 0; i < count; ++i) {
        vec4 ce = uContactCenterExt[i]; // (cx, cz, hx, hz)
        vec4 bs = uContactBasis[i];     // (cosYaw, sinYaw, softRadius, strength)
        vec2 d = p.xz - ce.xy;
        // Xoay ngược về hệ cục bộ của vật thể
        vec2 local = vec2(d.x * bs.x - d.y * bs.y,
                          d.x * bs.y + d.y * bs.x);
        vec2 q = abs(local) - ce.zw;
        float dist = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0);
        // Mượt tối đa ngay dưới thân đồ nội thất và tan hết ở khoảng cách softRadius
        float factor = smoothstep(-0.04, max(bs.z, 0.05), dist);
        occ = min(occ, mix(1.0 - bs.w, 1.0, factor));
    }
    return clamp(occ, 0.62, 1.0);
}

void main() {
    vec3 base = linearColor(uColor.rgb);
    if (uHasTexture != 0) base *= texture(uDiffuseMap, vTexCoord).rgb;
    vec3 n = unitVector(vNormal);

    if (uFloor) {
        // Vân gỗ sồi tự nhiên theo tọa độ mét, chống aliasing mạch ván ở góc nhìn xa bằng fwidth()
        float grain = 0.965 + 0.035 * cos(18.0 * vWorld.x + 0.7 * sin(2.2 * vWorld.z))
                    + 0.015 * sin(42.0 * vWorld.x + 1.3 * vWorld.z);
        if (uToneMapEnabled != 0 || uShadowEnabled != 0) {
            float plankW = 0.20;
            float plankL = 1.25;
            float plankIdx = floor(vWorld.x / plankW);
            float plankHash = fract(sin(plankIdx * 127.1 + 311.7) * 43758.5453);
            float distX = abs(fract(vWorld.x / plankW + 0.5) - 0.5) * plankW;
            float staggeredZ = vWorld.z + plankHash * 1.15;
            float distZ = abs(fract(staggeredZ / plankL + 0.5) - 0.5) * plankL;

            vec2 fw = max(fwidth(vWorld.xz), vec2(0.0005));
            float aaX = smoothstep(0.0, max(0.006, fw.x * 1.35), distX);
            float aaZ = smoothstep(0.0, max(0.006, fw.y * 1.35), distZ);
            // Làm mờ mạch ghép mượt khi khoảng cách xa để triệt tiêu sọc moiré
            float fadeX = 1.0 - smoothstep(0.03, 0.14, fw.x);
            float fadeZ = 1.0 - smoothstep(0.03, 0.14, fw.y);
            float seamX = mix(1.0, aaX, fadeX);
            float seamZ = mix(1.0, aaZ, fadeZ);

            float plankTone = 0.95 + 0.09 * plankHash;
            float plankSeam = mix(0.89, 1.0, min(seamX, seamZ));
            base *= grain * plankTone * plankSeam;
        } else {
            base *= grain;
        }
    }

    vec3 rgb = base;
    if (uLit) {
        vec3 v = unitVector(uOrthographic ? uOrthoView : uEye - vWorld);
        vec3 sunDir = unitVector(-uSunDirection);
        float sunVis = (uShadowEnabled != 0) ? computeSunVisibility(vWorld, n, sunDir) : 1.0;
        float cornerAo = (uToneMapEnabled != 0 || uShadowEnabled != 0)
                             ? computeRoomCornerAO(vWorld, n, uRoomHalfSize, uVisibleWalls)
                             : 1.0;

        vec3 ambientTerm;
        if (uToneMapEnabled != 0) {
            float contactAo = (uFloor && n.y > 0.7) ? computeFloorContactShadow(vWorld) : 1.0;
            // Ánh sáng bán cầu 2 tông màu (bầu trời dịu phía trên + phản xạ gỗ ấm phía dưới)
            vec3 skyTint = vec3(0.95, 0.98, 1.04);
            vec3 groundBounce = vec3(0.86, 0.80, 0.73);
            vec3 hemiColor = mix(groundBounce, skyTint, 0.5 * (n.y + 1.0));
            ambientTerm = (uAmbient * cornerAo * contactAo) * hemiColor * base;

            // Ánh sáng lấp (soft bounce fill) nhẹ từ hướng ngược mặt trời để vùng tối giữ được độ nổi khối
            vec3 fillDir = unitVector(vec3(-sunDir.x, 0.55, -sunDir.z));
            float fillDiff = max(dot(n, fillDir), 0.0);
            ambientTerm += (0.09 * cornerAo * contactAo * fillDiff) * (vec3(0.85, 0.91, 1.0) * max(length(uSunRadiance) * 0.58, 0.18)) * base;
        } else {
            float hemiFactor = (uShadowEnabled != 0) ? mix(0.84, 1.06, 0.5 * (n.y + 1.0)) : 1.0;
            ambientTerm = (uAmbient * hemiFactor * cornerAo) * base;
        }

        rgb = ambientTerm + sunVis * illuminate(base, n, v, sunDir, uSunRadiance);

        vec3 delta = uPointPosition - vWorld;
        float d = length(delta);
        float divisor = max(dot(uAttenuation, vec3(1.0, d, d * d)), 0.001);
        rgb += illuminate(base, n, v, unitVector(delta), uPointRadiance / divisor);

        if (uToneMapEnabled != 0) {
            rgb = acesFilmicToneMap(max(rgb, vec3(0.0)) * uExposure);
        }
    }

    // Chuyển tuyến tính -> sRGB ĐÚNG MỘT LẦN duy nhất sau tone mapping
    rgb = displayColor(max(rgb, vec3(0.0)));

    if (uFloor && uGrid.z > 0.0) {
        vec2 cell = vWorld.xz / uGrid.z;
        vec2 footprint = max(fwidth(cell), vec2(0.0001));
        vec2 distanceInCells = min(abs(fract(cell + 0.5) - 0.5), abs(abs(vWorld.xz) - uGrid.xy) / uGrid.z);
        vec2 coverage = (1.0 - smoothstep(vec2(0.0), footprint, distanceInCells))
                      * (1.0 - smoothstep(vec2(0.25), vec2(1.0), footprint));
        if (n.y > 0.9 && all(lessThanEqual(abs(vWorld.xz), uGrid.xy)))
            rgb = mix(rgb, vec3(0.32, 0.36, 0.44), 0.59 * max(coverage.x, coverage.y));
    }
    FragColor = vec4(rgb, uColor.a);
}
