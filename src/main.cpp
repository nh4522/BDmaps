#include "Application.h"
#include <iostream>

int main() {
    try {
        Application app(1280, 720, "Bangladesh 3D District Map");
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
