#include <iostream>
#include <type_traits>

// Create unique static storage for each generated string
#define MAKE_STR_STORAGE(name, value) \
    static constexpr char name[] = value;

// Turn integer into string
#define STR(x) #x
#define XSTR(x) STR(x)

// Combine file + line into a single compile-time string
#define MAKE_COMBINED_STRING(file, line) file ":" line

// Main macro user calls
#define MAKE_SOURCE_INFO() \
([](){ \
    MAKE_STR_STORAGE(CONCAT_STORAGE(__LINE__), MAKE_COMBINED_STRING(__FILE__, XSTR(__LINE__))); \
    return SourceInfo<CONCAT_STORAGE(__LINE__)>(); \
})()

#define CONCAT_IMPL(a,b) a##b
#define CONCAT(a,b) CONCAT_IMPL(a,b)
#define CONCAT_STORAGE(line) CONCAT(_src_info_, line)

// Template storing a compile-time string
template<const char* str>
struct SourceInfo {
    using file = std::integral_constant<const char*, str>;

    void print() const {
        std::cout << "File: " << file::value << "\n";
    }
};

int main() {
    auto info = MAKE_SOURCE_INFO();
    info.print();
}
