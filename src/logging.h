#pragma once

#include <source_location>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#define ERROR(...)                                                             \
  SPDLOG_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)

#define INFO(...) SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)

#ifdef NDEBUG
#define DEBUG(msg)
#else
#define DEBUG(msg) spdlog::debug((msg))
#endif
#ifdef NDEBUG
#define DEBUGF(msg, ...)
#else
#define DEBUGF(msg, ...) spdlog::debug((msg), __VA_ARGS__)
#endif
