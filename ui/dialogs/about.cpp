#include "about.h"

AboutDlg::AboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg)
{
    ui->setupUi(this);
    ui->label_5->setText(_PVER_);
    connect(ui->label_7, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->label_8, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->label_9, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openRepo()));
}

void AboutDlg::openLink(QString link)
{
    QDesktopServices::openUrl(link);
}

void AboutDlg::openRepo()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Bs0Dd/GRiDISKCOM"));
}

AboutDlg::~AboutDlg()
{
    delete ui;
}
