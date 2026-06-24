#pragma once
#include <QDialog>

class ThemeManager;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(ThemeManager* themeMgr, QWidget* parent = nullptr);

private:
    ThemeManager* m_themeMgr = nullptr;
};
