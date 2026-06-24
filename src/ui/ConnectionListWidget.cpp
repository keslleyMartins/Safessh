#include "ConnectionListWidget.h"
#include <core/SessionManager.h>

#include <QVBoxLayout>
#include <QHeaderView>

ConnectionListWidget::ConnectionListWidget(SessionManager* mgr, QWidget* parent)
    : QWidget(parent), m_mgr(mgr)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({tr("Name"), tr("Type")});

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_treeView->header()->setStretchLastSection(true);
    lay->addWidget(m_treeView);

    connect(m_mgr, &SessionManager::connectionsChanged, this, &ConnectionListWidget::rebuildTree);
    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex& idx) {
        if (auto* item = m_model->itemFromIndex(idx)) {
            auto connName = item->data(Qt::UserRole).toString();
            if (!connName.isEmpty()) emit connectionActivated(connName);
        }
    });

    rebuildTree();
}

static QString protocolIcon(ProtocolType p)
{
    switch (p) {
        case ProtocolType::SSH:    return "ssh";
        case ProtocolType::Telnet: return "telnet";
        case ProtocolType::Serial: return "serial";
        case ProtocolType::RDP:    return "rdp";
        case ProtocolType::VNC:    return "vnc";
    }
    return "ssh";
}

void ConnectionListWidget::rebuildTree()
{
    m_model->clear();
    m_model->setHorizontalHeaderLabels({tr("Name"), tr("Type")});

    // Group connections by their group field
    QHash<QString, QStandardItem*> groups;
    for (const auto& conn : m_mgr->connections()) {
        auto* parent = m_model->invisibleRootItem();
        if (!conn.group.isEmpty()) {
            auto* g = groups.value(conn.group);
            if (!g) {
                g = new QStandardItem(conn.group);
                g->setFlags(g->flags() & ~Qt::ItemIsEditable);
                parent->appendRow(g);
                groups[conn.group] = g;
            }
            parent = g;
        }
        auto* nameItem = new QStandardItem(conn.name);
        nameItem->setData(conn.name, Qt::UserRole);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        parent->appendRow(nameItem);
        parent->setChild(parent->rowCount() - 1, 1,
            new QStandardItem(protocolIcon(conn.protocol)));
    }
    m_treeView->expandAll();
}
