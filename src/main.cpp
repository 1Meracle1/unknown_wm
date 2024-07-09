#include "window_manager.h"
#include <cstdlib>

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  INFO("Initializing window manager...");
  auto wm = WindowManager::Init();
  if (!wm) {
    ERROR("Failed to initialize window manager");
    return EXIT_FAILURE;
  }
  INFO("Starting window manager...");
  wm->Run();
}
