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
    void Addf(QStringList files);
    void closeEvent(QCloseEvent *event);
    int  Closef();
    void Copy();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);
    void Delete();
    void enterDir();
    void focusChanged(QWidget*, QWidget* now);
    void Ext();
    void ExtAll();
    void LoadImg(QString path);
    void Label();
    void MkDir();
    void Openf();
    void Rename();
    void Save();
    void SaveAs();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool isop[2] = {0}, nrot[2] = {0};
    uint8_t* dat[2] = {NULL};
    size_t siz[2] = {0};
    bool acdisk = 0;
    QString name[2];
private:
    Ui::MainWindow *ui;
    AbDlg *abss;
    RenDlg *rnam;
    qint8 focused;
    void setFocused(qint8 focused);
};
#endif // MAINWINDOW_H
