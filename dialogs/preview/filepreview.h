#ifndef FILEPREVIEW_H
#define FILEPREVIEW_H

#include <QSize>
#include <QString>
#include <ccos_image/ccos_image.h>

class QWidget;

class FilePreview {
public:
    virtual ~FilePreview() = default;

    virtual QString name() const = 0;

    virtual bool supports(const QString& fileType, size_t fileSize) const = 0;

    virtual QWidget* createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) = 0;

    // Optional hint: a preview may request a larger initial dialog than the
    // default (e.g. the font preview needs room for its glyph table). An empty
    // size means "use the dialog default". The dialog still clamps to its own
    // minimum/maximum and to the available screen space.
    virtual QSize preferredDialogSize() const { return QSize(); }
};

#endif  // FILEPREVIEW_H
