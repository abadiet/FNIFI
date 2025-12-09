cmake_minimum_required(VERSION 3.10)

# Options
option(BUILD_UML "Build UML" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(FNIFI_DEBUG "Debug mode" OFF)
option(ENABLE_SAMBA
    "Use Samba for SMB implementation (see https://www.samba.org)" OFF)
option(ENABLE_LIBSMB2
    "Use libsmb2 for SMB implementation (see https://github.com/sahlberg/libsmb2)"
    OFF)
option(ENABLE_OPENCV "Use OpenCV for building preview images" ON)
option(ENABLE_EXIV2 "Use Exiv2 for metadata exfiltration" ON)
if(ENABLE_SAMBA AND ENABLE_LIBSMB2)
    message(FATAL_ERROR
        "ENABLE_SAMBA and ENABLE_LIBSMB2 option flags cannot be true both at the same time"
    )
endif()
if(BUILD_EXAMPLES AND NOT (ENABLE_SAMBA OR ENABLE_LIBSMB2))
    message(FATAL_ERROR
        "One of ENABLE_SAMBA or ENABLE_LIBSMB2 option flags should be true to compile examples"
    )
endif()

# Project
project(fnifi)
set(FNIFI_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/DirectoryIterator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IConnection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Collection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Relative.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FNIFI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SyncDirectory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Local.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IFileHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/File.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TempFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DiskBacked.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConnectionBuilder.cpp)
if(ENABLE_SAMBA)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/SMB-Samba.cpp)
elseif(ENABLE_LIBSMB2)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/SMB-libsmb2.cpp)
endif()
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${FNIFI_INCLUDE})

# Flags
set(SXEVAL_FLAGS -Wno-deprecated-redundant-constexpr-static-def)
set(LIBSMB2_FLAGS -Wno-reserved-macro-identifier -Wno-zero-length-array)
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
    -Wno-padded -Wno-switch-default -Wno-unsafe-buffer-usage-in-libc-call
    -Wno-exit-time-destructors -Wno-global-constructors ${SXEVAL_FLAGS}
    ${LIBSMB2_FLAGS}>
    $<$<CXX_COMPILER_ID:GNU>:-O3 -Wall -Wextra -Werror -Wfloat-equal
    -Wundef -Wcast-align -Wwrite-strings -Wconversion
    -Wunreachable-code -Wpedantic -Wshadow -Wwrite-strings
    -Wcast-qual -Wstrict-aliasing -Wpointer-arith -Wformat=2
    -Wimplicit-fallthrough -Wno-padded -Wno-unsafe-buffer-usage-in-libc-call
    -Wno-exit-time-destructors -Wno-global-constructors ${SXEVAL_FLAGS}
    ${LIBSMB2_FLAGS}>
)
if(FNIFI_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -DFNIFI_DEBUG)
endif()
if(ENABLE_SAMBA)
    target_compile_options(${PROJECT_NAME} PRIVATE -DENABLE_SAMBA)
elseif(ENABLE_LIBSMB2)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libs")
    target_compile_options(${PROJECT_NAME} PRIVATE -DENABLE_LIBSMB2)
endif()
if(ENABLE_OPENCV)
    target_compile_options(${PROJECT_NAME} PRIVATE -DENABLE_OPENCV)
endif()
if(ENABLE_EXIV2)
    target_compile_options(${PROJECT_NAME} PRIVATE -DENABLE_EXIV2)
endif()

# Libraries
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SXEval)
target_link_libraries(${PROJECT_NAME} PUBLIC sxeval)
if(ENABLE_EXIV2)
    find_package(exiv2 REQUIRED CONFIG NAMES exiv2)
    target_link_libraries(${PROJECT_NAME} PUBLIC Exiv2::exiv2lib)
endif()
if(ENABLE_OPENCV)
    find_package(OpenCV REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
endif()
if(ENABLE_SAMBA)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SMBCLIENT REQUIRED IMPORTED_TARGET smbclient)
    target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::SMBCLIENT)
elseif(ENABLE_LIBSMB2)
    set(ENABLE_LIBKRB5 OFF CACHE BOOL "Disable libkrb5 support")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libsmb2)
    target_link_libraries(${PROJECT_NAME} PUBLIC smb2)
endif()

# UML
if(BUILD_UML)
    set(UML_OUTPUTS "")
    file(GLOB UML_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/uml/*.plantuml)
    foreach(UML_FILE ${UML_SOURCES})
        message(STATUS "Adding UML file: ${UML_FILE}")
        get_filename_component(UML_NAME ${UML_FILE} NAME_WE)
        set(UML_TMP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/uml/${UML_NAME}.svg)
        set(UML_OUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/${UML_NAME}.svg)
        add_custom_command(
            OUTPUT ${UML_OUT_FILE}
            COMMAND plantuml ${UML_FILE} --svg && mv ${UML_TMP_FILE}
                ${UML_OUT_FILE}
            DEPENDS ${UML_FILE}
            COMMENT "Generating UML diagram: ${UML_NAME}.svg"
            VERBATIM
        )
        list(APPEND UML_OUTPUTS ${UML_OUT_FILE})
    endforeach()
    add_custom_target(generate_uml ALL
        DEPENDS ${UML_OUTPUTS}
    )
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()
