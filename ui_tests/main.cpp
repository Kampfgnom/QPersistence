#include "mainwindow.h"

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QApplication>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.createObjects();

    return a.exec();
}
