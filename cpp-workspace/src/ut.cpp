#include <iostream>
#include <cstdarg>
#include <cstdint>
#include <cstring>

constexpr uint32_t fnv1a_32(const char* str, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint32_t>(str[i]);
        hash *= 16777619u;
    }
    return hash;
}

template <size_t N>
constexpr uint32_t compute_fnv_hash_of_label(const char (&str)[N]) {
    return fnv1a_32(str, N - 1); // Exclude null terminator
}

struct UTtr {
    int msg_limit;
    double time_interval;
    const char *label;
    uint32_t format_hash;
    int n;
    // ... stopwatch and other members ...

    // Templated constructor for string literals
    template <size_t N>
    UTtr(int msg_limit, double in_time_interval, const char (&label)[N])
        : msg_limit(msg_limit),
          time_interval(in_time_interval),
          label(label),
          format_hash(compute_fnv_hash_of_label(label)),
          n(0)
    {}

    // Fallback for runtime strings
    UTtr(int msg_limit, double in_time_interval, const char *label) 
        : msg_limit(msg_limit),
          time_interval(in_time_interval),
          label(label),
          format_hash(label ? fnv1a_32(label, strlen(label)) : 0),
          n(0)
    {
        std::cout << label;
    }
};


void
u_tr_log(int module, int level, const char *func, const char *fmt, va_list ap)
{
    char buffer[1024]; // Adjust the size as needed
    vsprintf(buffer, fmt, ap);
    std::cout << "In function: " << buffer << std::endl;
}


/* This external function logs the message after checking whether it should be logged. */
static inline
void
u_tr_check(int module, int level, const char *func, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    u_tr_log(module, level, func, fmt, ap);
    va_end(ap);
}


#define u_tr_mod(module, level, fmt, ...)                               \
    do {                                                                \
        u_tr_check((module), (level), __FUNCTION__, (fmt), ## __VA_ARGS__); \
    } while (0)

#define u_ntr_mod(module, level, fmt, ...)                      \
    u_tr_mod((module), (level), (fmt), ## __VA_ARGS__)


#define u_ttr_st(module, level, ttr, fmt, ...)                          \
    do {                                                                \
        u_ntr_mod(module, level, "%u" fmt, ttr.format_hash,## __VA_ARGS__);              \
    } while (0)

#define u_ttr_detail_module(module, level, arg_msg_limit, arg_time_interval, fmt, ...) \
    do {                                                                               \
        static UTtr ttr(arg_msg_limit, arg_time_interval, fmt);                        \
        u_ttr_st(module, level, ttr, fmt, ## __VA_ARGS__);                             \
    } while (0)

#define U_TTR_DEFAULT_NUM_MSGS_BEFORE_QUIET  1000
#define U_TTR_DEFAULT_TIME_INTERVAL          20.0
#define __MODULE__ 1000
#define u_ttr_module(module, level, ...)                       \
    u_ttr_detail_module((module), level,                       \
                 U_TTR_DEFAULT_NUM_MSGS_BEFORE_QUIET,          \
                 U_TTR_DEFAULT_TIME_INTERVAL,                  \
                 ## __VA_ARGS__)

// Issue a trace message by explicitly specifying module id
#define u_tr_module(module, level, fmt, ...)                    \
    u_ttr_module((module), level, fmt, ## __VA_ARGS__);         \
    
#define u_tr(level, fmt, ...)                                   \
    u_tr_module(__MODULE__, level, fmt, ## __VA_ARGS__)

#define T_dbg 1
    
int main(){
     uint32_t ab = 10;

      u_tr(T_dbg, "Starting writer thread %u", ab);

     return 0;
}