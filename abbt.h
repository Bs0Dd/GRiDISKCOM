#ifndef ABBT_H
#define ABBT_H

#include <QDialog>

namespace Ui {
class abbt;
}

class abbt : public QDialog
{
    Q_OBJECT

public:
    explicit abbt(QWidget *parent = nullptr);
    ~abbt();

private:
    Ui::abbt *ui;
};

#endif // ABBT_H
