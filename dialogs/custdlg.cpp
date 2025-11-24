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

CustDlg::~CustDlg()
{
    delete ui;
}


