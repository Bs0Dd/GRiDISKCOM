#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "about.h"
#include "rename.h"
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
    int isop[2], nrot[2];
    uint8_t* dat[2];
    size_t siz[2];
    int acdisk = 0;
    QFont diskfont;
    void setactive0();
    void setactive1();
    void Openf();
    void enterDIR();
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
    void Label();
    void aboutQtShow();
    void closeEvent(QCloseEvent *event);
    QString name[2];
private:
    Ui::MainWindow *ui;
    about *abss;
    Rename *rnam;
};
#endif // MAINWINDOW_H
