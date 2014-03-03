#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QPersistence.h>

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QMainWindow>
#include <QFutureWatcher>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

namespace Ui {
    class MainWindow;
}

class Object;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void createObjects();

    void modelCreationFinished();
private slots:
    void on_actionCreate_clean_schema_triggered();

    void on_actionCreate_objects_triggered();

    void on_lineEdit_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
