add_library(safessh_ui STATIC
    MainWindow.cpp
    ConnectionListWidget.cpp
    TerminalTabWidget.cpp
    ConnectionDialog.cpp
    SFTPBrowser.cpp
    SettingsDialog.cpp
    ThemeManager.cpp
)

target_include_directories(safessh_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(safessh_ui PUBLIC
    safessh_core
    safessh_crypto
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    spdlog::spdlog
)

safessh_set_warnings(safessh_ui)
