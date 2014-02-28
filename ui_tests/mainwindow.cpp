#include "mainwindow.h"

#include "object.h"

#include <QPersistence.h>

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSqlDatabase db = Qp::database();
    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");

        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(false);
        Qp::registerClass<Object>();
        Qp::adjustDatabaseSchema();
    }
    QpObjectListModel<Object> *model = new QpObjectListModel<Object>(this);
    model->setFetchCount(10000);

    m_model = new QpSortFilterProxyObjectModel<Object>(model, this);
    ui->treeView->setModel(m_model);
    ui->treeView->setColumnHidden(1, true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createObjects()
{
}

void MainWindow::on_actionCreate_clean_schema_triggered()
{
    Qp::createCleanSchema();
}

void MainWindow::on_actionCreate_objects_triggered()
{
    QMessageBox msg(this);
    msg.setText("Creating objects. This may take a while...");
    msg.show();

    QAbstractItemModel * m = ui->treeView->model();
    ui->treeView->setModel(0);
    delete m;

    QSqlQuery query(Qp::database());
    query.prepare("INSERT INTO  `niklas`.`object` ("
                  "`_Qp_ID` ,"
                  "`_Qp_creationTime` ,"
                  "`string` ,"
                  "`_Qp_updateTime` ,"
                  "`date`"
                  ")"
                  "VALUES ("
                  "NULL , NOW( ) ,  ?, NOW( ) , NOW( )"
                  ");");

    const int COUNT = 1000000;
    for(int i = 0; i < COUNT; ++i) {
        if(msg.result() == QMessageBox::Ok)
            break;

        query.addBindValue(tr("Object %1").arg(i));
        query.exec();

        if(query.lastError().isValid())
            qDebug() << query.lastError();

        if(i % 100 == 0) {
            msg.setInformativeText(tr("%1 / %2").arg(i + 1).arg(COUNT));
            QApplication::processEvents();
        }
    }

    QpObjectListModel<Object> *model = new QpObjectListModel<Object>(this);
    model->setFetchCount(1000);

    m_model = new QpSortFilterProxyObjectModel<Object>(model, this);
    ui->treeView->setModel(m_model);
    ui->treeView->setColumnHidden(1, true);
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    m_model->setFilterFixedString(arg1);
    m_model->setFilterKeyColumn(2);
}
