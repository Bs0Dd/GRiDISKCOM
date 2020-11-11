#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
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

vector<ccos_inode_t*> inodeon0, inodeon1;
ccos_inode_t* curdir0, *curdir1;
int isch0 = 0;
int isch1 = 0;

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

void doListTable(ccos_inode_t** dirdata, int fils, int noRoot, uint8_t* dat, size_t siz, int curdisk, char* labd, Ui::MainWindow* ui){
    vector<ccos_inode_t*> inodact;
    QTableWidget* tableWidget;
    QLabel* label;
    QGroupBox* box;
    QString disk, msg;
    int isch;
    if (curdisk == 0){
        tableWidget = ui->tableWidget;
        label = ui->label;
        box = ui->groupBox;
        disk = "I";
        isch = isch0;
    }
    else{
        tableWidget = ui->tableWidget_2;
        label = ui->label_2;
        box = ui->groupBox_2;
        disk = "II";
        isch = isch0;
    }
    for(int row= tableWidget->rowCount(); 0<=row; row--){
        tableWidget-> removeRow(row);
    }
    if (strlen(labd) == 0){
        if (isch == 0){
        msg = "Disk %1 - No label";}
        else{
        msg = "Disk %1 - No label*";}
        box->setTitle(msg.arg(disk));
    }
    else{
        if (isch == 0){
        msg = "Disk %1 -%2";}
        else{
        msg = "Disk %1 -%2*";}
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
    inodact.insert(inodact.begin(), 0x0000000);
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
    if (qtype == "subject"){
        tType->setText("subject <DIR>");;
    }
    if (qtype == "Subject"){
        tType->setText("Subject <DIR>");;
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
        inodact.insert(inodact.begin() + fcount,dirdata[c]);
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
    inodact.push_back(dirdata[c]);
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
if (curdisk == 0){
    inodeon0=inodact;
}
else if (curdisk == 1){
    inodeon1=inodact;
}
size_t free = ccos_calc_free_space(dat, siz);
msg = "Free space: %1 bytes.";
label->setText(msg.arg(free));
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
    connect(ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);
    connect(ui->tableWidget, &QTableWidget::clicked, this, &MainWindow::setactive0);
    connect(ui->tableWidget_2, &QTableWidget::clicked, this, &MainWindow::setactive1);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::Openf);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::Save);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::MkDir);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::Closef);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::Delete);
    connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::Extall);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::aboutShow);
    connect(ui->actionAbout_Qt, &QAction::triggered, this, &MainWindow::aboutQtShow);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::Openf);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::Save);
    connect(ui->actionSave_as, &QAction::triggered, this, &MainWindow::SaveAs);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::Closef);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::Delete);
    connect(ui->actionExtract_all, &QAction::triggered, this, &MainWindow::Extall);
    connect(ui->tableWidget, &QTableWidget::cellActivated, this, &MainWindow::enterDIR0);
    connect(ui->tableWidget_2, &QTableWidget::cellActivated, this, &MainWindow::enterDIR1);
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
    QMainWindow::closeEvent(event);
}

void MainWindow::enterDIR0(){
    QTableWidgetItem* called = ui->tableWidget->currentItem();
    ccos_inode_t* dir;
    dir = inodeon0[called -> row()];
    if (called -> row() == 0 and nrot0== 1){
        ccos_inode_t* root = ccos_get_root_dir(dat0, siz0);
        char* fname = short_string_to_string(ccos_get_file_name(root));
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        curdir0 = (ccos_get_parent_dir(curdir0, dat0));
        ccos_get_dir_contents(curdir0, dat0, &fils, &dirdata);
        if (curdir0 == root){
            nrot0 = 0;
        }
        doListTable(dirdata, fils, nrot0, dat0, siz0, 0, fname, ui);
    }
    else if (ccos_is_dir(dir)){
    curdir0 = dir;
    ccos_inode_t* root = ccos_get_root_dir(dat0, siz0);
    char* fname = short_string_to_string(ccos_get_file_name(root));
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(dir, dat0, &fils, &dirdata);
    nrot0 = 1;
    doListTable(dirdata, fils, nrot0, dat0, siz0, 0, fname, ui);
    } 
}

void MainWindow::enterDIR1(){
    QTableWidgetItem* called = ui->tableWidget_2->currentItem();
    ccos_inode_t* dir;
    dir = inodeon1[called -> row()];
    if (called -> row() == 0 and nrot1== 1){
        ccos_inode_t* root = ccos_get_root_dir(dat1, siz1);
        char* fname = short_string_to_string(ccos_get_file_name(root));
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        curdir1 = (ccos_get_parent_dir(curdir1, dat1));
        ccos_get_dir_contents(curdir1, dat1, &fils, &dirdata);
        if (curdir1 == root){
            nrot1 = 0;
        }
        doListTable(dirdata, fils, nrot1, dat1, siz1, 1, fname, ui);
    }
    else if (ccos_is_dir(dir)){
    curdir1 = dir;
    ccos_inode_t* root = ccos_get_root_dir(dat1, siz1);
    char* fname = short_string_to_string(ccos_get_file_name(root));
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(dir, dat1, &fils, &dirdata);
    nrot1 = 1;
    doListTable(dirdata, fils, nrot1, dat1, siz1, 1, fname, ui);
    }
}

void MainWindow::Delete(){
    if (acdisk == 0 and isop0 == 1){
        QList<QTableWidgetItem *> called = ui->tableWidget->selectedItems();
        for (int t = 0; t< called.size(); t+=6){
            if (inodeon0[called[t]->row()]==0x0000000){
                continue;
            }
            ccos_delete_file(dat0, siz0, inodeon0[called[t]->row()]);
        }
        ccos_inode_t* root = ccos_get_root_dir(dat0, siz0);
        char* fname = short_string_to_string(ccos_get_file_name(root));
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        isch0 = 1;
        ccos_get_dir_contents(curdir0, dat0, &fils, &dirdata);
        doListTable(dirdata, fils, nrot0, dat0, siz0, 0, fname, ui);
    }
    else if (acdisk == 1 and isop1 == 1){
        QList<QTableWidgetItem *> called = ui->tableWidget_2->selectedItems();
        for (int t = 0; t< called.size(); t+=6){
            if (inodeon1[called[t]->row()]==0x0000000){
                continue;
            }
            ccos_delete_file(dat1, siz1, inodeon1[called[t]->row()]);
        }
        ccos_inode_t* root = ccos_get_root_dir(dat1, siz1);
        char* fname = short_string_to_string(ccos_get_file_name(root));
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        isch1 = 1;
        ccos_get_dir_contents(curdir1, dat1, &fils, &dirdata);
        doListTable(dirdata, fils, nrot1, dat1, siz1, 1, fname, ui);
    }
}

void MainWindow::Extall(){
    uint8_t* dat = NULL;
    size_t siz = 0;
    QString name, todir;

    if (acdisk == 0){
        todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == ""){
            return;
        }
        dat = dat0;
        siz = siz0;
        name = name0;
    }
    else{
        todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == ""){
            return;
        }
        dat = dat1;
        siz = siz1;
        name = name1;
    }
    dump_image_to(name.toStdString().c_str(), dat, siz, todir.toStdString().c_str());
}

void MainWindow::Save(){
    if (acdisk == 0 and isch0 == 1){
        save_image(name0.toStdString().c_str(), dat0, siz0, true);
        isch0 = 0;
        ui->groupBox->setTitle(ui->groupBox->title().left(ui->groupBox->title().size()-1));
    }
    else if (acdisk == 1 and isch1 == 1){
        save_image(name1.toStdString().c_str(), dat1, siz1, true);
        isch1 = 0;
        ui->groupBox_2->setTitle(ui->groupBox_2->title().left(ui->groupBox_2->title().size()-1));
    }
}

void MainWindow::SaveAs(){
    if (acdisk == 0 and isop0 == 1){
        QString name = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
        if (name == ""){
            return;
        }
        save_image(name.toStdString().c_str(), dat0, siz0, true);
        name0 = name;
        isch0 = 0;
        ui->groupBox->setTitle(ui->groupBox->title().left(ui->groupBox->title().size()-1));
    }
    else if (acdisk == 1 and isop1 == 1){
        QString name = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
        if (name == ""){
            return;
        }
        save_image(name.toStdString().c_str(), dat1, siz1, true);
        name1 = name;
        isch1 = 0;
        ui->groupBox_2->setTitle(ui->groupBox_2->title().left(ui->groupBox_2->title().size()-1));
    }
}

void MainWindow::MkDir(){
    if (acdisk == 0 and isop0 == 1){
        QString name = QInputDialog::getText(this, tr("Make dir"),
                                             tr("New directory name:"), QLineEdit::Normal,"");
        if (name == ""){
            return;
        }
        ccos_inode_t* root = ccos_get_root_dir(dat0, siz0);
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        ccos_create_dir(curdir0, name.toStdString().c_str(), dat0, siz0);
        char* fname = short_string_to_string(ccos_get_file_name(root));
        ccos_get_dir_contents(curdir0, dat0, &fils, &dirdata);
        isch0 = 1;
        doListTable(dirdata, fils, nrot0, dat0, siz0, 0, fname, ui);
    }
    else if (acdisk == 1 and isop1 == 1){
        QString name = QInputDialog::getText(this, tr("Make dir"),
                                             tr("New directory name:"), QLineEdit::Normal,"");
        if (name == ""){
            return;
        }
        ccos_inode_t* root = ccos_get_root_dir(dat1, siz1);
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        ccos_create_dir(curdir1, name.toStdString().c_str(), dat1, siz1);
        char* fname = short_string_to_string(ccos_get_file_name(root));
        ccos_get_dir_contents(curdir1, dat1, &fils, &dirdata);
        isch0 = 1;
        doListTable(dirdata, fils, nrot1, dat1, siz1, 0, fname, ui);
    }
}

void MainWindow::Openf(){
    QString name = QFileDialog::getOpenFileName(this, "Open Image", "", "GRiD Image Files (*.img *.IMG)");
    uint8_t* dat = NULL;
    size_t siz = 0;
    read_file(name.toStdString().c_str(), &dat, &siz);
    if (name == ""){
        return;
    }
    if(ccos_get_root_dir(dat, siz) == NULL){
        QMessageBox msgBox;
        msgBox.critical(0,"Incorrect Image File",
                        "<html><head/><body>"
                        "<p align=\"center\">Image broken or have non-GRiD format!</p>"
                        "<p align=\"center\">Keep in mind, that Bubble Memory and Hard Drive images is not supported now!</p>"
                        "</body></html>");
        return;
    }
    int panel = 0;
    if (isop0 == 1 and isop1 == 1){
        panel = acdisk;
        if(acdisk == 0){
            isch0 = 0;
            name0 = name;
            dat0 = dat;
            siz0 = siz;
            inodeon0.clear();

        }
        else{
            isch1 = 0;
            name1 = name;
            dat1 = dat;
            siz1 = siz;
            inodeon1.clear();
        }
    }
    else if (isop0 == 1){
        inodeon1.clear();
        dat1 = dat;
        siz1 = siz;
        panel = 1;
        isop1 = 1;
        isch1 = 0;
        name1 = name;
    }
    else if (isop0 == 0) {
        inodeon0.clear();
        dat0 = dat;
        siz0 = siz;
        isop0 = 1;
        isch0 = 0;
        name0 = name;
    }
    ccos_inode_t* root = ccos_get_root_dir(dat, siz);
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(root, dat, &fils, &dirdata);
    char* fname = short_string_to_string(ccos_get_file_name(root));
    if (panel == 0){
        curdir0 = root;
    }
    else{
        curdir1 = root;
    }
    doListTable(dirdata, fils, 0, dat, siz, panel, fname, ui);
}

void MainWindow::Closef(){
    QTableWidget* tableWidget;
    if (acdisk == 0){
        tableWidget = ui->tableWidget;
        inodeon0.clear();
        isop0 = 0;
        isch0 = 0;
        name0 = "";
        dat0 = NULL;
        siz0 = 0;
        ui->label->setText("Free space:");
        ui->groupBox->setTitle("Disk I - No disk");
    }
    else{
        tableWidget = ui->tableWidget_2;
        inodeon1.clear();
        isop1 = 0;
        isch1 = 0;
        name1 = "";
        dat1 = NULL;
        siz1 = 0;
        ui->label_2->setText("Free space:");
        ui->groupBox_2->setTitle("Disk II - No disk");
    }
    for(int row= tableWidget->rowCount(); 0<=row; row--){
        tableWidget-> removeRow(row);
    }

}

void MainWindow::setactive0()
{
    diskfont.setBold(false);
    ui->groupBox_2->setFont(diskfont);
    ui->tableWidget_2->setFont(diskfont);
    diskfont.setBold(true);
    ui->groupBox->setFont(diskfont);
    ui->tableWidget->setFont(diskfont);
    acdisk = 0;
}

void MainWindow::setactive1()
{
    diskfont.setBold(false);
    ui->groupBox->setFont(diskfont);
    ui->tableWidget->setFont(diskfont);
    diskfont.setBold(true);
    ui->groupBox_2->setFont(diskfont);
    ui->tableWidget_2->setFont(diskfont);
    acdisk = 1;
}


MainWindow::~MainWindow()
{
    delete ui;
}

