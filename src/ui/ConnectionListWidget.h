#pragma once
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>

class SessionManager;

class ConnectionListWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConnectionListWidget(SessionManager* mgr, QWidget* parent = nullptr);

signals:
    void connectionActivated(const QString& name);
    void connectionContextMenu(const QString& name);

private:
    void rebuildTree();
    SessionManager* m_mgr = nullptr;
    QTreeView*      m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
};
