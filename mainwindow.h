#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int acdisk = 0;
    QFont diskfont;
    void setactive0();
    void setactive1();
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
