#ifndef DATEDLG_H
#define DATEDLG_H

#include <QDialog>
#include <QMessageBox>
#include "ui_datedlg.h"

extern "C"{ //Include for ccos_date_t type
#include <ccos_image/ccos_private.h>
}

namespace Ui {
class DateDlg;
}

class DateDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DateDlg(QWidget *parent = nullptr);
    void init(QString fname, ccos_date_t cre, ccos_date_t mod, ccos_date_t exp);
    void retDates(ccos_date_t* cre, ccos_date_t* mod, ccos_date_t* exp);
    ~DateDlg();

private:
    Ui::DateDlg *ui;
};

#endif // DATEDLG_H
