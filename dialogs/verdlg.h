#ifndef VERDLG_H
#define VERDLG_H

#include "ui_verdlg.h"

#include <ccos_image/ccos_image.h>

namespace Ui {
class VerDlg;
}

class VerDlg : public QDialog
{
    Q_OBJECT

public:
    explicit VerDlg(QWidget *parent = nullptr);
    void init(QString fname, version_t version);
    version_t retVer();
    ~VerDlg();

private:
    Ui::VerDlg *ui;
};

#endif // VERDLG_H
