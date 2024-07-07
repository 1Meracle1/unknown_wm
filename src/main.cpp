#include "window_manager.h"
#include <cstdlib>
#include <spdlog/spdlog.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  spdlog::info("Initializing stuff...");

  auto wm = WindowManager::Init();
  if (wm == nullptr) {
    spdlog::error("Failed to initialize window manager");
    return EXIT_FAILURE;
  }
  spdlog::info("Starting window manager...");
  wm->Run();
}
