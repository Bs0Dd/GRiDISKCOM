#ifndef CUSTOMDISKDLG_H
#define CUSTOMDISKDLG_H

#include <QDialog>

namespace Ui {
class CustomDiskDlg;
}

class CustomDiskDlg : public QDialog
{
    Q_OBJECT

public slots:
    void SblFocus(bool activ);
    void BmpFocus(bool activ);
    void SizFocus(bool activ);

public:
    explicit CustomDiskDlg(QWidget *parent = nullptr, bool openMode = false);
    ~CustomDiskDlg();
    void GetParams(uint16_t* sect, uint16_t* subl, uint16_t* bmp, uint16_t* isize, QString* labl);

private:
    Ui::CustomDiskDlg *ui;
    QFont HexFont;
};

#endif // CUSTOMDISKDLG_H
