#include "version.h"

VersionDlg::VersionDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VersionDlg)
{
    ui->setupUi(this);
}

void VersionDlg::init(QString fname, ccos_version_t version){
    ui->label->setText(ui->label->text().arg(fname));
    ui->major->setValue(version.major);
    ui->minor->setValue(version.minor);
    ui->patch->setValue(version.patch);
}

ccos_version_t VersionDlg::retVer(){
    return ccos_version_t{static_cast<uint8_t>(ui->major->value()),
                static_cast<uint8_t>(ui->minor->value()), static_cast<uint8_t>(ui->patch->value())};
}

VersionDlg::~VersionDlg()
{
    delete ui;
}
