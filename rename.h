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
    bool dir = 0;
    QString getName();
    QString getType();
    void lockType(bool swch);
    void setINFsect(QString text);
    void setName(QString text);
    void setType(QString text);
    ~Rename();

private:
    Ui::Rename *ui;
};

#endif // RENAME_H
