#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QInputDialog>
#include <vector>

#include <iostream>

extern "C"{
#include <ccos_image.h>
#include <ccos_private.h>
#include <common.h>
#include <wrapper.h>
#include <string_utils.h>
}

using namespace std;

vector<ccos_inode_t*> inodeon[2];
ccos_inode_t* curdir[2];
bool isch[2];

QString ccos_get_file_version_qstr(ccos_inode_t* file) {
  uint8_t major = file->version_major;
  uint8_t minor = file->version_minor;
  uint8_t patch = file->version_patch;
  QString ver = "%1.%2.%3";
  ver = ver.arg(major).arg(minor).arg(patch);
  return ver;
}

QString ccos_date_to_qstr(ccos_date_t date) {
  QString mzero = "";
  QString dzero = "";
  int year = date.year;
  int month = date.month;
  if (month < 10){
      mzero = "0";
  }
  int day = date.day;
  if (day < 10){
      dzero = "0";
  }
  QString qdat = "%1/%2%3/%4%5";
  qdat = qdat.arg(year).arg(mzero).arg(month).arg(dzero).arg(day);
  return qdat;
}

void drawempty(bool mod, QTableWidget* tableWidget){
    QTableWidgetItem *tFname, *tType, *tSize, *tVer, *tCreate, *tMod;
    tFname = new QTableWidgetItem();
    tType = new QTableWidgetItem();
    tSize = new QTableWidgetItem();
    tVer = new QTableWidgetItem();
    tCreate = new QTableWidgetItem();
    tMod = new QTableWidgetItem();
    if (mod == 0){
        tFname->setText("<EMPTY>");
    }
    else{
        tFname->setText("<EMPTY IMAGE>");
    }
    tFname->setFlags(tFname->flags() ^ Qt::ItemIsEditable);
    tType->setFlags(tType->flags() ^ Qt::ItemIsEditable);
    tSize->setFlags(tSize->flags() ^ Qt::ItemIsEditable);
    tVer->setFlags(tVer->flags() ^ Qt::ItemIsEditable);
    tCreate->setFlags(tCreate->flags() ^ Qt::ItemIsEditable);
    tMod->setFlags(tMod->flags() ^ Qt::ItemIsEditable);
    tableWidget->insertRow(0);
    tableWidget->setItem(0, 0, tFname);
    tableWidget->setItem(0, 1, tType);
    tableWidget->setItem(0, 2, tSize);
    tableWidget->setItem(0, 3, tVer);
    tableWidget->setItem(0, 4, tCreate);
    tableWidget->setItem(0, 5, tMod);
}

void doListTable(ccos_inode_t* directory, bool noRoot, uint8_t* dat, size_t siz, bool curdisk, Ui::MainWindow* ui){
    QTableWidget* tableWidget;
    QLabel* label;
    QGroupBox* box;
    QString disk, msg;
    inodeon[curdisk].clear();
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(directory, dat, &fils, &dirdata);
    if (curdisk == 0){
        tableWidget = ui->tableWidget;
        label = ui->label;
        box = ui->groupBox;
        disk = "I";
    }
    else{
        tableWidget = ui->tableWidget_2;
        label = ui->label_2;
        box = ui->groupBox_2;
        disk = "II";
    }
    for(int row= tableWidget->rowCount(); 0<=row; row--){
        tableWidget-> removeRow(row);
    }
    char* labd = ccos_get_image_label(dat, siz);
    if (strlen(labd) == 0){
        if (isch[curdisk] == 0){
        msg = "Disk %1 - No label";}
        else{
        msg = "Disk %1 - No label*";}
        box->setTitle(msg.arg(disk));
    }
    else{
        if (isch[curdisk] == 0){
        msg = "Disk %1 - %2";}
        else{
        msg = "Disk %1 - %2*";}
        box->setTitle(msg.arg(disk).arg(labd));
    }
char basename[CCOS_MAX_FILE_NAME];
char type[CCOS_MAX_FILE_NAME];
int fcount = 0;
QTableWidgetItem *tFname, *tType, *tSize, *tVer, *tCreate, *tMod;
if (noRoot == 1){
    tFname = new QTableWidgetItem();
    tType = new QTableWidgetItem();
    tSize = new QTableWidgetItem();
    tVer = new QTableWidgetItem();
    tCreate = new QTableWidgetItem();
    tMod = new QTableWidgetItem();
    tFname->setText("..");
    tFname->setFlags(tFname->flags() ^ Qt::ItemIsEditable);
    tType->setText("<PARENT-DIR>");
    tType->setFlags(tType->flags() ^ Qt::ItemIsEditable);
    tSize->setFlags(tSize->flags() ^ Qt::ItemIsEditable);
    tVer->setFlags(tVer->flags() ^ Qt::ItemIsEditable);
    tCreate->setFlags(tCreate->flags() ^ Qt::ItemIsEditable);
    tMod->setFlags(tMod->flags() ^ Qt::ItemIsEditable);
    tableWidget->insertRow(0);
    tableWidget->setItem(0, 0, tFname);
    tableWidget->setItem(0, 1, tType);
    tableWidget->setItem(0, 2, tSize);
    tableWidget->setItem(0, 3, tVer);
    tableWidget->setItem(0, 4, tCreate);
    tableWidget->setItem(0, 5, tMod);
    inodeon[curdisk].insert(inodeon[curdisk].begin(), 0x0000000);
    fcount = 1;
}
for(int c = 0; c < fils; c++){
    memset(basename, 0, CCOS_MAX_FILE_NAME);
    memset(type, 0, CCOS_MAX_FILE_NAME);
    ccos_parse_file_name(dirdata[c], basename, type, NULL, NULL);
    tFname = new QTableWidgetItem();
    tType = new QTableWidgetItem();
    tSize = new QTableWidgetItem();
    tVer = new QTableWidgetItem();
    tCreate = new QTableWidgetItem();
    tMod = new QTableWidgetItem();
    tFname->setText(basename);
    tFname->setFlags(tFname->flags() ^ Qt::ItemIsEditable);
    tType->setText(type);
    QString qtype = type;
    if (qtype.toLower() == "subject"){
        tType->setText(qtype + " <DIR>");
    }
    tType->setFlags(tType->flags() ^ Qt::ItemIsEditable);
    tSize->setText(QString::number(ccos_get_file_size(dirdata[c])));
    tSize->setFlags(tSize->flags() ^ Qt::ItemIsEditable);
    tVer->setText(ccos_get_file_version_qstr(dirdata[c]));
    tVer->setFlags(tVer->flags() ^ Qt::ItemIsEditable);
    tCreate->setText(ccos_date_to_qstr(ccos_get_creation_date(dirdata[c])));
    tCreate->setFlags(tCreate->flags() ^ Qt::ItemIsEditable);
    tMod->setText(ccos_date_to_qstr(ccos_get_mod_date(dirdata[c])));
    tMod->setFlags(tMod->flags() ^ Qt::ItemIsEditable);
    if (ccos_is_dir(dirdata[c])){
        inodeon[curdisk].insert(inodeon[curdisk].begin() + fcount,dirdata[c]);
        tableWidget->insertRow(fcount);
        tableWidget->setItem(fcount, 0, tFname);
        tableWidget->setItem(fcount, 1, tType);
        tableWidget->setItem(fcount, 2, tSize);
        tableWidget->setItem(fcount, 3, tVer);
        tableWidget->setItem(fcount, 4, tCreate);
        tableWidget->setItem(fcount, 5, tMod);
        fcount++;
    }
    else{
    inodeon[curdisk].push_back(dirdata[c]);
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);
    tableWidget->setItem(row, 0, tFname);
    tableWidget->setItem(row, 1, tType);
    tableWidget->setItem(row, 2, tSize);
    tableWidget->setItem(row, 3, tVer);
    tableWidget->setItem(row, 4, tCreate);
    tableWidget->setItem(row, 5, tMod);
    }
}
size_t free = ccos_calc_free_space(dat, siz);
msg = "Free space: %1 bytes.";
label->setText(msg.arg(free));
if (inodeon[curdisk].size()==0){
    drawempty(1, tableWidget);
    inodeon[curdisk].insert(inodeon[curdisk].begin(), 0x0000000);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    trace_init(1); //TRACE MESSAGES ENABLE

    QMainWindow::setWindowTitle(QString("GRiDISK Commander ")+_PVER_);
    ui->tableWidget->horizontalHeader()->resizeSection(0, 155);
    ui->tableWidget->horizontalHeader()->resizeSection(2, szcor);
    ui->tableWidget->horizontalHeader()->resizeSection(3, 45);
    ui->tableWidget->horizontalHeader()->resizeSection(4, 80);
    ui->tableWidget->horizontalHeader()->resizeSection(5, 80);
    ui->tableWidget_2->horizontalHeader()->resizeSection(0, 155);
    ui->tableWidget_2->horizontalHeader()->resizeSection(2, szcor);
    ui->tableWidget_2->horizontalHeader()->resizeSection(3, 45);
    ui->tableWidget_2->horizontalHeader()->resizeSection(4, 80);
    ui->tableWidget_2->horizontalHeader()->resizeSection(5, 80);
    drawempty(0, ui->tableWidget);
    drawempty(0, ui->tableWidget_2);
    diskfont.setFamily(QString::fromUtf8("Arial"));
    diskfont.setPointSize(9);
    diskfont.setUnderline(false);
    diskfont.setWeight(50);
    diskfont.setBold(true);
    ui->groupBox->setFont(diskfont);
    ui->tableWidget->setFont(diskfont);
    ui->groupBox_2->setFont(diskfont);
    ui->tableWidget_2->setFont(diskfont);
    diskfont.setBold(false);
    ui->groupBox_2->setFont(diskfont);
    ui->tableWidget_2->setFont(diskfont);
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_2->verticalHeader()->hide();
    ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->tableWidget, &QTableWidget::clicked, this, &MainWindow::setactive0);
    connect(ui->tableWidget_2, &QTableWidget::clicked, this, &MainWindow::setactive1);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::Openf);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::Save);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::MkDir);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::Closef);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &MainWindow::Ren);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::Delete);
    connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::Ext);
    connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::Extall);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::aboutShow);
    connect(ui->actionAbout_Qt, &QAction::triggered, this, &MainWindow::aboutQtShow);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::Openf);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::Save);
    connect(ui->actionSave_as, &QAction::triggered, this, &MainWindow::SaveAs);
    connect(ui->actionMake_dir, &QAction::triggered, this, &MainWindow::MkDir);
    connect(ui->actionChange_label, &QAction::triggered, this, &MainWindow::Label);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::Closef);
    connect(ui->actionRename, &QAction::triggered, this, &MainWindow::Ren);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::Delete);
    connect(ui->actionExtract, &QAction::triggered, this, &MainWindow::Ext);
    connect(ui->actionExtract_all, &QAction::triggered, this, &MainWindow::Extall);
    connect(ui->tableWidget, &QTableWidget::cellActivated, this, &MainWindow::enterDIR);
    connect(ui->tableWidget_2, &QTableWidget::cellActivated, this, &MainWindow::enterDIR);
    dat[0]= NULL;
    dat[1]= NULL;
    siz[0]= 0;
    siz[1]= 0;
}

void MainWindow::aboutShow(){
     abss = new about(this);
     abss->exec();
}

void MainWindow::aboutQtShow(){
     QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Do you want to save your changes?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    bool backdstat = acdisk;
    if (isch[0] == 1){
        msgBox.setText("The Disk I has been modified.");
        int ret = msgBox.exec();
        if (ret == QMessageBox::Save){
            acdisk = 0;
            MainWindow::Save();
            acdisk = backdstat;
        }
        else if (ret == QMessageBox::Cancel){
            event->ignore();
        }
    }
    if (isch[1] == 1){
        msgBox.setText("The Disk II has been modified.");
        int ret = msgBox.exec();
        if (ret == QMessageBox::Save){
            acdisk = 1;
            MainWindow::Save();
            acdisk = backdstat;
        }
        else if (ret == QMessageBox::Cancel){
            event->ignore();
        }
    }
}

void MainWindow::enterDIR(){
    QTableWidget* tw;
    if (isop[acdisk] == 1){
    if (acdisk ==0){
        tw= ui->tableWidget;
    }
    else{
        tw= ui->tableWidget_2;
    }
    QTableWidgetItem* called = tw->currentItem();
    ccos_inode_t* dir;
    dir = inodeon[acdisk][called -> row()];
    if (dir == 0x0000000 and nrot[acdisk]== 0){
        return;
    }
    if (called -> row() == 0 and nrot[acdisk]== 1){
        ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
        curdir[acdisk] = (ccos_get_parent_dir(curdir[acdisk], dat[acdisk]));
        if (curdir[acdisk] == root){
            nrot[acdisk] = 0;
        }
        doListTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
    else if (ccos_is_dir(dir)){
    curdir[acdisk] = dir;
    nrot[acdisk] = 1;
    doListTable(dir, nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    } 
}
}

void MainWindow::Ext(){
    QTableWidget* tw;
    if (isop[acdisk] == 1){
        if (acdisk ==0){
            tw= ui->tableWidget;
        }
        else{
            tw= ui->tableWidget_2;
        }
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0){
            return;
        }
        if (called.size() == 6 and inodeon[acdisk][called[0]->row()]==0x0000000){
            return;
        }
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == ""){
            return;
        }
        for (int t = 0; t< called.size(); t+=6){
            if (inodeon[acdisk][called[t]->row()]==0x0000000){
                continue;
            }
            if (ccos_is_dir(inodeon[acdisk][called[t]->row()])){
                dump_dir_to(name[acdisk].toStdString().c_str(), inodeon[acdisk][called[t]->row()],
                        dat[acdisk], todir.toStdString().c_str());
            }
            else{
                dump_file(todir.toStdString().c_str(), inodeon[acdisk][called[t]->row()], dat[acdisk]);
            }
        }
    }
}

void MainWindow::Delete(){
    QTableWidget* tw;
    if (isop[acdisk] == 1){
        QMessageBox msgBox;
        if (acdisk ==0){
            tw= ui->tableWidget;
        }
        else{
            tw= ui->tableWidget_2;
        }
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0){
            return;
        }
        if (called.size() == 6 and inodeon[acdisk][called[0]->row()]==0x0000000){
            return;
        }
        bool selpar = 0;
        for (int t = 0; t< called.size(); t+=6){
            if (inodeon[acdisk][called[t]->row()]==0x0000000){
                selpar = 1;
                break;
            }
        }
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to delete %1 file(s)?").arg((called.size()- selpar)/6));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        if (ret != QMessageBox::Yes){
            return;
        }
        for (int t = 0; t< called.size(); t+=6){
            if (inodeon[acdisk][called[t]->row()]==0x0000000){
                continue;
            }
            ccos_delete_file(dat[acdisk], siz[acdisk], inodeon[acdisk][called[t]->row()]);
        }
        isch[acdisk] = 1;
        doListTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::Label(){
    if (isop[acdisk] == 1){
        QString dsk;
        if (acdisk == 0){
            dsk= "I";
        }
        else{
            dsk= "II";
        }
        char* fname = ccos_get_image_label(dat[acdisk], siz[acdisk]);
        QString nameQ = QInputDialog::getText(this, tr("New label"),
                                         QString("Set new label for the disk %1:").arg(dsk), QLineEdit::Normal, fname);
        ccos_set_image_label(dat[acdisk], siz[acdisk], nameQ.toStdString().c_str());
        isch[acdisk]= 1;
        doListTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void  MainWindow::Ren(){
    if (isop[acdisk] == 1){
        ccos_inode_t* reninode;
        if (acdisk == 0){
            if (ui->tableWidget->currentRow() == -1){
                return;
            }
            reninode = inodeon[acdisk][ui->tableWidget->currentRow()];
        }
        else{
            if (ui->tableWidget_2->currentRow() == -1){
                return;
            }
            reninode = inodeon[acdisk][ui->tableWidget_2->currentRow()];
        }
        if (reninode == 0x0000000){
            return;
        }
        char basename[CCOS_MAX_FILE_NAME];
        char type[CCOS_MAX_FILE_NAME];
        memset(basename, 0, CCOS_MAX_FILE_NAME);
        memset(type, 0, CCOS_MAX_FILE_NAME);
        ccos_parse_file_name(reninode, basename, type, NULL, NULL);
        rnam = new Rename(this);
        rnam->setName(basename);
        rnam->setType(type);
        rnam->setINFsect((QString("Set new name and type for %1~%2~:").arg(basename).arg(type)));
        if (ccos_is_dir(reninode)){
            rnam->lockType(1);
        }
        bool ret = rnam->exec();
        if (ret == 1){
            if (rnam->getType().toLower() == "subject" and !ccos_is_dir(reninode)){
                QMessageBox msgBox;
                msgBox.critical(0,"Incorrect Type",
                                "Can't set directory type for file!");
                return;
            }
            QString newname = rnam->getName();
            QString newtype = rnam->getType();
            ccos_rename_file(reninode, newname.toStdString().c_str(), newtype.toStdString().c_str());
            isch[acdisk] = 1;
            doListTable(ccos_get_parent_dir(reninode, dat[acdisk]),
                                            nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
    }
}

void MainWindow::Extall(){
    if (isop[acdisk] == 1){
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == ""){
            return;
        }
        dump_image_to(name[acdisk].toStdString().c_str(), dat[acdisk], siz[acdisk], todir.toStdString().c_str());
    }
}

void MainWindow::Save(){
    QGroupBox* gb;
    if (isch[acdisk] == 1){
        if (acdisk ==0){
            gb= ui->groupBox;
        }
        else{
            gb= ui->groupBox_2;
        }
        save_image(name[acdisk].toStdString().c_str(), dat[acdisk], siz[acdisk], true);
        isch[acdisk] = 0;
       gb->setTitle(gb->title().left(gb->title().size()-1));
    }
}

void MainWindow::SaveAs(){
    QGroupBox* gb;
    if (isop[acdisk] == 1){
        if (acdisk ==0){
            gb= ui->groupBox;
        }
        else{
            gb= ui->groupBox_2;
        }
        QString nameQ = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
        if (nameQ == ""){
            return;
        }
        save_image(nameQ.toStdString().c_str(), dat[acdisk], siz[acdisk], true);
        name[acdisk] = nameQ;
        if (isch[acdisk] == 1){
            gb->setTitle(gb->title().left(gb->title().size()-1));
            isch[acdisk] = 0;
        }
    }
}

void MainWindow::MkDir(){
    if (isop[acdisk] == 1){
        if (nrot[acdisk] == 1){
            QMessageBox msgBox;
            msgBox.critical(0,"Make dir",
                            "Sorry, but GRiD support directories only in root!");
            return;
        }
        QString name = QInputDialog::getText(this, tr("Make dir"),
                                             tr("New directory name:"), QLineEdit::Normal,"");
        if (name == ""){
            return;
        }
        ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
        ccos_create_dir(root, name.toStdString().c_str(), dat[acdisk], siz[acdisk]);
        isch[acdisk] = 1;
        doListTable(root, nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::Openf(){
    QString Qname = QFileDialog::getOpenFileName(this, "Open Image", "", "GRiD Image Files (*.img *.IMG)");
    read_file(Qname.toStdString().c_str(), &dat[acdisk], &siz[acdisk]);
    if (Qname == ""){
        return;
    }
    if(ccos_get_root_dir(dat[acdisk], siz[acdisk]) == NULL){
        QMessageBox msgBox;
        msgBox.critical(0,"Incorrect Image File",
                        "<html><head/><body>"
                        "<p align=\"center\">Image broken or have non-GRiD format!</p>"
                        "<p align=\"center\">Keep in mind, that Bubble Memory and Hard Drive images is not supported now!</p>"
                        "</body></html>");
        return;
    }
    inodeon[acdisk].clear();
    isch[acdisk] = 0;
    isop[acdisk] = 1;
    name[acdisk] = Qname;
    ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
    curdir[acdisk] = root;
    doListTable(root, 0, dat[acdisk], siz[acdisk], acdisk, ui);
}

void MainWindow::Closef(){
    if (isch[acdisk] == 1){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        if (acdisk == 0){
            msgBox.setText("The Disk I has been modified.");
        }
        else{
            msgBox.setText("The Disk II has been modified.");
        }
        int ret = msgBox.exec();
        if (ret == QMessageBox::Save){
            MainWindow::Save();
        }
        else if (ret == QMessageBox::Cancel){
            return;
        }
    }

    QTableWidget* tableWidget;
    inodeon[acdisk].clear();
    isch[acdisk] = 0;
    isop[acdisk] = 0;
    name[acdisk] = "";
    dat[acdisk] = NULL;
    siz[acdisk] = 0;
    if (acdisk == 0){
        tableWidget = ui->tableWidget;
        ui->label->setText("Free space:");
        ui->groupBox->setTitle("Disk I - No disk");
    }
    else{
        tableWidget = ui->tableWidget_2;
        ui->label_2->setText("Free space:");
        ui->groupBox_2->setTitle("Disk II - No disk");
    }
    for(int row= tableWidget->rowCount(); 0<=row; row--){
        tableWidget-> removeRow(row);
    }
    drawempty(0, tableWidget);
}

void MainWindow::setactive0()
{
    if (acdisk == 1){
        diskfont.setBold(false);
        ui->groupBox_2->setFont(diskfont);
        ui->tableWidget_2->setFont(diskfont);
        diskfont.setBold(true);
        ui->groupBox->setFont(diskfont);
        ui->tableWidget->setFont(diskfont);
        acdisk = 0;
    }
}

void MainWindow::setactive1()
{
    if (acdisk == 0){
        diskfont.setBold(false);
        ui->groupBox->setFont(diskfont);
        ui->tableWidget->setFont(diskfont);
        diskfont.setBold(true);
        ui->groupBox_2->setFont(diskfont);
        ui->tableWidget_2->setFont(diskfont);
        acdisk = 1;
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}

