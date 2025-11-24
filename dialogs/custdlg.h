#ifndef CUSTDLG_H
#define CUSTDLG_H

#include <QDialog>

namespace Ui {
class CustDlg;
}

class CustDlg : public QDialog
{
    Q_OBJECT

public slots:
    void SblFocus(bool activ);
    void SizFocus(bool activ);

public:
    explicit CustDlg(QWidget *parent = nullptr);
    ~CustDlg();
    void GetParams(uint16_t* isize, uint16_t* sect, uint16_t* subl);
    void SetParams(uint16_t isize, uint16_t sect, uint16_t subl);
    void OpenMode();

private:
    Ui::CustDlg *ui;
    QFont HexFont;
};

#endif // CUSTDLG_H
