#include <cstdint>
#include <cstddef>
#include <cstring>
#include <atomic>
#include <cstdio>
#include <string>
#include <optional>
#include <array>
#include <iostream>
#include <thread>
#include <functional>
#include <utility>

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

// Registry of compile-time hashes and their original strings
struct HashEntry {
    uint32_t hash;
    const char* str;
};

// Build registry with known string literals (expand as needed)
constexpr std::array<HashEntry, 2> HASH_REGISTRY = {{
    { compute_fnv_hash_of_label("My log message %d"), "My log message %d" },
    { compute_fnv_hash_of_label("NK: 66 [Thread %s] Processing removal "), "NK: 66 [Thread %s] Processing removal " },
}};

// Unhash function: given a hash, return the original string literal (or nullptr if not found)
constexpr std::optional<const char*> unhash(uint32_t hash) {
    for (const auto& entry : HASH_REGISTRY) {
        if (entry.hash == hash) {
            return entry.str;
        }
    }
    return std::nullopt;
}

struct UTtr {
    int msg_limit;
    double time_interval;
    const char *label;
    uint32_t format_hash;
    std::atomic<int> n;
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

// Simple log level identifiers (expand as needed)
enum LogLevel {
    T_info = 1,
    T_warn = 2,
    T_error = 3
};

// --- u_tr implementation ---
// Internal implementation for array (string literal) formats
template <size_t N, typename... Args>
void u_tr_impl_array(int level, const char (&fmt)[N], Args&&... args) {
    static UTtr ttr(100 /*msg_limit*/, 1.0 /*interval*/, fmt);

    char buf[1024];
    int n = std::snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
    if (n < 0) {
        std::fprintf(stderr, "format error\n");
        return;
    }
    std::printf("raghu compile [L%d][hash=%u] %s\n", level, ttr.format_hash, buf);
}

// Internal implementation for pointer/runtime formats
template <typename... Args>
void u_tr_impl_ptr(int level, const char *fmt, Args&&... args) {
    static UTtr ttr(100 /*msg_limit*/, 1.0 /*interval*/, fmt);

    char buf[1024];
    int n = std::snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
    if (n < 0) {
        std::fprintf(stderr, "format error\n");
        return;
    }
    std::printf("raghu [L%d][hash=%u] %s\n", level, ttr.format_hash, buf);
}

// Dispatcher: choose array (literal) or pointer implementation based on the format argument type
template <typename Fmt, typename... Args>
void u_tr(int level, Fmt&& fmt, Args&&... args) {
    if constexpr (std::is_array_v<std::remove_reference_t<Fmt>>) {
        u_tr_impl_array(level, std::forward<Fmt>(fmt), std::forward<Args>(args)...);
    } else {
        // Ensure we pass a const char* to the impl
        u_tr_impl_ptr(level, std::forward<Fmt>(fmt), std::forward<Args>(args)...);
    }
}


void example(const char* str, int) {
    // This uses the templated constructor, so format_hash is computed at compile time!
    static UTtr ttr(10, 5.0, str);
    constexpr uint32_t CT_HASH = compute_fnv_hash_of_label("NK: 66 [Thread %s] Processing removal ");

    printf("Hash of format string: %u\n", ttr.format_hash); // Prints the hash value
    
    // Unhash: recover the original string from the hash
    if (auto original = unhash(ttr.format_hash)) {
        printf("Unhashed string: %s\n", original.value());
    } else {
        printf("Hash not found in registry\n");
    }
    
    // runtime check (will abort on failure)
    if (ttr.format_hash != CT_HASH) {
        std::fprintf(stderr, "format_hash differs: runtime=%u compile-time=%u\n",
                     ttr.format_hash, CT_HASH);
        
    }
}

/*int main() {
    int a =10;

    u_tr(T_info, "NK: 66 [Thread %s] Processing removal ",
     std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())).c_str());

    example("NK: 66 [Thread %s] Processing removal " , a); // creates static UTtr ttr
    
    return 0;
}*/