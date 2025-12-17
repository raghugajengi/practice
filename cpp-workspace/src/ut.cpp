#include <iostream>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <source_location>


class u_hash {
private: // Constants
    static const uint64_t FNV_prime = 1099511628211ULL;
    static const uint64_t FNV_offset = 14695981039346656037ULL;
public: // Constants
    static const uint64_t initial = FNV_offset;
public: // Methods:
    // Hash a buffer of a given len.
    // This function is additive, i.e., can be used like so:
    //   uint64_t r;
    //   r = hash(buf1, len1);   // Note: don't pass in an r==0, or else the
    //                           //       hash of a buf of 0s will be a 0.
    //                           //       You can pass u_hash::initial.
    //   r = hash(buf2, len2, r);
    //   r = hash(buf3, len3, r);
    //   // etc.
    //
    static uint64_t
    hash(const unsigned char* buf, size_t len, uint64_t r = initial)
    {
        while (len > 0) {
            r ^= (uint64_t)*buf++;
            r *= FNV_prime;
            --len;
        }
        return r;
    }
    // Generalization: allow calls to hash() with objects of any type
    template <typename T>
    static uint64_t
    hash(const T* buf, uint32_t count = 1, uint64_t r = initial)
    {
        return hash(reinterpret_cast<const unsigned char*>(buf), sizeof(T) * count, r);
    }

}; // u_hash

template <size_t N>
constexpr uint32_t compute_fnv_hash_of_label(const char (&str)[N]) {
    return u_hash::hash(str); // Exclude null terminator
}

#define STR(s) #s
#define ADD_LINE() (__FILE__ ":" STR(__LINE__))
//#define ADD_LINE() std::source_location::current_file_name() << ":" << std::source_location::current_line()
    
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
          format_hash(compute_fnv_hash_of_label(ADD_LINE())),
          n(0)
    {
        std::cout << "in constructor" <<label;
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
        static UTtr ttr(arg_msg_limit, arg_time_interval, "" fmt);                        \
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
      u_tr(T_dbg, "1. Starting writer thread %u", ab);
      u_tr(T_dbg, "2. Starting writer thread %u", ab);

     return 0;
}