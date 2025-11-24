#include "abdlg.h"

AbDlg::AbDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AbDlg)
{
    ui->setupUi(this);
    ui->label_5->setText(_PVER_);
    connect(ui->label_7, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->label_8, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->label_9, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openRepo()));
}

void AbDlg::openLink(QString link)
{
    QDesktopServices::openUrl(link);
}

void AbDlg::openRepo()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Bs0Dd/GRiDISKCOM"));
}

AbDlg::~AbDlg()
{
    delete ui;
}
