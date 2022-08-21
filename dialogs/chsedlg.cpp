#include "chsedlg.h"

ChsDlg::ChsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChsDlg)
{
    ui->setupUi(this);
    ui->checkBox->hide();
}

void ChsDlg::setInfo(QString text){
    ui->label->setText(text);
}

void ChsDlg::setName(QString text){
    ChsDlg::setWindowTitle(text);
}

void ChsDlg::addItem(QString name){
    ui->comboBox->addItem(name);
}

int ChsDlg::getIndex(){
    return ui->comboBox->currentIndex();
}

void ChsDlg::enCheckBox(){
    ui->checkBox->show();
}

bool ChsDlg::isChecked(){
    return ui->checkBox->isChecked();
}

ChsDlg::~ChsDlg()
{
    delete ui;
}
