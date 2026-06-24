#pragma once
#include <QObject>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(QObject* parent = nullptr);

    bool isDark() const { return m_dark; }
    void setDark(bool dark);

    void apply();

signals:
    void themeChanged(bool dark);

private:
    bool m_dark = true;
    QString styleSheet(const QString& path) const;
};
