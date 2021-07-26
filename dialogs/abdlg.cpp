#include "abdlg.h"

AbDlg::AbDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AbDlg)
{
    ui->setupUi(this);
    ui->label_5->setText(_PVER_);
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openrepo()));
}

void AbDlg::openrepo()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Bs0Dd/GRiDISKCOM"));
}

AbDlg::~AbDlg()
{
    delete ui;
}
