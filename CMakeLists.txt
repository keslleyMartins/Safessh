cmake_minimum_required(VERSION 3.28)
project(SafeSSH
    VERSION 0.1.0
    DESCRIPTION "Native SSH/RDP/VNC/Serial/Telnet client with encrypted vault"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Third-party dependencies ────────────────────────────────────────────────

include(FetchContent)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    GIT_SHALLOW TRUE
)
FetchContent_Declare(libsodium
    GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
    GIT_TAG 1.0.20-stable
    GIT_SHALLOW TRUE
)
FetchContent_Declare(libssh
    GIT_REPOSITORY https://git.libssh.org/projects/libssh.git
    GIT_TAG libssh-0.12.0
    GIT_SHALLOW TRUE
)
FetchContent_Declare(libssh2
    GIT_REPOSITORY https://github.com/libssh2/libssh2.git
    GIT_TAG libssh2-1.11.1
    GIT_SHALLOW TRUE
)
FetchContent_Declare(QTermWidget
    GIT_REPOSITORY https://github.com/lxqt/qtermwidget.git
    GIT_TAG 2.1.0
    GIT_SHALLOW TRUE
)
FetchContent_Declare(FreeRDP
    GIT_REPOSITORY https://github.com/FreeRDP/FreeRDP.git
    GIT_TAG 3.10.3
    GIT_SHALLOW TRUE
)
FetchContent_Declare(LibVNC
    GIT_REPOSITORY https://github.com/LibVNC/libvncserver.git
    GIT_TAG LibVNCServer-0.9.15
    GIT_SHALLOW TRUE
)

# Make dependencies available
FetchContent_MakeAvailable(
    nlohmann_json
    spdlog
    libsodium
    libssh
)

# libssh needs its CMake config path set
list(PREPEND CMAKE_PREFIX_PATH ${libssh_BINARY_DIR})

# ─── Qt6 ─────────────────────────────────────────────────────────────────────

find_package(Qt6 6.7 REQUIRED COMPONENTS
    Core
    Widgets
    SerialPort
    Svg
    Network
)

# ─── Compiler warnings ────────────────────────────────────────────────────────

include(cmake/CompilerWarnings.cmake)

# ─── Subdirectories ───────────────────────────────────────────────────────────

add_subdirectory(src)

# ─── Tests ────────────────────────────────────────────────────────────────────

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
