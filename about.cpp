#include "about.h"
#include "ui_about.h"
#include <QDesktopServices>
#include <QUrl>

about::about(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::about)
{
    ui->setupUi(this);
    ui->label_5->setText(_PVER_);
    connect(ui->pushButton, &QPushButton::clicked, this, &about::openrepo);
}

void about::openrepo()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Bs0Dd/GRiDISKCOM"));
}

about::~about()
{
    delete ui;
}
