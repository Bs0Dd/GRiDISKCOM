#include "partition.h"
#include "ui_partition.h"

#include <QListWidgetItem>

PartitionDlg::PartitionDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PartitionDlg)
{
    ui->setupUi(this);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
}

PartitionDlg::~PartitionDlg()
{
    delete ui;
}

void PartitionDlg::setTitle(const QString &text) {
    setWindowTitle(text);
}

void PartitionDlg::setInfo(const QString &text) {
    ui->label->setText(text);
}

void PartitionDlg::allowNone(const QString &noneText) {
    m_hasNone = true;
    m_noneText = noneText.isEmpty() ? QStringLiteral("None") : noneText;
}

void PartitionDlg::setPartitions(const std::vector<MbrPartition> &parts,
                                 const QStringList &labels,
                                 const QSet<int> &disabledSlots) {
    ui->listWidget->clear();

    if (m_hasNone)
        addRow(m_noneText, true);

    for (size_t i = 0; i < parts.size(); ++i) {
        const MbrPartition &p = parts[i];
        QString text = QString("Partition %1%2").arg(p.index + 1).arg(p.isActive ? "*" : "");

        bool enabled = true;
        if (!p.isGRiD) {
            text += " (not a GRiD partition)";
            enabled = false;
        } else {
            if ((int)i < labels.size() && !labels[i].isEmpty())
                text += QString(" - %1").arg(labels[i]);
            if (disabledSlots.contains((int)p.index)) {
                text += " (already open)";
                enabled = false;
            }
        }
        addRow(text, enabled);
    }
}

int PartitionDlg::selectedIndex() const {
    int row = ui->listWidget->currentRow();
    if (row < 0)
        return -1;
    return m_hasNone ? row - 1 : row;
}

void PartitionDlg::addRow(const QString &text, bool enabled) {
    auto *item = new QListWidgetItem(text, ui->listWidget);
    if (!enabled)
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
    else if (ui->listWidget->currentRow() < 0)
        ui->listWidget->setCurrentItem(item);
}
