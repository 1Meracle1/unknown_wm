#pragma once

#include "connection.h"
#include <format>
#include <memory>
#include <vector>
#include <xcb/xcb.h>

class WindowManager {
public:
  static auto Init() -> std::unique_ptr<WindowManager>;
  void Run();

private:
  explicit WindowManager(std::unique_ptr<Connection> connection);

private:
  std::unique_ptr<Connection> connection_;
};
