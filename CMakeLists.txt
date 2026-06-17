cmake_minimum_required(VERSION 3.27)

project(ProxyServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_executable(${PROJECT_NAME} 
                src/main.cpp
                src/endpoint_parser.cpp
                src/proxy_server.cpp
                src/http_proxy.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system spdlog::spdlog)
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE iphlpapi ws2_32)
endif()

# include(GNUInstallDirs)

# install(TARGETS ${PROJECT_NAME}
#     RUNTIME DESTINATION bin
# )

# install(RUNTIME_DEPENDENCIES
#     EXECUTABLES ${PROJECT_NAME}
#     PRE_EXCLUDE_REGEXES
#         "api-ms-.*"
#     POST_EXCLUDE_REGEXES
#         ".*system32.*"
#     DESTINATION bin
# )

# if(NOT DEFINED MSYS2_UCRT64_BIN)
#     set(MSYS2_UCRT64_BIN "D:/msys64/ucrt64/bin")
# endif()

# set(MSYS2_DLLS
#     libgcc_s_seh-1.dll
#     libstdc++-6.dll
#     libwinpthread-1.dll
# )

# set(_msys2_found_dlls)
# foreach(_dll IN LISTS MSYS2_DLLS)
#     set(_src "${MSYS2_UCRT64_BIN}/${_dll}")
#     if(EXISTS "${_src}")
#         list(APPEND _msys2_found_dlls "${_src}")
#     endif()
# endforeach()

# if(_msys2_found_dlls)
#     foreach(_src IN LISTS _msys2_found_dlls)
#         get_filename_component(_dllname "${_src}" NAME)
#         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${_dllname}"
#             COMMENT "Copy ${_dllname} to output directory"
#         )
#     endforeach()
#     message(STATUS "Will copy MSYS2 DLLs: ${_msys2_found_dlls}")
# else()
#     message(STATUS "No MSYS2 ucrt64 DLLs found at ${MSYS2_UCRT64_BIN}; skipping copy step.")
# endif()