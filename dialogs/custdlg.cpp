#include "custdlg.h"
#include "ui_custdlg.h"

CustDlg::CustDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustDlg)
{
    ui->setupUi(this);

    HexFont = ui->spinBox->font();
    HexFont.setCapitalization(QFont::AllUppercase);
    ui->spinBox->setFont(HexFont);

    connect(ui->radioButton_5, SIGNAL(toggled(bool)), this, SLOT(SblFocus(bool)));
    connect(ui->radioButton_9, SIGNAL(toggled(bool)), this, SLOT(SizFocus(bool)));
}

void CustDlg::SblFocus(bool activ) {
    ui->spinBox->setEnabled(activ);
    if (activ) {
        ui->spinBox->setFocus();
        ui->spinBox->selectAll();
    }
}

void CustDlg::SizFocus(bool activ) {
    ui->spinBox_2->setEnabled(activ);
    if (activ) {
        ui->spinBox_2->setFocus();
        ui->spinBox_2->selectAll();
    }
}


void CustDlg::GetParams(uint16_t* isize, uint16_t* sect, uint16_t* subl) {
    *sect = ui->radioButton->isChecked() ? 512 : 256;

    if (ui->radioButton_3->isChecked()) {
        *subl = 0x121;
    }
    else if (ui->radioButton_4->isChecked()) {
        *subl = 0x3FE;
    }
    else {
        *subl = ui->spinBox->value();
    }

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
        *isize = ui->spinBox_2->value() * 1024;
    }
}

void CustDlg::SetParams(uint16_t isize, uint16_t sect, uint16_t subl) {
    ui->radioButton->setChecked(sect == 512);
    ui->radioButton_2->setChecked(sect == 256);

    ui->radioButton_3->setChecked(subl == 0x121);
    ui->radioButton_4->setChecked(subl == 0x3FE);
    if ((subl != 0x121) && (subl != 0x3FE)) {
        ui->radioButton_5->setChecked(true);
        ui->spinBox->setValue(subl);
    }

    ui->radioButton_6->setChecked(isize == 360);
    ui->radioButton_7->setChecked(isize == 384);
    ui->radioButton_8->setChecked(isize == 720);
    if ((isize != 360) && (isize != 384) && (isize != 720)) {
        ui->radioButton_9->setChecked(true);
        ui->spinBox_2->setValue(subl);
    }
}


CustDlg::~CustDlg()
{
    delete ui;
}


