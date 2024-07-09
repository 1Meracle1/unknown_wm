#pragma once

#include <spdlog/spdlog.h>

#define ERROR(msg) spdlog::error((msg))
#define INFO(msg) spdlog::info((msg))
#ifdef NDEBUG
#define DEBUG(msg)
#else
#define DEBUG(msg) spdlog::debug((msg))
#endif

#define ERRORF(msg, ...) spdlog::error((msg), __VA_ARGS__)
#define INFOF(msg, ...) spdlog::info((msg), __VA_ARGS__)
#ifdef NDEBUG
#define DEBUGF(msg, ...)
#else
#define DEBUGF(msg, ...) spdlog::debug((msg), __VA_ARGS__)
#endif
