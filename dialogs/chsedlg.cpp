#include "chsedlg.h"

ChsDlg::ChsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChsDlg)
{
    ui->setupUi(this);
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

ChsDlg::~ChsDlg()
{
    delete ui;
}
