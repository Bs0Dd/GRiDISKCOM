#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QInputDialog>
#include <QMimeData>
#include "ui_mainwindow.h"
#include "abdlg.h"
#include "custdlg.h"
#include "chsedlg.h"
#include "datedlg.h"
#include "rendlg.h"
#include "verdlg.h"

#include <ccos_image/ccos_disk.h>
#include <ccos_image/ccos_format.h>
#include <ccos_image/ccos_image.h>
#include <ccos_image/ccos_private.h>

typedef struct {
  bool isgrid;
  bool active;
  uint64_t offset;
  uint64_t size;
} mbr_part_t;

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
    void AnPartMenu();
    void AnotherPart(bool fromMenu);
    void closeEvent(QCloseEvent *event);
    int  CloseImg();
    void Copy();
    void CopyLoc();
    void Date();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);
    void DebTrace();
    void Delete();
    void OpenDir();
    void FocusChanged(QWidget*, QWidget* now);
    void HDDMenu(bool activ);
    void Extract();
    void ExtractAll();
    void LoadImg(QString path);
    void Label();
    void MakeDir();
    void NewImage();
    void OpenImg();
    void Rename();
    void Save();
    void SaveAs();
    void SavePart();
    void SetActivePart();
    void Version();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool isop[2] = {false}, nrot[2] = {false};
    bool acdisk = false;
    QString name[2];
    bool hddmode[2] = {false};
    uint8_t* hdddat[2] = {NULL};
    size_t hddsiz[2] = {0};
    bool oneimg = false;
    ccos_disk_t ccdesc[2] = {};
private:
    Ui::MainWindow *ui;
    AbDlg *abss;
    ChsDlg *chsd;
    CustDlg *cstdlg;
    DateDlg *datd;
    RenDlg *rnam;
    VerDlg *vdlg;
};
#endif // MAINWINDOW_H
