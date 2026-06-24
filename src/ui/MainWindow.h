#pragma once
#include <QMainWindow>

class SessionManager;
class Vault;
class ConnectionListWidget;
class TerminalTabWidget;
class ThemeManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void connectSignals();
    void loadSettings();
    void saveSettings();

    SessionManager*       m_sessionManager = nullptr;
    Vault*                m_vault = nullptr;
    ConnectionListWidget* m_connectionList = nullptr;
    TerminalTabWidget*    m_terminalTabs = nullptr;
    ThemeManager*         m_themeManager = nullptr;
};
