#ifndef IMAGECREATIONWIZARD_H
#define IMAGECREATIONWIZARD_H

#include <QVector>
#include <QWizard>

class ImageCreationWizard : public QWizard {
    Q_OBJECT

public:
    enum class ImageKind {
        Bubble,
        Floppy,
        HardDisk,
    };

    enum class FloppyKind {
        Compass360K,
        GridCase720K,
    };

    struct Result {
        ImageKind kind = ImageKind::Bubble;
        FloppyKind floppyKind = FloppyKind::Compass360K;
        bool useMbr = false;
        int sizeMiB = 10;
        QVector<int> partitionSizesMiB;
        QVector<QString> partitionLabels;
        QString label;
    };

    enum PageId {
        MediaPage,
        FloppyPage,
        HardDiskLayoutPage,
        HardDiskSizePage,
        PartitionPage,
        LabelPage,
    };

    explicit ImageCreationWizard(QWidget* parent = nullptr);

    Result result() const;
};

#endif  // IMAGECREATIONWIZARD_H
