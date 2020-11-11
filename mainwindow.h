#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "about.h"
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
    int nrot0 = 0;
    int nrot1 = 0;
    uint8_t* dat0 = NULL;
    size_t siz0 = 0;
    uint8_t* dat1 = NULL;
    size_t siz1 = 0;
    QFont diskfont;
    void setactive0();
    void setactive1();
    void Openf();
    void enterDIR0();
    void enterDIR1();
    void Closef();
    void aboutShow();
    void Delete();
    void Extall();
    void Ext();
    void Save();
    void SaveAs();
    void Add();
    void MkDir();
    void Copy();
    void Ren();
    void aboutQtShow();
    void closeEvent(QCloseEvent *event);
    QString name0, name1;
private:
    Ui::MainWindow *ui;
    about *abss;
};
#endif // MAINWINDOW_H
