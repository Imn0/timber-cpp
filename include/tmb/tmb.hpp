#ifndef TMB_CPP_HPP_
#define TMB_CPP_HPP_

#include <cstring>
#include <format>
#include <source_location>
#include <string_view>
namespace tmb {
namespace c {
extern "C" {
#include <tmb/tmb.h>
}
} // namespace c

enum class LogLevel : int {
    None    = TMB_LEVEL_NONE,
    Fatal   = TMB_LEVEL_FATAL,
    Error   = TMB_LEVEL_ERROR,
    Warning = TMB_LEVEL_WARNING,
    Info    = TMB_LEVEL_INFO,
    Debug   = TMB_LEVEL_DEBUG,
    Trace   = TMB_LEVEL_TRACE,
    All     = TMB_LEVEL_ALL
};

inline void print_version() {
    c::tmb_print_version();
}
inline const char* get_version() {
    return c::tmb_get_version();
}

namespace internal {

class LogContext {
  public:
    LogContext(
            LogLevel level,
            const std::source_location& loc = std::source_location::current()) :
        level_(level), location_(loc) {}

    c::tmb_log_ctx_t to_c() const noexcept {
        auto filename     = location_.file_name();
        auto funcname     = location_.function_name();
        auto filename_len = std::strlen(filename);
        auto funcname_len = std::strlen(funcname);

        const char* base = filename;
        for (const char* p = filename; *p; ++p) {
            if (*p == '/' || *p == '\\') { base = p + 1; }
        }
        auto base_len = filename_len - (base - filename);

        return c::tmb_log_ctx_t {
            .log_level         = static_cast<c::tmb_log_level>(level_),
            .line_no           = static_cast<int>(location_.line()),
            .filename          = filename,
            .filename_len      = static_cast<int>(filename_len),
            .filename_base     = base,
            .filename_base_len = static_cast<int>(base_len),
            .funcname          = funcname,
            .funcname_len      = static_cast<int>(funcname_len),
            .message           = nullptr,
            .message_len       = 0,
            .ts_sec            = 0,
            .ts_nsec           = 0,
            .stopwatch_sec     = 0,
            .stopwatch_nsec    = 0
        };
    }

  private:
    LogLevel level_;
    std::source_location location_;
};

inline void log_default_logger_impl(LogLevel level,
                                    const std::source_location& loc,
                                    std::string_view msg) {
    c::tmb_log_default(LogContext(level, loc).to_c(),
                       "%.*s",
                       static_cast<int>(msg.size()),
                       msg.data());
}
template <typename... Args>
inline void log_default_logger(LogLevel level,
                               const std::source_location& loc,
                               std::format_string<Args...> fmt,
                               Args&&... args) {
    auto msg = std::format(fmt, std::forward<Args>(args)...);
    log_default_logger_impl(level, loc, msg);
}
} // namespace internal

#define _tmb_ccp_LOG_LEVEL__(_m_name, _m_level)                                \
    template <typename... Args> struct _m_name {                               \
        _m_name(std::format_string<Args...> fmt,                               \
                Args&&... args,                                                \
                const std::source_location& loc =                              \
                        std::source_location::current()) {                     \
            internal::log_default_logger(                                      \
                    _m_level, loc, fmt, std::forward<Args>(args)...);          \
        }                                                                      \
    };                                                                         \
                                                                               \
    template <typename... Args>                                                \
    _m_name(std::format_string<Args...>, Args&&...)->_m_name<Args...>

_tmb_ccp_LOG_LEVEL__(fatal, LogLevel::Fatal);
_tmb_ccp_LOG_LEVEL__(error, LogLevel::Error);
_tmb_ccp_LOG_LEVEL__(warning, LogLevel::Warning);
_tmb_ccp_LOG_LEVEL__(warn, LogLevel::Warning);
_tmb_ccp_LOG_LEVEL__(info, LogLevel::Info);
_tmb_ccp_LOG_LEVEL__(debug, LogLevel::Debug);
_tmb_ccp_LOG_LEVEL__(trace, LogLevel::Trace);

} // namespace tmb

#endif // TMB_CPP_HPP_
