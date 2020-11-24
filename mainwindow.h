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
public slots:
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
    //void Copy();
    void Ren();
    void Label();
    void aboutQtShow();
    void closeEvent(QCloseEvent *event);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool isop[2] ={0}, nrot[2] ={0};
    uint8_t* dat[2] ={NULL};
    size_t siz[2] = {0};
    bool acdisk = 0;
    QFont diskfont;
    QString name[2];
private:
    Ui::MainWindow *ui;
    about *abss;
    Rename *rnam;
};
#endif // MAINWINDOW_H
