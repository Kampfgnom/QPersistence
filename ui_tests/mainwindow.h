#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QMainWindow>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void createObjects();

private slots:
    void on_actionCreate_clean_schema_triggered();

    void on_actionCreate_objects_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
