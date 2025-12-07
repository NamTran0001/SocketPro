cmake_minimum_required(VERSION 3.10)
project(NetworkingProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# For Windows and Visual Studio
if(WIN32 AND MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /wd4267 /FS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /wd4244")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Include project directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/core
)

# OpenCV configuration
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib") 
find_package(OpenCV REQUIRED) 

if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
    
    # Set OpenCV DLL path
		# Or C:/Evironment_Code/opencv/build/x64/vc16/bin
    set(OpenCV_BIN_DIR "C:/opencv/build/x64/vc16/bin")
    message(STATUS "OpenCV DLL directory: ${OpenCV_BIN_DIR}")
else()
    message(FATAL_ERROR "OpenCV not found!")
endif()

include_directories(${OpenCV_INCLUDE_DIRS})

# Automatically collect all .cpp files for client and server
file(GLOB CLIENT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/client/*.cpp
		${CMAKE_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
)

file(GLOB SERVER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/server/*.cpp
		${CMAKE_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
)

add_executable(client ${CLIENT_SOURCES})
add_executable(server ${SERVER_SOURCES})

# Link OpenCV libraries
target_link_libraries(client ${OpenCV_LIBS})
target_link_libraries(server ${OpenCV_LIBS})

# Windows specific libraries
if (WIN32)
    target_link_libraries(client ws2_32 user32)
    target_link_libraries(server ws2_32 user32)
endif()

# Set target properties for Visual Studio
if(MSVC)
    set_target_properties(client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
    )
    set_target_properties(server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
    )
    
    # Copy OpenCV DLLs to output directory
    if(EXISTS "${OpenCV_BIN_DIR}")
        file(GLOB OpenCV_DLLS "${OpenCV_BIN_DIR}/*.dll")
        foreach(dll ${OpenCV_DLLS})
            add_custom_command(TARGET client POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${dll} $<TARGET_FILE_DIR:client>)
            add_custom_command(TARGET server POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${dll} $<TARGET_FILE_DIR:server>)
        endforeach()
        message(STATUS "OpenCV DLLs will be copied to output directory")
    else()
        message(WARNING "OpenCV bin directory not found: ${OpenCV_BIN_DIR}")
    endif()
endif()

# Additional Windows libraries for new features
if (WIN32)
    target_link_libraries(client 
        kernel32 
        advapi32 
        psapi 
        shell32
        gdi32
    )
    target_link_libraries(server 
        kernel32 
        advapi32 
        psapi 
        shell32
        gdi32
    )
endif()