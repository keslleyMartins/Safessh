#include "SFTPBrowser.h"
#include <QVBoxLayout>

SFTPBrowser::SFTPBrowser(QWidget* parent)
    : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_model = new QFileSystemModel(this);
    m_model->setRootPath(QDir::homePath());

    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    m_tree->setRootIndex(m_model->index(QDir::homePath()));
    m_tree->setSortingEnabled(true);
    m_tree->setAnimated(true);
    m_tree->hideColumn(1); // size
    m_tree->hideColumn(3); // type

    lay->addWidget(m_tree);
}
