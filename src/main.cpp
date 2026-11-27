#include "roomcraft/core/Application.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
    roomcraft::core::ApplicationConfig config;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--smoke-test") {
            config.smokeTestMode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "RoomCraft 3D (M0 Foundation)\n"
                      << "Usage: RoomCraft3D [--smoke-test]\n"
                      << "  --smoke-test  Run automated M0 verification (init, render, resize, clean exit)\n";
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
