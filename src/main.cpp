#include "roomcraft/core/Application.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
    roomcraft::core::ApplicationConfig config;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--smoke-test" || arg == "--m0-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM0;
            config.title = "RoomCraft 3D - M0 Regression Smoke Test";
        } else if (arg == "--m1-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM1;
            config.title = "RoomCraft 3D - M1 Regression Smoke Test";
        } else if (arg == "--m2-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM2;
            config.title = "RoomCraft 3D - M2 Regression Smoke Test";
        } else if (arg == "--m3-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM3;
            config.title = "RoomCraft 3D - M3 Regression Smoke Test";
        } else if (arg == "--m4-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM4;
            config.title = "RoomCraft 3D - M4 Regression Smoke Test";
        } else if (arg == "--m4-benchmark") {
            config.mode = roomcraft::core::ApplicationMode::BenchmarkM4;
            config.title = "RoomCraft 3D - M4 Performance Benchmark (10/30/100 Objects)";
        } else if (arg == "--m5-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM5;
            config.title = "RoomCraft 3D - M5 Regression Smoke Test";
        } else if (arg == "--m6-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM6;
            config.title = "RoomCraft 3D - M6 Professional UI/UX, Thumbnails & Gizmo Smoke Test";
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "RoomCraft 3D v0.6.0 — 3D Interior Design Studio\n"
                      << "Usage: RoomCraft3D [option]\n"
                      << "  (no option)      Launch RoomCraft 3D Interactive Editor\n"
                      << "  --smoke-test     Run M0 regression smoke test\n"
                      << "  --m1-smoke-test  Run M1 regression GPU integration smoke test\n"
                      << "  --m2-smoke-test  Run M2 regression GPU integration smoke test\n"
                      << "  --m3-smoke-test  Run M3 regression GPU integration smoke test\n"
                      << "  --m4-smoke-test  Run M4 regression GPU integration smoke test\n"
                      << "  --m4-benchmark   Run M4 10/30/100 furniture object GPU performance benchmark\n"
                      << "  --m5-smoke-test  Run M5 regression GPU integration smoke test\n"
                      << "  --m6-smoke-test  Run M6 UI/UX, Thumbnails, Move Gizmo & Performance smoke test\n";
            return 0;
        }
    }

    roomcraft::core::Application app(config);
    if (!app.initialize(argc, argv)) {
        std::cerr << "[RoomCraft3D][FATAL] Application initialization failed." << std::endl;
        return 1;
    }

    return app.run();
}
