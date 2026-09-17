#ifndef RENAMEDLG_H
#define RENAMEDLG_H

#include "ui_rename.h"

namespace Ui {
class RenameDlg;
}

class RenameDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDlg(QWidget *parent = nullptr, bool lockedType = false);
    QString getName();
    QString getType();
    void setInfo(QString text);
    void setName(QString text);
    void setType(QString text);
    ~RenameDlg();

private:
    Ui::RenameDlg *ui;
};

#endif // RENAMEDLG_H
