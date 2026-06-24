#pragma once
#include <QDialog>
#include "core/ConnectionConfig.h"

class QLineEdit;
class QSpinBox;
class QComboBox;

class ConnectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectionDialog(QWidget* parent = nullptr);

    ConnectionConfig config() const;
    void setConfig(const ConnectionConfig& cfg);

private:
    QLineEdit* m_nameEdit       = nullptr;
    QComboBox* m_protocolCombo  = nullptr;
    QLineEdit* m_hostEdit       = nullptr;
    QSpinBox*  m_portSpin       = nullptr;
    QLineEdit* m_userEdit       = nullptr;
    QLineEdit* m_passwordEdit   = nullptr;
    QComboBox* m_authCombo      = nullptr;
    QLineEdit* m_identityEdit   = nullptr;
    QLineEdit* m_groupEdit      = nullptr;
    QSpinBox*  m_timeoutSpin    = nullptr;
};
