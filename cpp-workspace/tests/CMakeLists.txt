include(FetchContent)

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(catch2)

add_executable(tests_main test_main.cpp)
target_link_libraries(tests_main PRIVATE Catch2::Catch2WithMain)
target_include_directories(tests_main PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME unit_tests COMMAND tests_main)
