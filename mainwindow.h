#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "abbt.h"
#include "sys/stat.h"

#if defined(WIN32)
#define szcor 20
#define MKDIR(filename, mode) mkdir(filename)
#else
#define szcor 42
#define MKDIR(filename, mode) mkdir(filename, mode)
#endif

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int acdisk = 0;
    int isop0 = 0;
    int isop1 = 0;
    int isch0 = 0;
    int isch1 = 0;
    int nrot0 = 0;
    int nrot1 = 0;
    int posi0 = 0;
    int posi1 = 0;
    QFont diskfont;
    void setactive0();
    void setactive1();
    void Openf();
    void enterDIR0();
    void enterDIR1();
    void Closef();
    void aboutShow();
    void Dfils();
    QString name0, name1;
private:
    Ui::MainWindow *ui;
    abbt *abss;
};
#endif // MAINWINDOW_H
