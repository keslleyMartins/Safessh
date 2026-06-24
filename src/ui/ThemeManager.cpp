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

    // Fallback palette
    QPalette pal;
    if (m_dark) {
        pal.setColor(QPalette::Window,        QColor(30, 30, 30));
        pal.setColor(QPalette::WindowText,    QColor(220, 220, 220));
        pal.setColor(QPalette::Base,          QColor(18, 18, 18));
        pal.setColor(QPalette::Text,          QColor(220, 220, 220));
        pal.setColor(QPalette::Highlight,     QColor(58, 134, 255));
    } else {
        pal.setColor(QPalette::Window,        QColor(240, 240, 240));
        pal.setColor(QPalette::WindowText,    Qt::black);
        pal.setColor(QPalette::Base,          Qt::white);
        pal.setColor(QPalette::Text,          Qt::black);
        pal.setColor(QPalette::Highlight,     QColor(42, 130, 218));
    }
    app->setPalette(pal);
}

QString ThemeManager::styleSheet(const QString& path) const
{
    return loadFile(path);
}
