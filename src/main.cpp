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
            config.title = "RoomCraft 3D - M2 Procedural Geometry GPU Smoke Test";
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "RoomCraft 3D (M2 Procedural Geometry & Modeling)\n"
                      << "Usage: RoomCraft3D [option]\n"
                      << "  (no option)      Run interactive M2 Geometry Playground\n"
                      << "  --smoke-test     Run M0 regression smoke test\n"
                      << "  --m1-smoke-test  Run M1 regression GPU integration smoke test\n"
                      << "  --m2-smoke-test  Run M2 procedural geometry GPU integration smoke test\n";
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
