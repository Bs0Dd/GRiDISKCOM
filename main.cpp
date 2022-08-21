#include <QApplication>
#include "dialogs/mainwindow.h"

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QObject::connect(&a, SIGNAL(focusChanged(QWidget*,QWidget*)), &w, SLOT(FocusChanged(QWidget*,QWidget*)));
    w.show();
    return a.exec();
}
