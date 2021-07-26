#include "rendlg.h"

RenDlg::RenDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RenDlg)
{
    ui->setupUi(this);
}

QString RenDlg::getName(){
    return ui->lineEdit->text();
}

QString RenDlg::getType(){
    return ui->lineEdit_2->text();
}

void RenDlg::lockType(bool swch){
    ui->lineEdit_2->setDisabled(swch);
}

void RenDlg::setInfo(QString text){
    ui->label->setText(text);
}

void RenDlg::setName(QString text){
    ui->lineEdit->setText(text);
}

void RenDlg::setType(QString text){
    ui->lineEdit_2->setText(text);
}

RenDlg::~RenDlg()
{
    delete ui;
}
