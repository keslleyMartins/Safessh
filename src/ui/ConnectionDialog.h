#pragma once
#include <QDialog>
#include "core/ConnectionConfig.h"

class QLineEdit;
class QSpinBox;
class QComboBox;
class QStackedWidget;
class QWidget;
class QCheckBox;
class QLabel;

class ConnectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectionDialog(QWidget* parent = nullptr);

    ConnectionConfig config() const;
    void setConfig(const ConnectionConfig& cfg);

private slots:
    void onProtocolChanged(int index);

private:
    void buildSshFields(QWidget* parent);
    void buildSerialFields(QWidget* parent);
    void buildTelnetFields(QWidget* parent);
    void buildRdpFields(QWidget* parent);
    void buildVncFields(QWidget* parent);

    QLineEdit* m_nameEdit       = nullptr;
    QComboBox* m_protocolCombo  = nullptr;
    QStackedWidget* m_protoStack = nullptr;

    // SSH
    QLineEdit* m_hostEdit       = nullptr;
    QSpinBox*  m_portSpin       = nullptr;
    QLineEdit* m_userEdit       = nullptr;
    QLineEdit* m_passwordEdit   = nullptr;
    QComboBox* m_authCombo      = nullptr;
    QLineEdit* m_identityEdit   = nullptr;
    QLineEdit* m_proxyHostEdit  = nullptr;
    QSpinBox*  m_proxyPortSpin  = nullptr;
    QCheckBox* m_keepAliveCheck = nullptr;

    // Serial
    QComboBox* m_serialPortCombo = nullptr;
    QComboBox* m_baudCombo       = nullptr;
    QComboBox* m_dataBitsCombo   = nullptr;
    QComboBox* m_parityCombo     = nullptr;
    QComboBox* m_stopBitsCombo   = nullptr;

    // Generic
    QLineEdit* m_groupEdit      = nullptr;
    QSpinBox*  m_timeoutSpin    = nullptr;
};
