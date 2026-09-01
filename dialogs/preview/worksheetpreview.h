#ifndef WORKSHEETPREVIEW_H
#define WORKSHEETPREVIEW_H

#include "filepreview.h"

class WorksheetPreview : public FilePreview {
public:
    QString name() const override { return "Worksheet"; }
    bool supports(const QString& fileType, size_t fileSize) const override;
    QWidget* createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) override;
};

#endif  // WORKSHEETPREVIEW_H
