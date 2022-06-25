add_library(logic util.cc)
target_include_directories(logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set_property(TARGET logic PROPERTY POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(logic PRIVATE /W4 /WX)
else()
    target_compile_options(logic PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unknown-attributes)
endif()

# only for windows and clang
if (WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    target_link_libraries(logic PRIVATE clang_rt.builtins-x86_64.lib)
endif()
