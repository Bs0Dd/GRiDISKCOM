#include "abbt.h"
#include "ui_abbt.h"
#include <QDesktopServices>
#include <QUrl>

abbt::abbt(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::abbt)
{
    ui->setupUi(this);
    ui->label_5->setText(_PVER_);
    connect(ui->pushButton, &QPushButton::clicked, this, &abbt::openrepo);
}

void abbt::openrepo()
{
  QDesktopServices::openUrl(QUrl("https://github.com/Bs0Dd/GRiDISKCOM"));
}

abbt::~abbt()
{
    delete ui;
}
