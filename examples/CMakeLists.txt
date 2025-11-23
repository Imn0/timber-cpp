if(BUILD_CPP_EXAMPLES)
    function(add_cpp_example EXAMPLE_NAME)
        add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)
        target_link_libraries(01-default_logger PRIVATE timber-cpp::timber-cpp)
        set_target_properties(${EXAMPLE_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        target_compile_options(${EXAMPLE_NAME} PRIVATE
            $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:
            -Wno-initializer-overrides>
        )
    endfunction(add_cpp_example example_name)

    add_cpp_example(01-default_logger)

endif()
