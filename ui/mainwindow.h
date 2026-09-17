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
#include "filepanelwidget.h"
#include "dialogs/about.h"
#include "dialogs/customdisk.h"
#include "dialogs/imagewizard.h"
#include "dialogs/partition.h"
#include "dialogs/date.h"
#include "dialogs/rename.h"
#include "dialogs/version.h"

#include <ccos_image/ccos_disk.h>
#include <ccos_image/ccos_format.h>
#include <ccos_image/ccos_image.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <ctime>
#include <QHash>

struct DirViewState {
    int scroll = 0;
    int selectIndex = -1;
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
    QString search_query;
    std::optional<int> hdd_partition;

    // Keyed by the directory inode pointer, so going back into a previously
    // visited directory (e.g. leaving a subfolder) restores selection + scroll.
    QHash<quintptr, DirViewState> view_state;

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
    void Search();
    void SetActivePart();
    void Version();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    std::array<std::optional<DiskPanel>, 2> panels;
    int active_panel = 0;

private:
    std::unique_ptr<Ui::MainWindow> ui;

    // --- Panel helpers -----------------------------------------------------
    FilePanelWidget* panelWidget(int panel_idx);
    void onPanelActivated(int panel_idx);

    // Signal handlers wired to both FilePanelWidgets.
    void onFileDoubleClicked(int panel_idx, int fileIndex);
    void onFileRightClicked(int panel_idx, int fileIndex, const QPoint& globalPos);
    void onGoUpToParent(int panel_idx);
    void onUrlsDropped(int panel_idx, const QStringList& files, const QStringList& dirs);
    void onOpenRequested(int panel_idx);
    void onSearchRequested(int panel_idx, const QString& query);

    bool isFileAlreadyOpened(const QString& path);
    void handleAlreadyOpenedImg(const QString& path, int targetPanel);
    void openAnotherPartition(int targetPanel, int sourcePanel);
    bool suggestSelectAnotherPartition();
    void openValidNonMbrDisk(QString path, ccos_disk_t* disk);
    void tryToOpenValidMbrDisk(QString path, uint8_t* data, size_t size);
    void openValidMbrPartition(QString path, std::vector<uint8_t> hdddata, int partition_index, ccos_disk_t* disk);
    void loadCustomImg(QString path, uint8_t* data, size_t size);

    // Standard image opening pipeline (bootsector/size/MBR/custom detection).
    // Used by LoadImg after optional IMD->img conversion.
    void loadImgStandard(QString path);
    void createMonolithicImage(const ImageCreationWizard::Result& settings);
    void createMbrImage(const ImageCreationWizard::Result& settings);

    void fillTable(int panel_idx, ccos_inode_t* directory, bool noRoot);

    void saveCurrentViewState(int panel_idx);
    void applyViewState(int panel_idx, ccos_inode_t* directory);
    void refreshPanel(int panel_idx);

    void updatePanelTitle(int panel_idx);
    void refreshActivePanelUI();
    void updateActionStates();  // context-sensitive menu items (Add/MakeDir, label)

    bool goToParentDir(int panel_idx);

    void doPreview(int panel_idx, ccos_inode_t* file);

    QVector<PanelFileEntry> buildFileEntries(int panel_idx, ccos_inode_t* directory);
    QVector<PanelFileEntry> buildSearchResults(int panel_idx, const QString& query);
    void showSearchResults(int panel_idx);
};
#endif // MAINWINDOW_H
