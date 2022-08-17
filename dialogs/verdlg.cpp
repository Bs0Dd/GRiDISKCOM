#include "verdlg.h"

VerDlg::VerDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VerDlg)
{
    ui->setupUi(this);
}

void VerDlg::init(QString fname, version_t version){
    ui->label->setText(ui->label->text().arg(fname));
    ui->major->setValue(version.major);
    ui->minor->setValue(version.minor);
    ui->patch->setValue(version.patch);
}

version_t VerDlg::retVer(){
    return version_t{static_cast<uint8_t>(ui->major->value()),
                static_cast<uint8_t>(ui->minor->value()), static_cast<uint8_t>(ui->patch->value())};
}

VerDlg::~VerDlg()
{
    delete ui;
}
