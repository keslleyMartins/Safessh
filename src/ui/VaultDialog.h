#pragma once
#include <QDialog>

class QLineEdit;
class Vault;

class VaultDialog : public QDialog {
    Q_OBJECT
public:
    explicit VaultDialog(Vault* vault, QWidget* parent = nullptr);

private slots:
    void onCreateOrUnlock();

private:
    Vault* m_vault = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_confirmEdit = nullptr;
    bool m_creating = false;
};
