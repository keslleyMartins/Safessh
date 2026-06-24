add_executable(test_vault test_vault.cpp)
target_link_libraries(test_vault PRIVATE
    safessh_crypto
    Qt6::Core
    Qt6::Test
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
safessh_set_warnings(test_vault)
add_test(NAME VaultTest COMMAND test_vault)

add_executable(test_session test_session.cpp)
target_link_libraries(test_session PRIVATE
    safessh_core
    Qt6::Core
    Qt6::Test
    Qt6::Network
    Qt6::SerialPort
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
safessh_set_warnings(test_session)
add_test(NAME SessionTest COMMAND test_session)
