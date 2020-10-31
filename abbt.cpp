#include "abbt.h"
#include "ui_abbt.h"

abbt::abbt(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::abbt)
{
    ui->setupUi(this);
}

abbt::~abbt()
{
    delete ui;
}
