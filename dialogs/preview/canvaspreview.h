#ifndef CANVASPREVIEW_H
#define CANVASPREVIEW_H

#include "filepreview.h"

class CanvasPreview : public FilePreview {
public:
    QString name() const override { return "Canvas"; }
    bool supports(const QString& fileType, size_t fileSize) const override;
    QWidget* createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) override;
};

#endif  // CANVASPREVIEW_H
