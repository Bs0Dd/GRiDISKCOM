#include "datedlg.h"

DateDlg::DateDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DateDlg)
{
    ui->setupUi(this);
}

void DateDlg::init(QString fname, ccos_date_t cre, ccos_date_t mod, ccos_date_t exp){
    ui->label->setText(ui->label->text().arg(fname));
    ui->creBox->setDate(QDate(cre.year, cre.month, cre.day));
    ui->modBox->setDate(QDate(mod.year, mod.month, mod.day));
    ui->expBox->setDate(QDate(exp.year, exp.month, exp.day));
}

void DateDlg::retDates(ccos_date_t* cre, ccos_date_t* mod, ccos_date_t* exp){
    QDate Qcre = ui->creBox->date();
    QDate Qmod = ui->modBox->date();
    QDate Qexp = ui->expBox->date();
    if (Qcre.year() == 1752 && Qcre.month() == 9 && Qcre.day() == 14){
        cre->year = 0; cre->month = 0; cre->day = 0;
    }
    else{
        cre->year = Qcre.year(); cre->month = Qcre.month(); cre->day = Qcre.day();
    }
    if (Qmod.year() == 1752 && Qmod.month() == 9 && Qmod.day() == 14){
        mod->year = 0; mod->month = 0; mod->day = 0;
    }
    else{
        mod->year = Qmod.year(); mod->month = Qmod.month(); mod->day = Qmod.day();
    }
    if (Qexp.year() == 1752 && Qexp.month() == 9 && Qexp.day() == 14){
        exp->year = 0; exp->month = 0; exp->day = 0;
    }
    else{
        exp->year = Qexp.year(); exp->month = Qexp.month(); exp->day = Qexp.day();
    }
}

DateDlg::~DateDlg()
{
    delete ui;
}
