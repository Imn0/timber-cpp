#ifndef TMB_CPP_HPP_
#define TMB_CPP_HPP_

#include <cstring>
#include <format>
#include <source_location>
#include <string_view>

#include <atomic> // very very important to include it BEFORE tmb.h :)

namespace tmb {

namespace c {
typedef std::atomic<int> IAmActuallyUsingTheAtomicHeader;
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
                               std::string_view fmt,
                               Args&&... args) {
    std::string msg;
    try {
        msg = std::vformat(fmt, std::make_format_args(args...));
    } catch (const std::format_error& e) {
        msg   = std::string("[format error] ") + e.what();
        level = LogLevel::Error;
    }
    log_default_logger_impl(level, loc, msg);
}

struct format_with_location {
    std::string_view value;
    std::source_location loc;

    template <typename String>
    format_with_location(const String& s,
                         const std::source_location& location =
                                 std::source_location::current()) :
        value { s }, loc { location } {}
};

} // namespace internal

class Logger {
  public:
    Logger(std::string_view name,
           const c::tmb_logger_cfg_t& cfg = {
                   .log_level     = c::TMB_LOG_LEVEL_DEBUG,
                   .enable_colors = true,
           }) {
        _logger = c::tmb_logger_create(name.data(), cfg);
        if (!_logger) { throw std::runtime_error("Failed to create logger"); }
        _name = std::string(name);
    }

    ~Logger() {
        if (_logger) {
            c::tmb_logger_destroy(_logger);
            _logger = nullptr;
        }
    }

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&& other) noexcept :
        _logger(other._logger), _name(std::move(other._name)) {
        other._logger = nullptr;
    }

    Logger& operator=(Logger&& other) noexcept {
        if (this != &other) {
            if (_logger) c::tmb_logger_destroy(_logger);
            _logger       = other._logger;
            _name         = std::move(other._name);
            other._logger = nullptr;
        }
        return *this;
    }

    template <typename... Args>
    void log(LogLevel level,
             const std::source_location& loc,
             std::string_view fmt,
             Args&&... args) {
        std::string msg;
        try {
            msg = std::vformat(fmt, std::make_format_args(args...));
        } catch (const std::format_error& e) {
            msg   = std::string("[format error] ") + e.what();
            level = LogLevel::Error;
        }
        log_impl(level, loc, msg);
    }

#define _tmb_ccp_LOG_LEVEL__(_m_name, _m_level)                                \
    template <typename... Args>                                                \
    void _m_name(internal::format_with_location fmt, Args&&... args) {         \
        log(_m_level, fmt.loc, fmt.value, std::forward<Args>(args)...);        \
    }

    _tmb_ccp_LOG_LEVEL__(fatal, LogLevel::Fatal);
    _tmb_ccp_LOG_LEVEL__(error, LogLevel::Error);
    _tmb_ccp_LOG_LEVEL__(warning, LogLevel::Warning);
    _tmb_ccp_LOG_LEVEL__(warn, LogLevel::Warning);
    _tmb_ccp_LOG_LEVEL__(info, LogLevel::Info);
    _tmb_ccp_LOG_LEVEL__(debug, LogLevel::Debug);
    _tmb_ccp_LOG_LEVEL__(trace, LogLevel::Trace);
#undef _tmb_ccp_LOG_LEVEL__

    bool set_default_format(const char* fmt) {
        return c::tmb_logger_set_default_format(this->_logger, fmt);
    }

  private:
    void log_impl(LogLevel level,
                  const std::source_location& loc,
                  std::string_view msg) {
        c::tmb_log(internal::LogContext(level, loc).to_c(),
                   _logger,
                   "%.*s",
                   static_cast<int>(msg.size()),
                   msg.data());
    }
    c::tmb_logger_t* _logger { nullptr };
    std::string _name;
};

// https://github.com/gabime/spdlog/issues/1959
#define _tmb_ccp_LOG_LEVEL__(_m_name, _m_level)                                \
    template <typename... Args>                                                \
    void _m_name(internal::format_with_location fmt, Args&&... args) {         \
        internal::log_default_logger(                                          \
                _m_level, fmt.loc, fmt.value, std::forward<Args>(args)...);    \
    }

_tmb_ccp_LOG_LEVEL__(fatal, LogLevel::Fatal);
_tmb_ccp_LOG_LEVEL__(error, LogLevel::Error);
_tmb_ccp_LOG_LEVEL__(warning, LogLevel::Warning);
_tmb_ccp_LOG_LEVEL__(warn, LogLevel::Warning);
_tmb_ccp_LOG_LEVEL__(info, LogLevel::Info);
_tmb_ccp_LOG_LEVEL__(debug, LogLevel::Debug);
_tmb_ccp_LOG_LEVEL__(trace, LogLevel::Trace);

inline bool set_default_format(const char* fmt) {
    return c::tmb_logger_set_default_format(c::tmb_get_default_logger(), fmt);
}

#undef _tmb_ccp_LOG_LEVEL__
} // namespace tmb

#endif // TMB_CPP_HPP_
