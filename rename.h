#ifndef RENAME_H
#define RENAME_H

#include <QDialog>

namespace Ui {
class Rename;
}

class Rename : public QDialog
{
    Q_OBJECT

public:
    explicit Rename(QWidget *parent = nullptr);
    ~Rename();

private:
    Ui::Rename *ui;
};

#endif // RENAME_H
