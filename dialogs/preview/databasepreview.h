#ifndef DATABASEPREVIEW_H
#define DATABASEPREVIEW_H

#include "filepreview.h"

class DatabasePreview : public FilePreview {
public:
    QString name() const override { return "Database"; }
    bool supports(const QString& fileType, size_t fileSize) const override;
    QWidget* createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) override;
};

#endif  // DATABASEPREVIEW_H
