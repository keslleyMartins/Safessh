#include "TerminalTabWidget.h"
#include <QTermWidget>
#include <QVBoxLayout>
#include <QToolButton>
#include <QTabBar>

TerminalTabWidget::TerminalTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setTabPosition(QTabWidget::South);

    connect(this, &QTabWidget::tabCloseRequested,
            this, &TerminalTabWidget::closeTerminal);
}

QTermWidget* TerminalTabWidget::openTerminal(const QString& title)
{
    auto* term = new QTermWidget(this);
    term->setScrollBarPosition(QTermWidget::ScrollBarRight);
    term->setBlinkingCursor(true);
    term->setTerminalOpacity(0.92);

    auto* container = new QWidget(this);
    auto* lay = new QVBoxLayout(container);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(term);

    const auto tabTitle = title.isEmpty()
        ? QStringLiteral("Terminal %1").arg(++m_tabCounter)
        : title;
    addTab(container, tabTitle);
    setCurrentWidget(container);

    return term;
}

void TerminalTabWidget::closeTerminal(int index)
{
    auto* w = widget(index);
    removeTab(index);
    w->deleteLater();
}
