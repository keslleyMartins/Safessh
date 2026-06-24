#pragma once
#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>

class SFTPBrowser : public QWidget {
    Q_OBJECT
public:
    explicit SFTPBrowser(QWidget* parent = nullptr);

private:
    QTreeView* m_tree = nullptr;
    QFileSystemModel* m_model = nullptr;
};
