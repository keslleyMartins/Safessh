#include "ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QPalette>
#include <spdlog/spdlog.h>

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    apply();
}

void ThemeManager::setDark(bool dark)
{
    if (m_dark == dark) return;
    m_dark = dark;
    apply();
    emit themeChanged(dark);
}

static QString loadFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll());
}

void ThemeManager::apply()
{
    auto* app = qApp;
    if (!app) return;

    const auto theme = m_dark ? "dark" : "light";
    const auto qssPath = QStringLiteral(":/styles/%1.qss").arg(theme);

    const auto sheet = loadFile(qssPath);
    if (!sheet.isEmpty()) {
        app->setStyleSheet(sheet);
    }

    QPalette pal;
    if (m_dark) {
        pal.setColor(QPalette::Window,        QColor(26, 27, 30));
        pal.setColor(QPalette::WindowText,    QColor(228, 228, 231));
        pal.setColor(QPalette::Base,          QColor(32, 33, 36));
        pal.setColor(QPalette::AltBase,       QColor(39, 39, 42));
        pal.setColor(QPalette::Text,          QColor(228, 228, 231));
        pal.setColor(QPalette::Button,        QColor(39, 39, 42));
        pal.setColor(QPalette::ButtonText,    QColor(228, 228, 231));
        pal.setColor(QPalette::Highlight,     QColor(91, 110, 226));
        pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    } else {
        pal.setColor(QPalette::Window,        QColor(255, 255, 255));
        pal.setColor(QPalette::WindowText,    QColor(55, 65, 81));
        pal.setColor(QPalette::Base,          QColor(249, 250, 251));
        pal.setColor(QPalette::AltBase,       QColor(243, 244, 246));
        pal.setColor(QPalette::Text,          QColor(55, 65, 81));
        pal.setColor(QPalette::Button,        QColor(243, 244, 246));
        pal.setColor(QPalette::ButtonText,    QColor(55, 65, 81));
        pal.setColor(QPalette::Highlight,     QColor(91, 110, 226));
        pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    }
    app->setPalette(pal);
}

QString ThemeManager::styleSheet(const QString& path) const
{
    return loadFile(path);
}
