#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vTexCoord;
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
void main() {
    vec3 base = linearColor(uColor.rgb);
    if (uHasTexture != 0) base *= texture(uDiffuseMap,vTexCoord).rgb;
    vec3 n = unitVector(vNormal);
    if (uFloor) {
        // Vân sồi nhẹ trong tọa độ mét, không bị kéo dãn khi đổi kích thước phòng.
        base *= 0.96 + 0.04*cos(18.0*vWorld.x + 0.7*sin(2.0*vWorld.z));
    }
    vec3 rgb = base;
    if (uLit) {
        vec3 v = unitVector(uOrthographic ? uOrthoView : uEye-vWorld);
        rgb = uAmbient*base + illuminate(base,n,v,unitVector(-uSunDirection),uSunRadiance);
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
