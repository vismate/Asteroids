#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdarg>
#include <bitset>

/*
TODO: Clean this up
TODO: Documentation
*/

#ifndef DEFAULT_DATETIME_FORMAT
#define DEFAULT_DATETIME_FORMAT "[%Y-%m-%d %H:%M:%S]"
#endif

namespace Log
{
    enum class Color
    {
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
        Default
    };

    enum class Flag
    {
        Ansi,
        Tag,
        Datetime,
        Enabled,
        SuccesIfDisabled,
        SuccessIfHidden
    };

    static size_t GLOBAL_LOG_LEVEL{0};

    class Logger
    {

    public:

        Logger() = default;
        Logger(const char *tag, Color color, size_t log_level) : tag(tag), color(color), log_level(log_level)
        {
        }

        operator size_t() { return log_level; }

        template <typename T>
        inline auto operator()(const T &e) const -> bool
        {
            if (not get_flag(Flag::Enabled))
                return get_flag(Flag::SuccesIfDisabled);

            if (log_level < GLOBAL_LOG_LEVEL)
                return get_flag(Flag::SuccessIfHidden);

            if (get_flag(Flag::Ansi))
                *stream << color_code(color);

            if (get_flag(Flag::Datetime))
            {
                auto time = std::time(nullptr);
                *stream << std::put_time(std::localtime(&time), datetime_format.c_str()) << ' ';
            }

            if (get_flag(Flag::Tag))
                *stream << "[" << tag << "] ";

            *stream << e << std::endl;

            if (get_flag(Flag::Ansi))
                *stream << color_code(Color::Default);

            return stream->good();
        }
        inline auto set_flag(Flag flag, bool value) -> Logger &
        {
            flags[static_cast<size_t>(flag)] = value;
            return *this;
        }
        inline auto set_log_level(size_t log_level) -> Logger &
        {
            this->log_level = log_level;
            return *this;
        }
        inline auto set_tag(const std::string &tag) -> Logger &
        {
            this->tag = tag;
            return *this;
        }
        inline auto set_datetime_format(const std::string &format) -> Logger &
        {
            this->datetime_format = format;
            return *this;
        }
        inline auto set_stream(std::ostream &stream) -> Logger &
        {
            this->stream = &stream;
            return *this;
        }
        inline auto set_color(Color color) -> Logger &
        {
            this->color = color;
            return *this;
        }

    private:
        std::string tag{"LOG"};
        std::string datetime_format{DEFAULT_DATETIME_FORMAT};

        Color color{Color::Default};
        std::ostream *stream{&std::clog};

        std::bitset<6> flags{0xffffffffUL};

        size_t log_level{0};

        inline auto get_flag(Flag flag) const -> bool
        {
            return flags[static_cast<size_t>(flag)];
        }

        static constexpr inline auto color_code(Color color) -> const char *
        {
            switch (color)
            {
            case Color::Black:
                return "\u001b[30m";
            case Color::Red:
                return "\u001b[31m";
            case Color::Green:
                return "\u001b[32m";
            case Color::Yellow:
                return "\u001b[33m";
            case Color::Blue:
                return "\u001b[34m";
            case Color::Magenta:
                return "\u001b[35m";
            case Color::Cyan:
                return "\u001b[36m";
            case Color::White:
                return "\u001b[37m";
            default:
                return "\u001b[0m";
            }
        }
    };

    static Logger info("INFO", Color::Default, 10);
    static Logger debug("DEBUG", Color::Green, 20);
    static Logger warn("WARNING", Color::Yellow, 30);
    static Logger error("ERROR", Color::Red, 40);

    template <size_t BuffSize = 1024>
    inline auto format(const char *fmt, ...) -> std::string
    {
        char buff[BuffSize];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buff, BuffSize, fmt, args);
        va_end(args);
        return std::string{buff};
    }

}

#endif