#include "VaultDialog.h"
#include <crypto/Vault.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

VaultDialog::VaultDialog(Vault* vault, QWidget* parent)
    : QDialog(parent), m_vault(vault)
{
    setWindowTitle(tr("Vault"));
    setMinimumWidth(380);

    auto* lay = new QVBoxLayout(this);

    auto* vaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/vault.dat";
    m_creating = !QFileInfo::exists(vaultPath);

    auto* title = new QLabel(m_creating
        ? tr("Create a master password to encrypt your credentials.")
        : tr("Enter your master password to unlock the vault."),
        this);
    title->setWordWrap(true);
    lay->addWidget(title);

    auto* form = new QFormLayout;

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Master password:"), m_passwordEdit);

    if (m_creating) {
        m_confirmEdit = new QLineEdit(this);
        m_confirmEdit->setEchoMode(QLineEdit::Password);
        form->addRow(tr("Confirm password:"), m_confirmEdit);
    }

    lay->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto* actionBtn = buttons->addButton(
        m_creating ? tr("Create Vault") : tr("Unlock"),
        QDialogButtonBox::AcceptRole);
    actionBtn->setDefault(true);
    lay->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &VaultDialog::onCreateOrUnlock);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void VaultDialog::onCreateOrUnlock()
{
    auto pw = m_passwordEdit->text();
    if (pw.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Password cannot be empty"));
        return;
    }

    auto vaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + "/vault.dat";

    if (m_creating) {
        if (pw != m_confirmEdit->text()) {
            QMessageBox::warning(this, tr("Error"), tr("Passwords do not match"));
            return;
        }
        QDir().mkpath(QFileInfo(vaultPath).absolutePath());
        if (!m_vault->create(vaultPath, pw)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to create vault"));
            return;
        }
    } else {
        if (!m_vault->unlock(vaultPath, pw)) {
            QMessageBox::critical(this, tr("Error"), tr("Wrong password or corrupted vault"));
            return;
        }
    }

    accept();
}
