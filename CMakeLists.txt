cmake_minimum_required(VERSION 3.16)
project(SteamIdlerEnhanced VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable LTO to avoid linker issues with mingw
# include(CheckIPOSupported)
# check_ipo_supported(RESULT ipo_supported OUTPUT error)
# if(ipo_supported)
#     set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
# endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Sql Concurrent Network)

# 1. Main GUI Sources (Excluding the background worker)
file(GLOB_RECURSE MAIN_SOURCES "src/*.cpp")
list(REMOVE_ITEM MAIN_SOURCES "${CMAKE_SOURCE_DIR}/src/ghost_launcher.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# 2. Configure Main Executable
add_executable(${PROJECT_NAME} WIN32 ${MAIN_SOURCES} ${HEADERS} resources.qrc)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets 
    Qt6::Sql 
    Qt6::Concurrent 
    Qt6::Network 
    Qt6::Gui 
    Qt6::Core
    crypt32
)

# 3. Configure Background Worker (Ghost Launcher)
# Note: We build this without Qt to keep it under 20KB
add_executable(ghost_launcher WIN32 src/ghost_launcher.cpp)

# Apply size optimizations specifically to the launcher
if(MSVC)
    target_link_options(ghost_launcher PRIVATE "/OPT:REF" "/OPT:ICF" "/NODEFAULTLIB:LIBCMT")
else()
    # MinGW/GCC optimizations: strip symbols and optimize for size
    target_compile_options(ghost_launcher PRIVATE -Os)
    # Link statically against MinGW runtime
    target_link_options(ghost_launcher PRIVATE -static-libgcc -static-libstdc++ -s)
endif()

# 4. Deployment Logic (Post-Build)
# Ensure the Steam API exists before trying to copy it
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/steam_api64.dll")
    message(WARNING "steam_api64.dll not found in project root! Idling will not work.")
endif()



add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Sync data and themes
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/themes" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/themes"
    
    # Sync binaries
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ghost_launcher>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ghost_launcher.exe"
    
    # Sync Steam dependency
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/steam_api64.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/steam_api64.dll"
    
    COMMENT "Syncing assets and workers to build directory.."
)