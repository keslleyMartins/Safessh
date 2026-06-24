#include "ConnectionDialog.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSerialPortInfo>

ConnectionDialog::ConnectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Connection"));
    setMinimumWidth(480);

    auto* mainLay = new QVBoxLayout(this);

    // ── Common fields ──
    auto* form = new QFormLayout;

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("e.g. My Production Server");
    form->addRow(tr("Name:"), m_nameEdit);

    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItems({"SSH", "Telnet", "Serial", "RDP", "VNC"});
    form->addRow(tr("Protocol:"), m_protocolCombo);

    m_groupEdit = new QLineEdit(this);
    m_groupEdit->setPlaceholderText("e.g. Production");
    form->addRow(tr("Group:"), m_groupEdit);

    m_timeoutSpin = new QSpinBox(this);
    m_timeoutSpin->setRange(1, 300);
    m_timeoutSpin->setValue(10);
    m_timeoutSpin->setSuffix(" s");
    form->addRow(tr("Timeout:"), m_timeoutSpin);

    mainLay->addLayout(form);

    // ── Protocol-specific stacked widget ──
    m_protoStack = new QStackedWidget(this);

    auto* sshPage = new QWidget;
    buildSshFields(sshPage);
    m_protoStack->addWidget(sshPage); // index 0

    auto* telnetPage = new QWidget;
    buildTelnetFields(telnetPage);
    m_protoStack->addWidget(telnetPage); // index 1

    auto* serialPage = new QWidget;
    buildSerialFields(serialPage);
    m_protoStack->addWidget(serialPage); // index 2

    auto* rdpPage = new QWidget;
    buildRdpFields(rdpPage);
    m_protoStack->addWidget(rdpPage); // index 3

    auto* vncPage = new QWidget;
    buildVncFields(vncPage);
    m_protoStack->addWidget(vncPage); // index 4

    mainLay->addWidget(m_protoStack);

    // ── Buttons ──
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLay->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_protocolCombo, &QComboBox::currentIndexChanged, this, &ConnectionDialog::onProtocolChanged);

    onProtocolChanged(0);
}

// ── Field builders ─────────────────────────────────────────────────────

void ConnectionDialog::buildSshFields(QWidget* parent)
{
    auto* form = new QFormLayout(parent);
    form->setContentsMargins(0, 8, 0, 0);

    m_hostEdit = new QLineEdit(parent);
    m_hostEdit->setPlaceholderText("hostname or IP");
    form->addRow(tr("Host:"), m_hostEdit);

    m_portSpin = new QSpinBox(parent);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    form->addRow(tr("Port:"), m_portSpin);

    m_userEdit = new QLineEdit(parent);
    m_userEdit->setPlaceholderText("root");
    form->addRow(tr("Username:"), m_userEdit);

    m_passwordEdit = new QLineEdit(parent);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Password:"), m_passwordEdit);

    m_authCombo = new QComboBox(parent);
    m_authCombo->addItems({"Password", "Key File", "Agent", "Interactive", "GSSAPI", "None"});
    form->addRow(tr("Auth method:"), m_authCombo);

    m_identityEdit = new QLineEdit(parent);
    m_identityEdit->setPlaceholderText("~/.ssh/id_ed25519");
    form->addRow(tr("Identity file:"), m_identityEdit);

    m_proxyHostEdit = new QLineEdit(parent);
    m_proxyHostEdit->setPlaceholderText("bastion.example.com");
    form->addRow(tr("ProxyJump host:"), m_proxyHostEdit);

    m_proxyPortSpin = new QSpinBox(parent);
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(22);
    form->addRow(tr("Proxy port:"), m_proxyPortSpin);

    m_keepAliveCheck = new QCheckBox(tr("Keep alive"), parent);
    m_keepAliveCheck->setChecked(true);
    form->addRow("", m_keepAliveCheck);
}

void ConnectionDialog::buildTelnetFields(QWidget* parent)
{
    auto* form = new QFormLayout(parent);
    form->setContentsMargins(0, 8, 0, 0);

    m_hostEdit = new QLineEdit(parent);
    m_hostEdit->setPlaceholderText("hostname or IP");
    form->addRow(tr("Host:"), m_hostEdit);

    m_portSpin = new QSpinBox(parent);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(23);
    form->addRow(tr("Port:"), m_portSpin);
}

void ConnectionDialog::buildSerialFields(QWidget* parent)
{
    auto* form = new QFormLayout(parent);
    form->setContentsMargins(0, 8, 0, 0);

    m_serialPortCombo = new QComboBox(parent);
    for (const auto& info : QSerialPortInfo::availablePorts())
        m_serialPortCombo->addItem(info.portName());
    if (m_serialPortCombo->count() == 0) {
        m_serialPortCombo->setEditable(true);
        m_serialPortCombo->setPlaceholderText(tr("No serial ports found"));
    }
    form->addRow(tr("Port:"), m_serialPortCombo);

    m_baudCombo = new QComboBox(parent);
    m_baudCombo->addItems({"1200", "2400", "4800", "9600", "19200", "38400",
                           "57600", "115200", "230400", "460800", "921600"});
    m_baudCombo->setCurrentText("115200");
    form->addRow(tr("Baud rate:"), m_baudCombo);

    m_dataBitsCombo = new QComboBox(parent);
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    form->addRow(tr("Data bits:"), m_dataBitsCombo);

    m_parityCombo = new QComboBox(parent);
    m_parityCombo->addItems({"None", "Even", "Odd", "Mark", "Space"});
    form->addRow(tr("Parity:"), m_parityCombo);

    m_stopBitsCombo = new QComboBox(parent);
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    form->addRow(tr("Stop bits:"), m_stopBitsCombo);
}

void ConnectionDialog::buildRdpFields(QWidget* parent)
{
    auto* form = new QFormLayout(parent);
    form->setContentsMargins(0, 8, 0, 0);

    m_hostEdit = new QLineEdit(parent);
    m_hostEdit->setPlaceholderText("hostname or IP");
    form->addRow(tr("Host:"), m_hostEdit);

    m_portSpin = new QSpinBox(parent);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(3389);
    form->addRow(tr("Port:"), m_portSpin);

    m_userEdit = new QLineEdit(parent);
    form->addRow(tr("Username:"), m_userEdit);

    m_passwordEdit = new QLineEdit(parent);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Password:"), m_passwordEdit);
}

void ConnectionDialog::buildVncFields(QWidget* parent)
{
    auto* form = new QFormLayout(parent);
    form->setContentsMargins(0, 8, 0, 0);

    m_hostEdit = new QLineEdit(parent);
    m_hostEdit->setPlaceholderText("hostname or IP");
    form->addRow(tr("Host:"), m_hostEdit);

    m_portSpin = new QSpinBox(parent);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(5900);
    form->addRow(tr("Port:"), m_portSpin);

    m_passwordEdit = new QLineEdit(parent);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Password:"), m_passwordEdit);
}

// ── Slots ──────────────────────────────────────────────────────────────

void ConnectionDialog::onProtocolChanged(int index)
{
    m_protoStack->setCurrentIndex(index);
}

// ── Config getter ──────────────────────────────────────────────────────

ConnectionConfig ConnectionDialog::config() const
{
    ConnectionConfig c;
    c.name       = m_nameEdit->text();
    c.protocol   = static_cast<ProtocolType>(m_protocolCombo->currentIndex());
    c.group      = m_groupEdit->text();
    c.timeoutSec = m_timeoutSpin->value();

    switch (c.protocol) {
    case ProtocolType::SSH:
        c.host     = m_hostEdit->text();
        c.port     = static_cast<uint16_t>(m_portSpin->value());
        c.username = m_userEdit->text();
        c.password = m_passwordEdit->text();
        c.authMethod   = static_cast<AuthMethod>(m_authCombo->currentIndex());
        c.identityFile = m_identityEdit->text();
        c.proxyHost    = m_proxyHostEdit->text();
        c.proxyPort    = static_cast<uint16_t>(m_proxyPortSpin->value());
        c.keepAlive    = m_keepAliveCheck->isChecked();
        break;
    case ProtocolType::Telnet:
        c.host = m_hostEdit->text();
        c.port = static_cast<uint16_t>(m_portSpin->value());
        break;
    case ProtocolType::Serial:
        // host field stores port name
        c.host = m_serialPortCombo->currentText();
        c.port = 0;
        break;
    case ProtocolType::RDP:
        c.host     = m_hostEdit->text();
        c.port     = static_cast<uint16_t>(m_portSpin->value());
        c.username = m_userEdit->text();
        c.password = m_passwordEdit->text();
        break;
    case ProtocolType::VNC:
        c.host     = m_hostEdit->text();
        c.port     = static_cast<uint16_t>(m_portSpin->value());
        c.password = m_passwordEdit->text();
        break;
    }
    return c;
}

void ConnectionDialog::setConfig(const ConnectionConfig& cfg)
{
    m_nameEdit->setText(cfg.name);
    m_protocolCombo->setCurrentIndex(static_cast<int>(cfg.protocol));
    m_groupEdit->setText(cfg.group);
    m_timeoutSpin->setValue(cfg.timeoutSec);

    // Common fields
    if (m_hostEdit)     m_hostEdit->setText(cfg.host);
    if (m_portSpin)     m_portSpin->setValue(cfg.port);
    if (m_userEdit)     m_userEdit->setText(cfg.username);
    if (m_passwordEdit) m_passwordEdit->setText(cfg.password);
    if (m_authCombo)    m_authCombo->setCurrentIndex(static_cast<int>(cfg.authMethod));
    if (m_identityEdit) m_identityEdit->setText(cfg.identityFile);
    if (m_proxyHostEdit) m_proxyHostEdit->setText(cfg.proxyHost);
    if (m_proxyPortSpin) m_proxyPortSpin->setValue(cfg.proxyPort);
    if (m_keepAliveCheck) m_keepAliveCheck->setChecked(cfg.keepAlive);
    if (m_serialPortCombo) {
        int idx = m_serialPortCombo->findText(cfg.host);
        if (idx >= 0) m_serialPortCombo->setCurrentIndex(idx);
        else if (m_serialPortCombo->isEditable()) m_serialPortCombo->setCurrentText(cfg.host);
    }
}
