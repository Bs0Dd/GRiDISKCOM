#include "rename.h"

RenameDlg::RenameDlg(QWidget *parent, bool lockedType) :
    QDialog(parent),
    ui(new Ui::RenameDlg)
{
    ui->setupUi(this);
    ui->lineEdit_2->setDisabled(lockedType);
}

QString RenameDlg::getName(){
    return ui->lineEdit->text();
}

QString RenameDlg::getType(){
    return ui->lineEdit_2->text();
}

void RenameDlg::setInfo(QString text){
    ui->label->setText(text);
}

void RenameDlg::setName(QString text){
    ui->lineEdit->setText(text);
}

void RenameDlg::setType(QString text){
    ui->lineEdit_2->setText(text);
}

RenameDlg::~RenameDlg()
{
    delete ui;
}
