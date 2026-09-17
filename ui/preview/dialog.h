#ifndef PREVIEWDLG_H
#define PREVIEWDLG_H

#include <QDialog>
#include <ccos_image/ccos_image.h>

class PreviewDlg : public QDialog {
    Q_OBJECT
public:
    PreviewDlg(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
};

#endif  // PREVIEWDLG_H
