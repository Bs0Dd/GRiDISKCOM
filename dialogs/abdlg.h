#ifndef ABDLG_H
#define ABDLG_H

#include <QDialog>

#define _PVER_ "v0.15-beta"

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
