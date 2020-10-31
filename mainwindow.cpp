﻿#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "abbt.h"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <vector>

extern "C"{
#include <ccos_image.h>
#include <ccos_private.h>
#include <common.h>
#include <wrapper.h>
#include <string_utils.h>
}

using namespace std;

vector<ccos_inode_t*> parents0, parents1, inodeon0, inodeon1, inodact;

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
    QTableWidget* tableWidget;
    QLabel* label;
    QGroupBox* box;
    QString disk, msg;
    if (curdisk == 0){
        tableWidget = ui->tableWidget;
        label = ui->label;
        box = ui->groupBox;
        disk= "I";
    }
    else{
        tableWidget = ui->tableWidget_2;
        label = ui->label_2;
        box = ui->groupBox_2;
        disk= "II";
    }
    for(int row= tableWidget->rowCount(); 0<=row; row--){
        tableWidget-> removeRow(row);
    }
    if (strlen(labd) == 0){
        msg = "Disk %1 - No label";
        box->setTitle(msg.arg(disk));
    }
    else{
        msg = "Disk %1 -%2";
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
    fcount = 1;
}
    inodact.clear();
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
    inodeon0.clear();
    inodeon0=inodact;
}
else if (curdisk == 1){
    inodeon1.clear();
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
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::Closef);
    connect(ui->actionAbout_GRiDISK_COmmander, &QAction::triggered, this, &MainWindow::aboutShow);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::Openf);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::Closef);
    connect(ui->tableWidget, &QTableWidget::cellActivated, this, &MainWindow::enterDIR0);
    connect(ui->tableWidget_2, &QTableWidget::cellActivated, this, &MainWindow::enterDIR1);
}

void MainWindow::aboutShow(){
     abss = new abbt(this);
     abss->show();
}

void MainWindow::enterDIR0(){
    QTableWidgetItem* called = ui->tableWidget->currentItem();
    ccos_inode_t* dir;
    if (nrot0 == 1 and called -> row() != 0){
        dir = inodeon0[called -> row()-1];
    }
    else{
        dir = inodeon0[called -> row()];
    }
    if (called -> row() == 0 and nrot0== 1){
        ccos_inode_t* dir;
        uint8_t* dat = NULL;
        size_t siz = 0;
        read_file(name0.toStdString().c_str(), &dat, &siz);
        ccos_inode_t* root = ccos_get_root_dir(dat, siz);
        if (parents0.size() <2){
           dir = ccos_get_root_dir(dat, siz);
        }
        else{
            dir = parents0[parents0.size()-2];
        }
        parents0.pop_back();
        char* fname = short_string_to_string(ccos_get_file_name(root));
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        ccos_get_dir_contents(dir, dat, &fils, &dirdata);
        if (parents0.size() == 0){
            nrot0 = 0;
        }
        doListTable(dirdata, fils, nrot0, dat, siz, 0, fname, ui);
    }
    else if (ccos_is_dir(dir)){
    parents0.push_back(dir);
    uint8_t* dat = NULL;
    size_t siz = 0;
    read_file(name0.toStdString().c_str(), &dat, &siz);
    ccos_inode_t* root = ccos_get_root_dir(dat, siz);
    char* fname = short_string_to_string(ccos_get_file_name(root));
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(dir, dat, &fils, &dirdata);
    nrot0 = 1;
    doListTable(dirdata, fils, nrot0, dat, siz, 0, fname, ui);
    } 
}

void MainWindow::enterDIR1(){
    QTableWidgetItem* called = ui->tableWidget_2->currentItem();
    ccos_inode_t* dir;
    if (nrot1 == 1 and called -> row() != 0){
        dir = inodeon1[called -> row()-1];
    }
    else{
        dir = inodeon1[called -> row()];
    }
    if (called -> row() == 0 and nrot1== 1){
        ccos_inode_t* dir;
        uint8_t* dat = NULL;
        size_t siz = 0;
        read_file(name1.toStdString().c_str(), &dat, &siz);
        ccos_inode_t* root = ccos_get_root_dir(dat, siz);
        if (parents1.size() <2){
           dir = ccos_get_root_dir(dat, siz);
        }
        else{
            dir = parents1[parents1.size()-2];
        }
        parents1.pop_back();
        char* fname = short_string_to_string(ccos_get_file_name(root));
        uint16_t fils = 0;
        ccos_inode_t** dirdata = NULL;
        ccos_get_dir_contents(dir, dat, &fils, &dirdata);
        if (parents1.size() == 0){
            nrot1 = 0;
        }
        doListTable(dirdata, fils, nrot1, dat, siz, 1, fname, ui);
    }
    else if (ccos_is_dir(dir)){
    parents1.push_back(dir);
    uint8_t* dat = NULL;
    size_t siz = 0;
    read_file(name1.toStdString().c_str(), &dat, &siz);
    ccos_inode_t* root = ccos_get_root_dir(dat, siz);
    char* fname = short_string_to_string(ccos_get_file_name(root));
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(dir, dat, &fils, &dirdata);
    nrot1 = 1;
    doListTable(dirdata, fils, nrot1, dat, siz, 1, fname, ui);
    }
}

void MainWindow::Dfils(){
    QTableWidget* tableWidget;
    if (acdisk == 0){
        tableWidget = ui->tableWidget;
    }
    else{
        tableWidget = ui->tableWidget_2;
    }

}

void MainWindow::Openf(){
    QString name = QFileDialog::getOpenFileName(this, "Open Image", "", "GRiD Image Files (*.img)");
    uint8_t* dat = NULL;
    size_t siz = 0;
    read_file(name.toStdString().c_str(), &dat, &siz);
    if (name == ""){
        return;
    }
    if(ccos_get_root_dir(dat, siz) == NULL){
        QMessageBox msgBox;
        msgBox.critical(0,"Incorrect Image File", "Image broken or have non-GRiD format!");
        return;
    }
    int panel = 0;
    if (isop0 == 1 and isop1 == 1){
        panel = acdisk;
        if(acdisk == 0){
            name0 = name;
        }
        else{
            name1 = name;
        }
    }
    else if (isop0 == 1){
        panel = 1;
        isop1 = 1;
        name1 = name;
    }
    else if (isop0 == 0) {
        isop0 = 1;
        name0 = name;
    }
    ccos_inode_t* root = ccos_get_root_dir(dat, siz);
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(root, dat, &fils, &dirdata);
    char* fname = short_string_to_string(ccos_get_file_name(root));
    doListTable(dirdata, fils, 0, dat, siz, panel, fname, ui);
}

void MainWindow::Closef(){
    QTableWidget* tableWidget;
    if (acdisk == 0){
        tableWidget = ui->tableWidget;
        parents0.clear();
        inodeon0.clear();
        isop0 = 0;
        name0 = "";
        ui->label->setText("Free space:");
        ui->groupBox->setTitle("Disk I - No disk");
    }
    else{
        tableWidget = ui->tableWidget_2;
        parents1.clear();
        inodeon1.clear();
        isop1 = 0;
        name1 = "";
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

