function(add_test target)
    add_executable(${target} ${target}.cc)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../extern/googletest/googletest/include)
    target_link_libraries(${target} PRIVATE gtest gtest_main)
endfunction()

add_test(test_4state)
