add_library(
    process
    process.cc
    message.cc
)
target_compile_options(
    process
    PUBLIC
    -Wall -Wextra -Werror -Wshadow -Wpedantic -O2 -std=c++17
)
target_include_directories(
    process
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
)
add_executable(
    logical-clocks.out
    main.cc
)
target_link_libraries(
    logical-clocks.out
    PUBLIC
    process
)
set_target_properties(
    logical-clocks.out
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
