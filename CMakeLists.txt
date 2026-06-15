cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

project(ProxyServer)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Put build artifacts into per-configuration folders (bin, lib, etc.) ---
# For single-config generators (MinGW Makefiles, Ninja single-config), use CMAKE_BUILD_TYPE.
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(DEFINED CMAKE_BUILD_TYPE)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    endif()
else()
    # Multi-config generators (Visual Studio, Xcode): set per-config variables
    foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${config} _cfg)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_BINARY_DIR}/bin)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_BINARY_DIR}/lib)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_BINARY_DIR}/lib)
    endforeach()
endif()

add_executable(${PROJECT_NAME} 
                src/main.cpp
                src/endpoint_parser.cpp
                src/proxy_server.cpp
                src/http_proxy.cpp)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(spdlog CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system iphlpapi ws2_32 spdlog::spdlog)


# --- Post-build: copy MSYS2 (ucrt64) runtime DLLs to the output directory ---
# Allow overriding the MSYS2 ucrt64 bin directory from the command line:
if(NOT DEFINED MSYS2_UCRT64_BIN)
    set(MSYS2_UCRT64_BIN "D:/msys64/ucrt64/bin")
endif()

# DLLs commonly required by MinGW/ucrt64 toolchain
set(MSYS2_DLLS
    libgcc_s_seh-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
)

# Collect only the DLLs that actually exist on this machine, then add post-build copy
set(_msys2_found_dlls)
foreach(_dll IN LISTS MSYS2_DLLS)
    set(_src "${MSYS2_UCRT64_BIN}/${_dll}")
    if(EXISTS "${_src}")
        list(APPEND _msys2_found_dlls "${_src}")
    endif()
endforeach()

if(_msys2_found_dlls)
    foreach(_src IN LISTS _msys2_found_dlls)
        get_filename_component(_dllname "${_src}" NAME)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${_dllname}"
            COMMENT "Copy ${_dllname} to output directory"
        )
    endforeach()
    message(STATUS "Will copy MSYS2 DLLs: ${_msys2_found_dlls}")
else()
    message(STATUS "No MSYS2 ucrt64 DLLs found at ${MSYS2_UCRT64_BIN}; skipping copy step.")
endif()