#ifndef TEXTPREVIEW_H
#define TEXTPREVIEW_H

#include "filepreview.h"

class TextPreview : public FilePreview {
public:
    QString name() const override { return "Text"; }
    bool supports(const QString& fileType, size_t fileSize) const override;
    QWidget* createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) override;
};

#endif  // TEXTPREVIEW_H
