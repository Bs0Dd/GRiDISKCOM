#ifndef ABDLG_H
#define ABDLG_H

#include <QDesktopServices>
#include <QUrl>
#include "ui_abdlg.h"

#define _PVER_ "0.28-beta"

namespace Ui {
class AbDlg;
}

class AbDlg : public QDialog
{
    Q_OBJECT

public slots:
    void openrepo();
public:
    explicit AbDlg(QWidget *parent = nullptr);
    ~AbDlg();

private:
    Ui::AbDlg *ui;
};

#endif // ABDLG_H
