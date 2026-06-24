add_library(safessh_core STATIC
    ProtocolHandler.cpp
    SSHSession.cpp
    TelnetSession.cpp
    SerialSession.cpp
    RDPSession.cpp
    VNCSession.cpp
    SessionManager.cpp
    ConnectionConfig.cpp
)

target_include_directories(safessh_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(safessh_core PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::SerialPort
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

safessh_set_warnings(safessh_core)
