#ifndef CHSDLG_H
#define CHSDLG_H

#include "ui_chsedlg.h"

namespace Ui {
class ChsDlg;
}

class ChsDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ChsDlg(QWidget *parent = nullptr);
    void setInfo(QString text);
    void setName(QString text);
    void addItem(QString name);
    int getIndex();
    void enCheckBox();
    bool isChecked();
    ~ChsDlg();

private:
    Ui::ChsDlg *ui;
};

#endif // CHSDLG_H
