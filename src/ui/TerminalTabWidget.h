#pragma once
#include <QTabWidget>

class QTermWidget;

class TerminalTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit TerminalTabWidget(QWidget* parent = nullptr);

    QTermWidget* openTerminal(const QString& title);
    void closeTerminal(int index);

private:
    int m_tabCounter = 0;
};
