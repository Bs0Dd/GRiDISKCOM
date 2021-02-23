#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "abdlg.h"
#include "rendlg.h"
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
    void AboutShow();
    void AboutQtShow();
    void Add();
    void closeEvent(QCloseEvent *event);
    void Closef();
    void Copy();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);
    void Delete();
    void enterDir();
    void Ext();
    void ExtAll();
    void Label();
    void MkDir();
    void Openf();
    void Rename();
    void Save();
    void SaveAs();
    void setActive0();
    void setActive1();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool isop[2] = {0}, nrot[2] = {0};
    uint8_t* dat[2] = {NULL};
    size_t siz[2] = {0};
    bool acdisk = 0;
    QFont diskfont;
    QString name[2];
private:
    Ui::MainWindow *ui;
    AbDlg *abss;
    RenDlg *rnam;
};
#endif // MAINWINDOW_H
