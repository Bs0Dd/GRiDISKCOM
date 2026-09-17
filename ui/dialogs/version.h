#ifndef VERSIONDLG_H
#define VERSIONDLG_H

#include "ui_version.h"

#include <ccos_image/ccos_image.h>

namespace Ui {
class VersionDlg;
}

class VersionDlg : public QDialog
{
    Q_OBJECT

public:
    explicit VersionDlg(QWidget *parent = nullptr);
    void init(QString fname, ccos_version_t version);
    ccos_version_t retVer();
    ~VersionDlg();

private:
    Ui::VersionDlg *ui;
};

#endif // VERSIONDLG_H
