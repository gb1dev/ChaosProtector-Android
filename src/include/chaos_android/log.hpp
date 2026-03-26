#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include "chaos_android/config.hpp"

namespace chaos_android {

#ifdef CHAOS_ANDROID_DEBUG
#define SDEBUG(...) chaos_android::Logger::debug(__VA_ARGS__)
#else
#define SDEBUG(...)
#endif
#define STRACE(...) chaos_android::Logger::trace(__VA_ARGS__)
#define SINFO(...) chaos_android::Logger::info(__VA_ARGS__)
#define SWARN(...) chaos_android::Logger::warn(__VA_ARGS__)
#define SERR(...) chaos_android::Logger::err(__VA_ARGS__)

enum class LogLevel {
  Debug,
  Trace,
  Info,
  Warn,
  Err,
};

class Logger {
public:
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  template <typename... Ts>
  static void trace(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->trace(Fmt, Args...);
  }

  template <typename... Ts>
  static void debug(const char *Fmt, const Ts &...Args) {
#ifdef CHAOS_ANDROID_DEBUG
    CurrentOrDefault()->debug(Fmt, Args...);
#endif
  }

  template <typename... Ts>
  static void info(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->info(Fmt, Args...);
  }

  template <typename... Ts>
  static void warn(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->warn(Fmt, Args...);
  }

  template <typename... Ts>
  static void err(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->error(Fmt, Args...);
  }

  static void SetLevel(spdlog::level::level_enum L);
  static void set_level(spdlog::level::level_enum L) { SetLevel(L); }
  static void set_level(LogLevel L);

  static void BindModule(const std::string &Module, const std::string &Arch);
  static std::shared_ptr<spdlog::logger> CurrentOrDefault();

private:
  Logger();
  spdlog::level::level_enum Level = spdlog::level::debug;

  // Default sink used before any thread binds a module.
  std::shared_ptr<spdlog::logger> Default;

  // Per-thread bound module sink.
  static thread_local std::shared_ptr<spdlog::logger> Current;

public:
  static Logger &Instance();
};

class ThreadLoggerGuard {
public:
  ThreadLoggerGuard() = default;

  ~ThreadLoggerGuard() {
    auto Current = Logger::Instance().CurrentOrDefault();
    if (Current)
      Current->flush();
  }

  ThreadLoggerGuard(const ThreadLoggerGuard &) = delete;
  ThreadLoggerGuard &operator=(const ThreadLoggerGuard &) = delete;
};

} // end namespace chaos_android
