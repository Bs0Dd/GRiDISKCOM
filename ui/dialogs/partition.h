#ifndef PARTITIONDLG_H
#define PARTITIONDLG_H

#include <QDialog>
#include <QSet>
#include <QString>
#include <QStringList>
#include <vector>

#include "mbr.h"

namespace Ui {
class PartitionDlg;
}

class PartitionDlg : public QDialog
{
    Q_OBJECT

public:
    explicit PartitionDlg(QWidget *parent = nullptr);
    ~PartitionDlg();

    void setTitle(const QString &text);
    void setInfo(const QString &text);

    // `labels` is parallel to `parts`; empty strings are allowed.
    void setPartitions(const std::vector<MbrPartition> &parts,
                       const QStringList &labels,
                       const QSet<int> &disabledSlots = {});

    // Prepends a selectable entry before the partitions; selectedIndex()
    // returns -1 when it is chosen.
    void allowNone(const QString &noneText = {});

    int selectedIndex() const;

private:
    void addRow(const QString &text, bool enabled);

    Ui::PartitionDlg *ui;
    bool m_hasNone = false;
    QString m_noneText;
};

#endif // PARTITIONDLG_H
