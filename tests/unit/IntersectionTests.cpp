#include "roomcraft/math/Intersection.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace {

bool approx(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

bool check(bool cond, const char* label, int& failures)
{
    std::cout << "  [Intersection] " << label << ": " << (cond ? "PASS" : "FAIL") << std::endl;
    if (!cond) ++failures;
    return cond;
}

} // namespace

int runIntersectionTests()
{
    int failures = 0;

    roomcraft::math::BoundingBox unitBox;
    unitBox.expand(glm::vec3(-1.0f, 0.0f, -1.0f));
    unitBox.expand(glm::vec3(1.0f, 2.0f, 1.0f));

    // TC-M5-09: Ray-AABB standard hit
    {
        roomcraft::math::Ray r;
        roomcraft::math::Ray::create({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, r);
        roomcraft::math::RayAabbHit hit;
        const bool ok = roomcraft::math::intersectRayAABB(r, unitBox, hit);
        check(ok && approx(hit.tHit, 4.0f) && approx(hit.point.z, 1.0f) && !hit.startedInside,
              "TC-M5-09 Ray-AABB standard hit at exact distance t=4.0", failures);
    }

    // TC-M5-10: Ray-AABB miss + box behind ray
    {
        roomcraft::math::Ray missRay, behindRay;
        roomcraft::math::Ray::create({3.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}, missRay);
        roomcraft::math::Ray::create({0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, 1.0f}, behindRay);
        roomcraft::math::RayAabbHit h1, h2;
        check(!roomcraft::math::intersectRayAABB(missRay, unitBox, h1) &&
              !roomcraft::math::intersectRayAABB(behindRay, unitBox, h2),
              "TC-M5-10 Ray-AABB miss & box behind ray rejected", failures);
    }

    // TC-M5-11: Parallel ray (inside slab -> hits; outside slab -> misses without divide-by-zero)
    {
        roomcraft::math::Ray parIn, parOut;
        roomcraft::math::Ray::create({0.5f, 1.0f, -5.0f}, {0.0f, 0.0f, 1.0f}, parIn);
        roomcraft::math::Ray::create({2.5f, 1.0f, -5.0f}, {0.0f, 0.0f, 1.0f}, parOut);
        roomcraft::math::RayAabbHit hIn, hOut;
        check(roomcraft::math::intersectRayAABB(parIn, unitBox, hIn) && approx(hIn.tHit, 4.0f) &&
              !roomcraft::math::intersectRayAABB(parOut, unitBox, hOut),
              "TC-M5-11 Parallel axis-aligned ray inside/outside slab handled safely", failures);
    }

    // TC-M5-12: Ray origin inside box
    {
        roomcraft::math::Ray insideRay;
        roomcraft::math::Ray::create({0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, insideRay);
        roomcraft::math::RayAabbHit hit;
        check(roomcraft::math::intersectRayAABB(insideRay, unitBox, hit) && hit.startedInside && approx(hit.tHit, 0.0f),
              "TC-M5-12 Ray starting inside AABB reports hit with startedInside=true, tHit=0", failures);
    }

    // TC-M5-13: Negative direction ray + invalid/empty box
    {
        roomcraft::math::Ray negRay;
        roomcraft::math::Ray::create({4.0f, 1.0f, 4.0f}, {-1.0f, 0.0f, -1.0f}, negRay);
        roomcraft::math::RayAabbHit hNeg, hBad;
        roomcraft::math::BoundingBox invalidBox;
        check(roomcraft::math::intersectRayAABB(negRay, unitBox, hNeg) && hNeg.tHit > 0.0f &&
              !roomcraft::math::intersectRayAABB(negRay, invalidBox, hBad),
              "TC-M5-13 Negative ray direction hits & invalid AABB rejected", failures);
    }

    // TC-M5-26: Ray-Plane intersection (standard, parallel, origin on plane, plane behind ray)
    {
        const auto plane = roomcraft::math::Plane::horizontalAtY(0.75f);
        roomcraft::math::Ray downRay, parRay, onPlaneRay, upRay;
        roomcraft::math::Ray::create({1.0f, 2.75f, -2.0f}, {0.0f, -1.0f, 0.0f}, downRay);
        roomcraft::math::Ray::create({0.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, parRay);
        roomcraft::math::Ray::create({2.0f, 0.75f, 3.0f}, {0.0f, -1.0f, 0.0f}, onPlaneRay);
        roomcraft::math::Ray::create({0.0f, 2.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, upRay);

        roomcraft::math::RayPlaneHit hDown, hPar, hOn, hUp;
        const bool ok = roomcraft::math::intersectRayPlane(downRay, plane, hDown) &&
                        approx(hDown.t, 2.0f) && approx(hDown.point.y, 0.75f) && approx(hDown.point.x, 1.0f) &&
                        !roomcraft::math::intersectRayPlane(parRay, plane, hPar) &&
                        roomcraft::math::intersectRayPlane(onPlaneRay, plane, hOn, true) && approx(hOn.t, 0.0f) &&
                        !roomcraft::math::intersectRayPlane(upRay, plane, hUp);
        check(ok, "TC-M5-26 Ray-Plane intersection (hit, parallel, origin-on-plane, behind-origin)", failures);
    }

    // Narrow-phase Ray-Triangle (Möller-Trumbore)
    {
        roomcraft::math::Ray rHit, rMiss;
        roomcraft::math::Ray::create({0.25f, 0.25f, 2.0f}, {0.0f, 0.0f, -1.0f}, rHit);
        roomcraft::math::Ray::create({0.90f, 0.90f, 2.0f}, {0.0f, 0.0f, -1.0f}, rMiss);
        roomcraft::math::RayTriangleHit th1, th2;
        const bool ok = roomcraft::math::intersectRayTriangle(rHit, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, th1) &&
                        approx(th1.t, 2.0f) &&
                        !roomcraft::math::intersectRayTriangle(rMiss, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, th2);
        check(ok, "Möller-Trumbore Ray-Triangle hit & miss precision", failures);
    }

    return failures;
}
