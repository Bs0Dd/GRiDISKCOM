#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QDesktopServices>

#define _PVER_ "v0.15-beta"

namespace Ui {
class about;
}

class about : public QDialog
{
    Q_OBJECT

public:
    explicit about(QWidget *parent = nullptr);
    ~about();
    void openrepo();

private:
    Ui::about *ui;
};

#endif // ABBT_H
