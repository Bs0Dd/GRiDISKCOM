#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QInputDialog>
#include <QMimeData>
#include <vector>
#include "ui_mainwindow.h"
#include "abdlg.h"
#include "datedlg.h"
#include "rendlg.h"
#include "verdlg.h"

extern "C"{ //Load C "ccos_image" library headers
#include <ccos_image/ccos_image.h>
#include <ccos_image/ccos_private.h>
#include <ccos_image/common.h>
}

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public slots:
    void AboutShow();
    void Add();
    void AddDirs(QStringList dirs);
    int  AddFiles(QStringList files, ccos_inode_t* copyTo);
    void closeEvent(QCloseEvent *event);
    int  CloseImg();
    void Copy();
    void Date();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);
    void DebTrace();
    void Delete();
    void OpenDir();
    void focusChanged(QWidget*, QWidget* now);
    void Extract();
    void ExtractAll();
    void LoadImg(QString path);
    void Label();
    void MakeDir();
    void New();
    void OpenImg();
    void Rename();
    void Save();
    void SaveAs();
    void Version();
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
    DateDlg *datd;
    RenDlg *rnam;
    VerDlg *vdlg;
    qint8 focused;
    void setFocused(qint8 focused);
};
#endif // MAINWINDOW_H
