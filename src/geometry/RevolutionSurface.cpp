#include "roomcraft/geometry/RevolutionSurface.hpp"
#include "roomcraft/geometry/GeometryUtils.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace roomcraft::geometry {

namespace {

bool failWith(std::string* outError, const std::string& message)
{
    if (outError != nullptr) {
        *outError = message;
    }
    return false;
}

} // namespace

bool RevolutionSurface::generateFromProfile(const std::vector<glm::vec2>& profilePoints,
                                            int radialSegments,
                                            bool capBottom,
                                            bool capTop,
                                            MeshData& outMesh,
                                            std::string* outError)
{
    if (profilePoints.size() < 2) {
        return failWith(outError, "RevolutionSurface requires at least 2 profile points.");
    }
    if (radialSegments < limits::kMinRadialSegments || radialSegments > limits::kMaxRadialSegments) {
        return failWith(outError, "RevolutionSurface radialSegments must be in range [3, 256].");
    }

    // Kiểm tra toàn bộ điểm biên: hữu hạn và KHÔNG cho phép bán kính âm
    bool hasPositiveRadius = false;
    for (std::size_t i = 0; i < profilePoints.size(); ++i) {
        const auto& pt = profilePoints[i];
        if (!std::isfinite(pt.x) || !std::isfinite(pt.y)) {
            return failWith(outError, "RevolutionSurface profile contains non-finite values.");
        }
        if (pt.x < 0.0f) {
            return failWith(outError, "RevolutionSurface profile cannot contain negative radius.");
        }
        if (pt.x >= limits::kMinDimension) {
            hasPositiveRadius = true;
        }
    }
    if (!hasPositiveRadius) {
        return failWith(outError, "RevolutionSurface profile must have at least one point with radius > 0.");
    }

    // Lọc bỏ các điểm liên tiếp trùng nhau hoàn toàn và chuẩn hóa thứ tự từ dưới lên trên
    std::vector<glm::vec2> profile;
    profile.reserve(profilePoints.size());
    for (const auto& pt : profilePoints) {
        if (profile.empty() || glm::length(pt - profile.back()) > 1e-5f) {
            profile.push_back(pt);
        }
    }
    if (profile.size() < 2) {
        return failWith(outError, "RevolutionSurface profile collapsed to fewer than 2 distinct points.");
    }
    if (profile.back().y < profile.front().y) {
        std::reverse(profile.begin(), profile.end());
    }

    // Tính độ dài cung tích lũy để ánh xạ tọa độ V đồng đều trên [0, 1]
    const std::size_t ringCount = profile.size();
    std::vector<float> arcV(ringCount, 0.0f);
    float totalArc = 0.0f;
    for (std::size_t k = 1; k < ringCount; ++k) {
        totalArc += glm::length(profile[k] - profile[k - 1]);
        arcV[k] = totalArc;
    }
    if (totalArc > 1e-6f) {
        for (float& v : arcV) {
            v /= totalArc;
        }
    }

    // Tính vector pháp tuyến 2D (nRadial, nY) tại mỗi điểm biên dựa trên tiếp tuyến đường biên:
    // Tiếp tuyến đi từ dưới lên là (dr, dy) => pháp tuyến hướng ra ngoài là (+dy, -dr) chuẩn hóa
    std::vector<glm::vec2> profileNormals(ringCount, glm::vec2(1.0f, 0.0f));
    for (std::size_t k = 0; k < ringCount; ++k) {
        glm::vec2 tangent(0.0f, 1.0f);
        if (k == 0) {
            tangent = profile[1] - profile[0];
        } else if (k + 1 == ringCount) {
            tangent = profile[k] - profile[k - 1];
        } else {
            tangent = profile[k + 1] - profile[k - 1];
        }

        glm::vec2 n2d(tangent.y, -tangent.x);
        const float nLen = glm::length(n2d);
        if (nLen > 1e-6f) {
            profileNormals[k] = n2d / nLen;
        } else {
            profileNormals[k] = glm::vec2(1.0f, 0.0f);
        }
    }

    MeshData mesh;
    mesh.topology = graphics::PrimitiveTopology::Triangles;

    const int cols = radialSegments + 1; // +1 cột tạo UV seam sạch từ u = 0 đến u = 1
    mesh.vertices.reserve(ringCount * static_cast<std::size_t>(cols));

    for (std::size_t k = 0; k < ringCount; ++k) {
        const float r = profile[k].x;
        const float y = profile[k].y;
        const float v = arcV[k];
        const glm::vec2 n2d = profileNormals[k];

        for (int i = 0; i <= radialSegments; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(radialSegments);
            const float theta = u * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);

            const glm::vec3 pos(r * cosT, y, r * sinT);
            glm::vec3 normal(n2d.x * cosT, n2d.y, n2d.x * sinT);

            // Xử lý an toàn điểm trên trục xoay (r == 0): tránh pháp tuyến bị suy biến bằng cách hướng dọc theo trục Y
            if (r < limits::kMinDimension && glm::length(normal) < 1e-5f) {
                normal = (k == 0) ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                normal = glm::normalize(normal);
            }

            mesh.vertices.push_back({pos, normal, {u, v}});
        }
    }

    // Nối các vòng đỉnh kề nhau; xử lý đặc biệt khi một vòng có bán kính = 0 để không sinh tam giác suy biến
    for (std::size_t k = 0; k + 1 < ringCount; ++k) {
        const bool r0Zero = (profile[k].x < limits::kMinDimension);
        const bool r1Zero = (profile[k + 1].x < limits::kMinDimension);
        if (r0Zero && r1Zero) {
            continue;
        }

        const auto row0 = static_cast<std::uint32_t>(k * static_cast<std::size_t>(cols));
        const auto row1 = static_cast<std::uint32_t>((k + 1) * static_cast<std::size_t>(cols));

        for (int i = 0; i < radialSegments; ++i) {
            const std::uint32_t b0 = row0 + static_cast<std::uint32_t>(i);
            const std::uint32_t b1 = b0 + 1;
            const std::uint32_t t0 = row1 + static_cast<std::uint32_t>(i);
            const std::uint32_t t1 = t0 + 1;

            if (r0Zero) {
                // Cực đáy trên trục xoay -> 1 tam giác mỗi sector
                mesh.indices.push_back(b0);
                mesh.indices.push_back(t0);
                mesh.indices.push_back(t1);
            } else if (r1Zero) {
                // Cực đỉnh trên trục xoay -> 1 tam giác mỗi sector
                mesh.indices.push_back(b0);
                mesh.indices.push_back(t0);
                mesh.indices.push_back(b1);
            } else {
                // Dải hình trụ/nón cụt -> 2 tam giác CCW hướng ra ngoài
                mesh.indices.push_back(b0);
                mesh.indices.push_back(t0);
                mesh.indices.push_back(t1);

                mesh.indices.push_back(b0);
                mesh.indices.push_back(t1);
                mesh.indices.push_back(b1);
            }
        }
    }

    // Nắp đáy phẳng (-Y) nếu được yêu cầu và vòng đáy có bán kính dương
    if (capBottom && profile.front().x >= limits::kMinDimension) {
        const float r = profile.front().x;
        const float y = profile.front().y;
        const glm::vec3 n(0.0f, -1.0f, 0.0f);

        const auto centerIdx = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({{0.0f, y, 0.0f}, n, {0.5f, 0.5f}});

        const auto ringStart = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float theta = (static_cast<float>(i) / static_cast<float>(radialSegments)) * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);
            mesh.vertices.push_back({{r * cosT, y, r * sinT}, n, {0.5f + 0.5f * cosT, 0.5f + 0.5f * sinT}});
        }

        for (int i = 0; i < radialSegments; ++i) {
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i));
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i + 1));
        }
    }

    // Nắp đỉnh phẳng (+Y) nếu được yêu cầu và vòng đỉnh có bán kính dương
    if (capTop && profile.back().x >= limits::kMinDimension) {
        const float r = profile.back().x;
        const float y = profile.back().y;
        const glm::vec3 n(0.0f, 1.0f, 0.0f);

        const auto centerIdx = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({{0.0f, y, 0.0f}, n, {0.5f, 0.5f}});

        const auto ringStart = static_cast<std::uint32_t>(mesh.vertices.size());
        for (int i = 0; i <= radialSegments; ++i) {
            const float theta = (static_cast<float>(i) / static_cast<float>(radialSegments)) * limits::kTwoPi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);
            mesh.vertices.push_back({{r * cosT, y, r * sinT}, n, {0.5f + 0.5f * cosT, 0.5f - 0.5f * sinT}});
        }

        for (int i = 0; i < radialSegments; ++i) {
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i + 1));
            mesh.indices.push_back(ringStart + static_cast<std::uint32_t>(i));
        }
    }

    mesh.recomputeBoundingBox();
    if (!mesh.validate(outError)) {
        return false;
    }

    outMesh = std::move(mesh);
    return true;
}

bool RevolutionSurface::generateFromBezierProfile(const BezierCurve& profileCurve,
                                                  int profileSegments,
                                                  int radialSegments,
                                                  bool capBottom,
                                                  bool capTop,
                                                  MeshData& outMesh,
                                                  std::string* outError)
{
    std::vector<glm::vec3> sampled3D;
    if (!profileCurve.sample(profileSegments, sampled3D, outError)) {
        return false;
    }

    std::vector<glm::vec2> profile2D;
    profile2D.reserve(sampled3D.size());
    for (const auto& pt : sampled3D) {
        profile2D.emplace_back(pt.x, pt.y);
    }

    return generateFromProfile(profile2D, radialSegments, capBottom, capTop, outMesh, outError);
}

} // namespace roomcraft::geometry
