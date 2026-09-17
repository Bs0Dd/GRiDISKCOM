#include "customdisk.h"
#include "diskconstants.h"
#include "ui_customdisk.h"

CustomDiskDlg::CustomDiskDlg(QWidget *parent, bool openMode) :
    QDialog(parent),
    ui(new Ui::CustomDiskDlg)
{
    ui->setupUi(this);

    HexFont = ui->spinBox->font();
    HexFont.setCapitalization(QFont::AllUppercase);
    ui->spinBox->setFont(HexFont);

    connect(ui->radioButton_5, SIGNAL(toggled(bool)), this, SLOT(SblFocus(bool)));
    connect(ui->radioButton_12, SIGNAL(toggled(bool)), this, SLOT(BmpFocus(bool)));
    connect(ui->radioButton_9, SIGNAL(toggled(bool)), this, SLOT(SizFocus(bool)));

    if (openMode) {
        ui->groupBox_3->setVisible(false);
        ui->groupBox_4->setVisible(false);
        setWindowTitle("Open Custom Disk Image");
        setFixedHeight(sizeHint().height());
    }
}

void CustomDiskDlg::SblFocus(bool activ) {
    ui->spinBox->setEnabled(activ);
    if (activ) {
        ui->spinBox->setFocus();
        ui->spinBox->selectAll();
    }
}

void CustomDiskDlg::SizFocus(bool activ) {
    ui->spinBox_2->setEnabled(activ);
    if (activ) {
        ui->spinBox_2->setFocus();
        ui->spinBox_2->selectAll();
    }
}

void CustomDiskDlg::BmpFocus(bool activ) {
    ui->spinBox_3->setEnabled(activ);
    if (activ) {
        ui->spinBox_3->setFocus();
        ui->spinBox_3->selectAll();
    }
}


void CustomDiskDlg::GetParams(uint16_t* sect, uint16_t* subl, uint16_t* bmp, uint16_t* isize, QString* labl) {
    *sect = ui->radioButton->isChecked() ? 512 : 256;

    if (ui->radioButton_3->isChecked()) {
        *subl = GRID_FLOPPY_SUPERBLOCK_FID;
    }
    else if (ui->radioButton_4->isChecked()) {
        *subl = GRID_BUBBLE_SUPERBLOCK_FID;
    }
    else if (ui->radioButton_13->isChecked()) {
        *subl = GRID_HDD_SUPERBLOCK_FID;
    }
    else {
        *subl = ui->spinBox->value();
    }

    if (bmp != NULL) {
        if (ui->radioButton_10->isChecked()) {
            *bmp = GRID_FLOPPY_BITMAP_FID;
        }
        else if (ui->radioButton_11->isChecked()) {
            *bmp = GRID_BUBBLE_BITMAP_FID;
        }
        else if (ui->radioButton_14->isChecked()) {
            *bmp = GRID_HDD_BITMAP_FID;
        }
        else {
            *bmp = ui->spinBox_3->value();
        }
    }

    if (isize != NULL) {
        if (ui->radioButton_6->isChecked()) {
            *isize = 360;
        }
        else if (ui->radioButton_7->isChecked()) {
            *isize = 384;
        }
        else if (ui->radioButton_8->isChecked()) {
            *isize = 720;
        }
        else {
            *isize = ui->spinBox_2->value();
        }
    }

    if (labl != NULL) {
        *labl = ui->lineEdit->text();
    }
}


CustomDiskDlg::~CustomDiskDlg()
{
    delete ui;
}
