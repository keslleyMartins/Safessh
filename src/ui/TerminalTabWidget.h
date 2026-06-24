#pragma once
#include <QTabWidget>

class QTermWidget;

class TerminalTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit TerminalTabWidget(QWidget* parent = nullptr);

    QTermWidget* openTerminal(const QString& title);
    void closeTerminal(int index);
    void closeCurrentTerminal();
    QTermWidget* currentTerminal() const;
    QString currentTabName() const;
    int findTab(const QString& title) const;

private:
    int m_tabCounter = 0;
};
