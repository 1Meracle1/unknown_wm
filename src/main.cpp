#include "logging.h"
#include "window_manager.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  // do cleanup of zombie processes
  // {
  //   struct sigaction sa = {};
  //   sigemptyset(&sa.sa_mask);
  //   sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
  //   sa.sa_handler = SIG_IGN;
  //   sigaction(SIGCHLD, &sa, nullptr);
  //   while (waitpid(-1, nullptr, WNOHANG) > 0)
  //     ;
  // }

  auto wm = WindowManager::Init();
  if (!wm.IsValid()) {
    ERROR("Failed to initialize window manager");
    return 1;
  }
  wm.Run();
}
