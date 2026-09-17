#ifndef FONTPREVIEW_H
#define FONTPREVIEW_H

#include "file.h"

class FontPreview : public FilePreview {
public:
    QString name() const override { return "Font"; }
    bool supports(const QString& fileType, size_t fileSize) const override;
    QWidget* createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) override;
    // The glyph table (16 columns) plus the typed-text panel need more room
    // than the default 660x540 to be comfortable.
    QSize preferredDialogSize() const override { return QSize(860, 640); }
};

#endif  // FONTPREVIEW_H
