#include "rename.h"
#include "ui_rename.h"
#include <QMessageBox>

Rename::Rename(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Rename)
{
    ui->setupUi(this);
}

QString Rename::getName(){
    return ui->lineEdit->text();
}

QString Rename::getType(){
    return ui->lineEdit_2->text();
}

void Rename::lockType(bool swch){
    dir = swch;
    ui->lineEdit_2->setDisabled(swch);
}

void Rename::setINFsect(QString text){
    ui->label->setText(text);
}

void Rename::setName(QString text){
    ui->lineEdit->setText(text);
}

void Rename::setType(QString text){
    ui->lineEdit_2->setText(text);
}

Rename::~Rename()
{
    delete ui;
}
