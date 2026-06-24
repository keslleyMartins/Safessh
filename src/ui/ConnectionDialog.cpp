#include "ConnectionDialog.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>

ConnectionDialog::ConnectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connection"));
    setMinimumWidth(420);

    auto* form = new QFormLayout(this);

    m_nameEdit      = new QLineEdit(this);
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItems({"SSH", "Telnet", "Serial", "RDP", "VNC"});
    m_hostEdit      = new QLineEdit(this);
    m_hostEdit->setPlaceholderText("hostname or IP");
    m_portSpin      = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    m_userEdit      = new QLineEdit(this);
    m_passwordEdit  = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_authCombo     = new QComboBox(this);
    m_authCombo->addItems({"Password", "Key File", "Agent", "Interactive", "GSSAPI", "None"});
    m_identityEdit  = new QLineEdit(this);
    m_groupEdit     = new QLineEdit(this);
    m_timeoutSpin   = new QSpinBox(this);
    m_timeoutSpin->setRange(1, 300);
    m_timeoutSpin->setValue(10);
    m_timeoutSpin->setSuffix(" s");

    form->addRow(tr("Name:"),       m_nameEdit);
    form->addRow(tr("Protocol:"),   m_protocolCombo);
    form->addRow(tr("Host:"),       m_hostEdit);
    form->addRow(tr("Port:"),       m_portSpin);
    form->addRow(tr("Username:"),   m_userEdit);
    form->addRow(tr("Password:"),   m_passwordEdit);
    form->addRow(tr("Auth:"),       m_authCombo);
    form->addRow(tr("Identity:"),   m_identityEdit);
    form->addRow(tr("Group:"),      m_groupEdit);
    form->addRow(tr("Timeout:"),    m_timeoutSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Auto-port logic
    connect(m_protocolCombo, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        if      (t == "SSH")    m_portSpin->setValue(22);
        else if (t == "Telnet") m_portSpin->setValue(23);
        else if (t == "RDP")    m_portSpin->setValue(3389);
        else if (t == "VNC")    m_portSpin->setValue(5900);
        else if (t == "Serial") m_portSpin->setValue(0);
    });
}

ConnectionConfig ConnectionDialog::config() const
{
    ConnectionConfig c;
    c.name         = m_nameEdit->text();
    c.protocol     = static_cast<ProtocolType>(m_protocolCombo->currentIndex());
    c.host         = m_hostEdit->text();
    c.port         = static_cast<uint16_t>(m_portSpin->value());
    c.username     = m_userEdit->text();
    c.password     = m_passwordEdit->text();
    c.authMethod   = static_cast<AuthMethod>(m_authCombo->currentIndex());
    c.identityFile = m_identityEdit->text();
    c.group        = m_groupEdit->text();
    c.timeoutSec   = m_timeoutSpin->value();
    return c;
}

void ConnectionDialog::setConfig(const ConnectionConfig& cfg)
{
    m_nameEdit->setText(cfg.name);
    m_protocolCombo->setCurrentIndex(static_cast<int>(cfg.protocol));
    m_hostEdit->setText(cfg.host);
    m_portSpin->setValue(cfg.port);
    m_userEdit->setText(cfg.username);
    m_passwordEdit->setText(cfg.password);
    m_authCombo->setCurrentIndex(static_cast<int>(cfg.authMethod));
    m_identityEdit->setText(cfg.identityFile);
    m_groupEdit->setText(cfg.group);
    m_timeoutSpin->setValue(cfg.timeoutSec);
}
