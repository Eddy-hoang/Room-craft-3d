#include "roomcraft/geometry/BezierCurve.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr float kEps = 1e-4f;

bool approxVec3(const glm::vec3& a, const glm::vec3& b, float eps = kEps)
{
    return glm::length(a - b) <= eps;
}

} // namespace

int runBezierCurveTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [BezierCurve] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    const glm::vec3 p0(0.0f, 0.0f, 0.0f);
    const glm::vec3 p1(0.5f, 1.2f, 0.3f);
    const glm::vec3 p2(1.5f, 1.8f, -0.4f);
    const glm::vec3 p3(2.2f, 0.4f, 0.2f);
    const roomcraft::geometry::BezierCurve curve(p0, p1, p2, p3);

    // TC-M2-07: Endpoints B(0) = P0, B(1) = P3 & đối chiếu Bernstein vs de Casteljau
    {
        glm::vec3 b0, b1, bMid, dcMid;
        const bool ok0 = curve.evaluate(0.0f, b0);
        const bool ok1 = curve.evaluate(1.0f, b1);
        const bool okMid = curve.evaluate(0.37f, bMid) && curve.evaluateDeCasteljau(0.37f, dcMid);
        check("TC-M2-07 Endpoints B(0)==P0, B(1)==P3 and de Casteljau agreement",
              ok0 && ok1 && okMid && approxVec3(b0, p0) && approxVec3(b1, p3) && approxVec3(bMid, dcMid));
    }

    // TC-M2-08: Derivatives at endpoints, collinear control points, sampling & invalid inputs
    {
        glm::vec3 d0, d1;
        const bool okD = curve.derivative(0.0f, d0) && curve.derivative(1.0f, d1);
        const bool derivOk = okD &&
                             approxVec3(d0, 3.0f * (p1 - p0)) &&
                             approxVec3(d1, 3.0f * (p3 - p2));

        // 4 điểm thẳng hàng trên đoạn (0,0,0) -> (3,3,0) phải cho điểm nằm đúng trên đường thẳng
        const roomcraft::geometry::BezierCurve straight(
            {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {2.0f, 2.0f, 0.0f}, {3.0f, 3.0f, 0.0f});
        std::vector<glm::vec3> pts10;
        std::vector<glm::vec3> pts25;
        const bool sampleOk = straight.sample(10, pts10) && straight.sample(25, pts25) &&
                              pts10.size() == 11 && pts25.size() == 26;
        bool collinearOk = sampleOk;
        for (const auto& pt : pts10) {
            if (std::fabs(pt.x - pt.y) > kEps || std::fabs(pt.z) > kEps) {
                collinearOk = false;
            }
        }

        // Tham số t ngoài [0, 1] hoặc segmentCount <= 0 phải bị từ chối
        glm::vec3 dummy;
        std::string err;
        const bool invalidRejected = !curve.evaluate(-0.1f, dummy, &err) &&
                                     !curve.evaluate(1.1f, dummy, &err) &&
                                     !curve.sample(0, pts10, &err);

        check("TC-M2-08 Derivatives, collinearity, sampling count & invalid t rejection",
              derivOk && collinearOk && invalidRejected);
    }

    return failures;
}
