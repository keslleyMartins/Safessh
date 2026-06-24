#include "SettingsDialog.h"
#include "ThemeManager.h"

#include <QFormLayout>
#include <QComboBox>
#include <QTabWidget>
#include <QGroupBox>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(ThemeManager* themeMgr, QWidget* parent)
    : QDialog(parent), m_themeMgr(themeMgr)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(500);

    auto* mainLay = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);

    // ── Appearance tab ──
    auto* appearance = new QWidget(this);
    auto* appForm = new QFormLayout(appearance);
    auto* themeCombo = new QComboBox(appearance);
    themeCombo->addItems({"Dark", "Light"});
    if (m_themeMgr->isDark()) themeCombo->setCurrentIndex(0);
    else                      themeCombo->setCurrentIndex(1);
    connect(themeCombo, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        m_themeMgr->setDark(t == "Dark");
    });
    appForm->addRow(tr("Theme:"), themeCombo);
    tabs->addTab(appearance, tr("Appearance"));

    mainLay->addWidget(tabs);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLay->addWidget(buttons);
}
