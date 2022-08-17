#ifndef RENDLG_H
#define RENDLG_H

#include "ui_rendlg.h"

namespace Ui {
class RenDlg;
}

class RenDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RenDlg(QWidget *parent = nullptr);
    QString getName();
    QString getType();
    void lockType(bool swch);
    void setInfo(QString text);
    void setName(QString text);
    void setType(QString text);
    ~RenDlg();

private:
    Ui::RenDlg *ui;
};

#endif // RENDLG_H
