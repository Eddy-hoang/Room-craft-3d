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
            config.width = 1878;
            config.height = 815;
            config.title = "RoomCraft 3D - M4 Performance Benchmark (10/30/100 Objects)";
        } else if (arg == "--m5-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM5;
            config.title = "RoomCraft 3D - M5 Regression Smoke Test";
        } else if (arg == "--m6-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM6;
            config.title = "RoomCraft 3D - M6 Professional UI/UX, Thumbnails & Gizmo Smoke Test";
        } else if (arg == "--m8-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM8;
            config.title = "RoomCraft 3D - M8 Collision & Project Persistence GPU Smoke Test";
        } else if (arg == "--m9-smoke-test") {
            config.mode = roomcraft::core::ApplicationMode::SmokeTestM9;
            config.title = "RoomCraft 3D - M9 Shadow Mapping & Release Candidate GPU Smoke Test";
        } else if (arg == "--m9-benchmark") {
            config.mode = roomcraft::core::ApplicationMode::BenchmarkM9;
            config.width = 1280;
            config.height = 720;
            config.title = "RoomCraft 3D - M9 Performance Benchmark (1280x720)";
        } else if (arg == "--m9-capture-evidence") {
            config.mode = roomcraft::core::ApplicationMode::CaptureEvidenceM9;
            config.width = 1280;
            config.height = 720;
            config.title = "RoomCraft 3D — 3D Interior Design Studio";
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                config.outputDir = std::filesystem::u8path(argv[++i]);
            }
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "RoomCraft 3D v1.0.0-rc2 (OpenGL 3.3 Core Profile)\n";
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "RoomCraft 3D v1.0.0-rc2 — 3D Interior Design Studio\n"
                      << "Usage: RoomCraft3D [option]\n"
                      << "  (no option)              Launch RoomCraft 3D Interactive Editor\n"
                      << "  --version                Print application version\n"
                      << "  --smoke-test             Run M0 regression smoke test\n"
                      << "  --m1-smoke-test          Run M1 regression GPU integration smoke test\n"
                      << "  --m2-smoke-test          Run M2 regression GPU integration smoke test\n"
                      << "  --m3-smoke-test          Run M3 regression GPU integration smoke test\n"
                      << "  --m4-smoke-test          Run M4 regression GPU integration smoke test\n"
                      << "  --m4-benchmark           Run M4 10/30/100 furniture object GPU performance benchmark\n"
                      << "  --m5-smoke-test          Run M5 regression GPU integration smoke test\n"
                      << "  --m6-smoke-test          Run M6 UI/UX, Thumbnails, Move Gizmo & Performance smoke test\n"
                      << "  --m8-smoke-test          Run M8 Collision & Project Persistence GPU integration smoke test\n"
                      << "  --m9-smoke-test          Run M9 Shadow Mapping & Release Candidate GPU smoke test\n"
                      << "  --m9-benchmark           Run M9 reproducible CPU/GPU/Wall-clock benchmark suite\n"
                      << "  --m9-capture-evidence    Capture real GPU PNG screenshots & generate example projects\n";
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
