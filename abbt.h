#ifndef ABBT_H
#define ABBT_H

#include <QDialog>
#include <QDesktopServices>

#define _PVER_ "v0.15-beta"

namespace Ui {
class abbt;
}

class abbt : public QDialog
{
    Q_OBJECT

public:
    explicit abbt(QWidget *parent = nullptr);
    ~abbt();
    void openrepo();

private:
    Ui::abbt *ui;
};

#endif // ABBT_H
