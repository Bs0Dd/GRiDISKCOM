#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDesktopServices>
#include <QUrl>
#include "ui_about.h"

#include "appversion.h"

namespace Ui {
class AboutDlg;
}

class AboutDlg : public QDialog
{
    Q_OBJECT

public slots:
    void openLink(QString link);
    void openRepo();
public:
    explicit AboutDlg(QWidget *parent = nullptr);
    ~AboutDlg();

private:
    Ui::AboutDlg *ui;
};

#endif // ABOUTDLG_H
