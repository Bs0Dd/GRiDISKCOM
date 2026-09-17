#include "imagewizard.h"
#include "themedicon.h"

#include <QButtonGroup>
#include <QFormLayout>
#include <QFrame>
#include <QIcon>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

constexpr int kMaxHardDiskMiB = 30;
constexpr int kMaxPartitions = 4;

void configureWizardPage(QWizardPage* page) {
    page->setContentsMargins(0, 0, 0, 0);
}



void addPageHeader(QVBoxLayout* layout, const QString& title, const QString& description) {
    auto* titleLabel = new QLabel(title);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto* descriptionLabel = new QLabel(description);
    descriptionLabel->setWordWrap(true);

    auto* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addWidget(separator);
    layout->addSpacing(6);
}

class MediaPage : public QWizardPage {
public:
    explicit MediaPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        configureWizardPage(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        addPageHeader(layout, "Image type", "Choose the storage medium for the new GRiD-OS image.");
        auto* bubble = new QRadioButton("Bubble memory", this);
        auto* floppy = new QRadioButton("Floppy disk", this);
        auto* hardDisk = new QRadioButton("Hard disk", this);
        bubble->setChecked(true);
        layout->addWidget(bubble);
        layout->addWidget(floppy);
        layout->addWidget(hardDisk);
        layout->addStretch();

        m_buttons.addButton(bubble, int(ImageCreationWizard::ImageKind::Bubble));
        m_buttons.addButton(floppy, int(ImageCreationWizard::ImageKind::Floppy));
        m_buttons.addButton(hardDisk, int(ImageCreationWizard::ImageKind::HardDisk));
    }

    int nextId() const override {
        switch (ImageCreationWizard::ImageKind(m_buttons.checkedId())) {
        case ImageCreationWizard::ImageKind::Bubble: return ImageCreationWizard::LabelPage;
        case ImageCreationWizard::ImageKind::Floppy: return ImageCreationWizard::FloppyPage;
        case ImageCreationWizard::ImageKind::HardDisk: return ImageCreationWizard::HardDiskLayoutPage;
        }
        return -1;
    }

    ImageCreationWizard::ImageKind imageKind() const {
        return ImageCreationWizard::ImageKind(m_buttons.checkedId());
    }

private:
    QButtonGroup m_buttons;
};

class FloppyPage : public QWizardPage {
public:
    explicit FloppyPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        configureWizardPage(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        addPageHeader(layout, "Floppy type", "Choose the target floppy format.");
        auto* compass = new QRadioButton("Compass floppy - 360 KB", this);
        auto* gridCase = new QRadioButton("GRiDCASE floppy - 720 KB", this);
        compass->setChecked(true);
        layout->addWidget(compass);
        layout->addWidget(gridCase);
        layout->addStretch();

        m_buttons.addButton(compass, int(ImageCreationWizard::FloppyKind::Compass360K));
        m_buttons.addButton(gridCase, int(ImageCreationWizard::FloppyKind::GridCase720K));
    }

    int nextId() const override { return ImageCreationWizard::LabelPage; }

    ImageCreationWizard::FloppyKind floppyKind() const {
        return ImageCreationWizard::FloppyKind(m_buttons.checkedId());
    }

private:
    QButtonGroup m_buttons;
};

class HardDiskLayoutPage : public QWizardPage {
public:
    explicit HardDiskLayoutPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        configureWizardPage(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        addPageHeader(layout, "Hard disk layout",
                      "Choose a single GRiD-OS disk or an MBR disk with up to four GRiD partitions.");
        auto* single = new QRadioButton("Single GRiD-OS disk", this);
        auto* mbr = new QRadioButton("MBR disk with GRiD partitions", this);
        single->setChecked(true);
        layout->addWidget(single);
        layout->addWidget(mbr);
        layout->addStretch();

        m_buttons.addButton(single, 0);
        m_buttons.addButton(mbr, 1);
    }

    int nextId() const override {
        return m_buttons.checkedId() == 1 ? ImageCreationWizard::PartitionPage
                                          : ImageCreationWizard::HardDiskSizePage;
    }

    bool useMbr() const { return m_buttons.checkedId() == 1; }

private:
    QButtonGroup m_buttons;
};

class HardDiskSizePage : public QWizardPage {
public:
    explicit HardDiskSizePage(QWidget* parent = nullptr) : QWizardPage(parent) {
        configureWizardPage(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        addPageHeader(layout, "Hard disk size",
                      "Choose the size of the single GRiD-OS disk. The maximum is 30 MiB.");
        auto* form = new QFormLayout;
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_size.setRange(1, kMaxHardDiskMiB);
        m_size.setValue(10);
        m_size.setSuffix(" MiB");
        form->addRow("Disk size:", &m_size);
        layout->addLayout(form);
        layout->addStretch();
    }

    int nextId() const override { return ImageCreationWizard::LabelPage; }
    int sizeMiB() const { return m_size.value(); }

private:
    QSpinBox m_size;
};

class PartitionPage : public QWizardPage {
public:
    explicit PartitionPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        configureWizardPage(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        addPageHeader(layout, "MBR partitions",
                      "Set the label and size of each GRiD partition. At least one partition is required. Each can be at most 30 MiB.");
        m_table.setColumnCount(3);
        m_table.setRowCount(0);
        m_table.setHorizontalHeaderLabels({"#", "Label", "Size (MiB)"});
        m_table.verticalHeader()->setVisible(false);
        m_table.horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_table.horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_table.horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_table.setSelectionMode(QAbstractItemView::NoSelection);
        m_table.setEditTriggers(QAbstractItemView::NoEditTriggers);

        addPartitionRow();
        layout->addWidget(&m_table);

        auto* actions = new QHBoxLayout;
        actions->addStretch();
        m_addButton.setIcon(themedSvgIcon(":/resources/add.svg", m_addButton.palette()));
        m_addButton.setText("Add partition");
        m_addButton.setEnabled(m_table.rowCount() < kMaxPartitions);
        actions->addWidget(&m_addButton);
        layout->addLayout(actions);
        connect(&m_addButton, &QPushButton::clicked, this, [this] { addPartitionRow(); });
    }

    int nextId() const override { return -1; }

    QVector<int> partitionSizesMiB() const {
        QVector<int> sizes;
        sizes.reserve(m_table.rowCount());
        for (int row = 0; row < m_table.rowCount(); ++row) {
            sizes.append(sizeAt(row));
        }
        return sizes;
    }

    QVector<QString> partitionLabels() const {
        QVector<QString> labels;
        labels.reserve(m_table.rowCount());
        for (int row = 0; row < m_table.rowCount(); ++row) {
            labels.append(static_cast<QLineEdit*>(m_table.cellWidget(row, 1))->text());
        }
        return labels;
    }

private:
    void addPartitionRow() {
        const int row = m_table.rowCount();
        if (row >= kMaxPartitions) return;

        m_table.insertRow(row);
        auto* number = new QTableWidgetItem(QString::number(row + 1));
        number->setTextAlignment(Qt::AlignCenter);
        number->setFlags(number->flags() & ~Qt::ItemIsEditable);
        m_table.setItem(row, 0, number);

        auto* label = new QLineEdit(&m_table);
        label->setPlaceholderText("No label");
        label->setMaxLength(40);
        label->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("[\x00-\xFF]*")), label));
        m_table.setCellWidget(row, 1, label);

        auto* size = new QSpinBox(&m_table);
        size->setRange(1, kMaxHardDiskMiB);
        size->setValue(10);
        m_table.setCellWidget(row, 2, size);
        m_addButton.setEnabled(m_table.rowCount() < kMaxPartitions);
    }

    int sizeAt(int row) const {
        return static_cast<QSpinBox*>(m_table.cellWidget(row, 2))->value();
    }

    QTableWidget m_table;
    QPushButton m_addButton;
};



class LabelPage : public QWizardPage {
public:
    explicit LabelPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        configureWizardPage(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        addPageHeader(layout, "Disk label", "Optionally enter a label for the image.");
        layout->setSpacing(6);

        auto* label = new QLabel("Label:", this);
        m_label.setMaxLength(40);
        m_label.setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("[\x00-\xFF]*")), &m_label));
        layout->addWidget(label);
        layout->addWidget(&m_label);
        layout->addStretch();
    }

    QString label() const { return m_label.text(); }

private:
    QLineEdit m_label;
};

}  // namespace

ImageCreationWizard::ImageCreationWizard(QWidget* parent) : QWizard(parent) {
    setWindowTitle("Create GRiD-OS image");
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setOption(QWizard::IgnoreSubTitles, true);
    setPage(MediaPage, new ::MediaPage(this));
    setPage(FloppyPage, new ::FloppyPage(this));
    setPage(HardDiskLayoutPage, new ::HardDiskLayoutPage(this));
    setPage(HardDiskSizePage, new ::HardDiskSizePage(this));
    setPage(PartitionPage, new ::PartitionPage(this));
    setPage(LabelPage, new ::LabelPage(this));
    setStartId(MediaPage);
}

ImageCreationWizard::Result ImageCreationWizard::result() const {
    Result value;
    value.kind = static_cast<const ::MediaPage*>(page(MediaPage))->imageKind();
    value.floppyKind = static_cast<const ::FloppyPage*>(page(FloppyPage))->floppyKind();
    value.useMbr = static_cast<const ::HardDiskLayoutPage*>(page(HardDiskLayoutPage))->useMbr();
    value.sizeMiB = static_cast<const ::HardDiskSizePage*>(page(HardDiskSizePage))->sizeMiB();
    value.partitionSizesMiB = static_cast<const ::PartitionPage*>(page(PartitionPage))->partitionSizesMiB();
    if (value.kind == ImageKind::HardDisk && value.useMbr) {
        value.partitionLabels = static_cast<const ::PartitionPage*>(page(PartitionPage))->partitionLabels();
    } else {
        value.label = static_cast<const ::LabelPage*>(page(LabelPage))->label();
    }
    return value;
}
