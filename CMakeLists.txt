cmake_minimum_required(VERSION 3.10)

# Options
option(BUILD_UML "Build UML" OFF)
option(FNIFI_DEBUG "Debug mode" OFF)

# Project
project(fnifi)
set(FNIFI_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${FNIFI_INCLUDE})

# Flags
target_compile_options(${PROJECT_NAME} PRIVATE
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
    -Wno-padded>
    $<$<CXX_COMPILER_ID:GNU>:-O3 -Wall -Wextra -Werror -Wfloat-equal
    -Wundef -Wcast-align -Wwrite-strings -Wconversion
    -Wunreachable-code -Wpedantic -Wshadow -Wwrite-strings
    -Wcast-qual -Wstrict-aliasing -Wpointer-arith -Wformat=2
    -Wimplicit-fallthrough -Wno-padded>
)
if(FNIFI_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -DFNIFI_DEBUG)
endif()

# Download/Update librairies if needed
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Downloading git submodules...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SXEval")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE "
        "was turned off or failed. Please download submodules and try again.")
endif()

# Libraries
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SXEval)
target_link_libraries(${PROJECT_NAME} PRIVATE sxeval)

# UML
if(BUILD_UML)
    set(UML_OUTPUTS "")
    file(GLOB UML_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uml/*.plantuml)
    foreach(UML_FILE ${UML_SOURCES})
        message("Found UML file ${UML_FILE}")
        get_filename_component(UML_NAME ${UML_FILE} NAME_WE)
        set(UML_TMP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/uml/${UML_NAME}.png)
        set(UML_OUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/${UML_NAME}.png)
        add_custom_command(
            OUTPUT ${UML_OUT_FILE}
            COMMAND plantuml ${UML_FILE} && mv ${UML_TMP_FILE} ${UML_OUT_FILE}
            DEPENDS ${UML_FILE}
            COMMENT "Generating UML diagram: ${UML_NAME}.png"
            VERBATIM
        )
        list(APPEND UML_OUTPUTS ${UML_OUT_FILE})
    endforeach()
    add_custom_target(generate_uml ALL
        DEPENDS ${UML_OUTPUTS}
    )
endif()
