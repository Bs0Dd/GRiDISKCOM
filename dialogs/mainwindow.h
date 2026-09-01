#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QCloseEvent>
#include <QKeyEvent>
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

#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <ctime>

struct MbrPartition {
    size_t   index;
    bool     isGRiD;
    bool     isActive;
    uint64_t offset;
    uint64_t size;
};

struct DiskPanel {
    QString path;
    ccos_disk_t* disk = nullptr;
    ccos_inode_t* current_dir = nullptr;
    std::vector<ccos_inode_t*> inodes;
    bool modified = false;
    bool in_subdir = false;

    // True when the image was opened from an .IMD file. The actual on-disk
    // file pointed to by `path` is then a temporary converted .img, which we
    // own and clean up ourselves. IMD cannot be written back, so Save() is
    // blocked and the user is redirected to Save As.
    bool is_imd = false;

    bool hdd_mode = false;
    std::shared_ptr<std::vector<uint8_t>> hdd_data;
    std::optional<int> hdd_partition;

    ~DiskPanel() {
        if (is_imd && !path.isEmpty()) {
            QFile::remove(path); // remove the temporary converted image
        }
        if (disk == nullptr)
            return;
        if (hdd_mode) {
            // The image data is shared with hdd_data (an offset into the HDD
            // buffer), so only release the opaque disk handle and keep the data.
            free(disk);
        } else {
            ccos_disk_free(disk);
        }
    }
};

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
    void ShowPreview(const QPoint& pos);
    void ShowPreview();
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
    std::array<std::optional<DiskPanel>, 2> panels;
    int active_panel = 0;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    std::unique_ptr<Ui::MainWindow> ui;

    bool isFileAlreadyOpened(const QString& path);
    void handleAlreadyOpenedImg(QString path);
    bool suggestSelectAnotherPartition();
    void openValidNonMbrDisk(QString path, ccos_disk_t* disk);
    void tryToOpenValidMbrDisk(QString path, uint8_t* data, size_t size);
    void openValidMbrPartition(QString path, std::vector<uint8_t> hdddata, int partition_index, ccos_disk_t* disk);
    void loadCustomImg(QString path, uint8_t* data, size_t size);

    // Standard image opening pipeline (bootsector/size/MBR/custom detection).
    // Used by LoadImg after optional IMD->img conversion.
    void loadImgStandard(QString path);

    void fillTable(int panel_idx, ccos_inode_t* directory, bool noRoot);

    void updatePanelTitle(int panel_idx);
    void refreshActivePanelUI();

    bool goToParentDir(int panel_idx);

    void doPreview(int panel_idx, ccos_inode_t* file);
};
#endif // MAINWINDOW_H
