#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../src/objectlistmodel.h"
#include "model/round.h"

#include <QSortFilterProxyModel>
#include <QtConcurrent/QtConcurrent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QpObjectListModel<NewDatabase::Round> *olm = new QpObjectListModel<NewDatabase::Round>(this);
    QSortFilterProxyModel *model = new QSortFilterProxyModel(this);
    model->setSourceModel(olm);

    ui->treeView->setModel(model);
}

MainWindow::~MainWindow()
{
    delete ui;
}
