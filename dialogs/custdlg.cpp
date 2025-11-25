#include "custdlg.h"
#include "ui_custdlg.h"

CustDlg::CustDlg(QWidget *parent, bool openMode) :
    QDialog(parent),
    ui(new Ui::CustDlg)
{
    ui->setupUi(this);

    HexFont = ui->spinBox->font();
    HexFont.setCapitalization(QFont::AllUppercase);
    ui->spinBox->setFont(HexFont);

    connect(ui->radioButton_5, SIGNAL(toggled(bool)), this, SLOT(SblFocus(bool)));
    connect(ui->radioButton_9, SIGNAL(toggled(bool)), this, SLOT(SizFocus(bool)));

    if (openMode) {
        ui->groupBox_3->setVisible(false);
        ui->groupBox_4->setVisible(false);
        CustDlg::setWindowTitle("Custom image opening");
        CustDlg::setMinimumHeight(245);
        CustDlg::setMaximumHeight(245);
    }
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


void CustDlg::GetParams(uint16_t* sect, uint16_t* subl, uint16_t* isize, QString* labl) {
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


CustDlg::~CustDlg()
{
    delete ui;
}


