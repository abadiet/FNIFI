cmake_minimum_required(VERSION 3.10)

set(SXEVAL_FLAGS -Wno-deprecated-redundant-constexpr-static-def)
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)
foreach(SUBDIR ${SUBDIRS})
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.hpp)
        if (SOURCES)
            string(REPLACE "/" "_" TARGET_NAME ${SUBDIR})
            message(STATUS "Adding example: ${TARGET_NAME}")

            # Executable
            add_executable(${TARGET_NAME} ${SOURCES})

            # Flags
            target_compile_options(${TARGET_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:AppleClang>:-O3 -Wall -Wextra -Werror
                -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wconversion
                -Wunreachable-code -Wpedantic -Wshadow -Wwrite-strings
                -Wcast-qual -Wstrict-aliasing -Wpointer-arith -Wformat=2
                -Weverything -Wextra-semi -Wmissing-prototypes
                -Wunreachable-code-aggressive -Wimplicit-fallthrough
                -Wloop-analysis -Wrange-loop-analysis -Wcomma -Wmove
                -Wconditional-uninitialized -Wassign-enum
                -Wunused-exception-parameter -Wcovered-switch-default
                -Wno-c++98-compat -Wno-c++98-compat-pedantic
                -Wno-padded ${SXEVAL_FLAGS}>
            $<$<CXX_COMPILER_ID:GNU>:-O3 -Wall -Wextra -Werror -Wfloat-equal
                -Wundef -Wcast-align -Wwrite-strings -Wconversion
                -Wunreachable-code -Wpedantic -Wshadow -Wwrite-strings
                -Wcast-qual -Wstrict-aliasing -Wpointer-arith -Wformat=2
                -Wimplicit-fallthrough -Wno-padded ${SXEVAL_FLAGS}>
            )

            # Dependencies
            target_link_libraries(${TARGET_NAME} PRIVATE fnifi)
        endif()
    endif()
endforeach()
