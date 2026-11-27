#include "roomcraft/geometry/FurnitureGenerator.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr float kEps = 1e-3f;

bool approxEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int runFurnitureTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [Furniture] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M2-18: Sinh và kiểm tra toàn bộ 6 loại nội thất (5 bắt buộc + 1 Curved Chair Bézier Surface)
    roomcraft::geometry::FurnitureModel desk, chair, bookshelf, lamp, vase, curvedChair;
    std::string err;

    const bool okDesk = roomcraft::geometry::FurnitureGenerator::createDesk({}, desk, &err);
    const bool okChair = roomcraft::geometry::FurnitureGenerator::createChair({}, chair, &err);
    const bool okBookshelf = roomcraft::geometry::FurnitureGenerator::createBookshelf({}, bookshelf, &err);
    const bool okLamp = roomcraft::geometry::FurnitureGenerator::createDeskLamp({}, lamp, &err);
    const bool okVase = roomcraft::geometry::FurnitureGenerator::createVase({}, vase, &err);
    const bool okCurved = roomcraft::geometry::FurnitureGenerator::createCurvedChair({}, curvedChair, &err);

    const std::vector<const roomcraft::geometry::FurnitureModel*> allModels = {
        &desk, &chair, &bookshelf, &lamp, &vase, &curvedChair
    };

    bool allStandOnGround = okDesk && okChair && okBookshelf && okLamp && okVase && okCurved;
    for (const auto* m : allModels) {
        if (!m->validate(&err)) {
            allStandOnGround = false;
        }
        // Quy ước: mọi món nội thất đứng trên mặt sàn y = 0 (pivot tại tâm đáy)
        if (!approxEqual(m->boundingBox.min.y, 0.0f, 1e-3f)) {
            allStandOnGround = false;
        }
        const glm::vec3 sz = m->boundingBox.getSize();
        std::cout << "    [Stats] " << m->name
                  << " | parts=" << m->parts.size()
                  << " | verts=" << m->getTotalVertexCount()
                  << " | indices=" << m->getTotalIndexCount()
                  << " | tris=" << m->getTotalTriangleCount()
                  << " | size=(" << sz.x << "x" << sz.y << "x" << sz.z << "m)" << std::endl;
    }

    check("TC-M2-18 All 6 procedural furniture models valid & resting on ground y=0", allStandOnGround);

    // TC-M2-17: Kiểm tra phép biến đổi phân cấp M_world_part = M_parent * M_local_part
    {
        roomcraft::scene::Transform parentTransform({3.0f, 0.0f, -2.0f}, {0.0f, 90.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        const glm::mat4 parentMat = parentTransform.getModelMatrix();

        roomcraft::math::BoundingBox mergedWorldBox;
        for (const auto& part : desk.parts) {
            const glm::mat4 partWorldMat = parentMat * part.localTransform.getModelMatrix();
            mergedWorldBox.expand(part.meshData.boundingBox.transformed(partWorldMat));
        }

        const glm::vec3 expectedSize(desk.boundingBox.getSize().z,
                                     desk.boundingBox.getSize().y,
                                     desk.boundingBox.getSize().x);
        const glm::vec3 actualSize = mergedWorldBox.getSize();
        const glm::vec3 center = mergedWorldBox.getCenter();

        check("TC-M2-17 Hierarchy M_parent * M_local propagates accurately to all desk parts",
              glm::length(actualSize - expectedSize) < 1e-3f &&
              approxEqual(center.x, 3.0f, 1e-2f) &&
              approxEqual(center.z, -2.0f, 1e-2f) &&
              approxEqual(mergedWorldBox.min.y, 0.0f, 1e-3f));
    }

    // TC-M2-15: Tham số nội thất lỗi bị từ chối sạch
    {
        roomcraft::geometry::FurnitureModel bad;
        roomcraft::geometry::DeskParams badDesk;
        badDesk.width = -1.0f;
        roomcraft::geometry::BookshelfParams badShelf;
        badShelf.shelfCount = 1;
        roomcraft::geometry::VaseParams badVase;
        badVase.bellyRadius = -0.2f;

        const bool rejected = !roomcraft::geometry::FurnitureGenerator::createDesk(badDesk, bad, &err) &&
                              !roomcraft::geometry::FurnitureGenerator::createBookshelf(badShelf, bad, &err) &&
                              !roomcraft::geometry::FurnitureGenerator::createVase(badVase, bad, &err);
        check("TC-M2-15 Invalid furniture parameters rejected cleanly", rejected);
    }

    return failures;
}
