#include "mainwindow.h"

extern "C"{ //Load C "ccos_image" library headers
#include <ccos_image/ccos_image.h>
#include <ccos_image/common.h>
#include <ccos_image/wrapper.h>
}

using namespace std;

vector<ccos_inode_t*> inodeon[2];
ccos_inode_t* curdir[2] = {NULL};
bool isch[2] = {0};
struct stat info;

//[Service functions]

//*Get file version and convert to QString ("A.B.C")
QString ccosGetFileVersionQstr(ccos_inode_t* file) {
    version_t ver = ccos_get_file_version(file);
    return QString("%1.%2.%3").arg(QString::number(ver.major), QString::number(ver.minor),
                                   QString::number(ver.patch));
}

//*Convert date ("dd.MM.yyyy")
QString ccosDateToQstr(ccos_date_t date) {
    QString day = QString::number(date.day);
    QString month = QString::number(date.month);
    QString year = QString::number(date.year);
    while (day.size() < 2)
        day = "0" + day;
    while (month.size() < 2)
        month = "0" + month;
    while (year.size() < 4)
        year = "0" + year;
    return QString("%1.%2.%3").arg(day, month, year);
}

//*Insert file row to the widget
void addFile(QTableWidget* tableWidget, int mode){ //*For empty
    QTableWidgetItem* rows[7];
    tableWidget->insertRow(0);
    for (int i = 0; i < 7; i++){
        rows[i] = new QTableWidgetItem();
        rows[i]->setFlags(rows[i]->flags() ^ Qt::ItemIsEditable);
        tableWidget->setItem(0, i, rows[i]);
    }
    rows[0]->setText(mode == 1 ? "<EMPTY IMAGE>" : mode == 2 ? ".." : "<EMPTY>");
    if (mode == 2)
        rows[1]->setText("<PARENT-DIR>");
}

void addFile(QTableWidget* tableWidget, QString text[], int row){ //*For normal
    QTableWidgetItem* rows[7];
    tableWidget->insertRow(row);
    for (int i = 0; i < 7; i++){
        rows[i] = new QTableWidgetItem();
        rows[i]->setFlags(rows[i]->flags() ^ Qt::ItemIsEditable);
        rows[i]->setText(text[i]);
        tableWidget->setItem(row, i, rows[i]);
    }
}

//*Check if real file named as <Name>~<Type>~
int tildaCheck(string parse_str){
    vector<string> output;

    size_t pos = 0;
    string token;
    while ((pos = parse_str.find("~")) != string::npos) {
        token = parse_str.substr(0, pos);
        output.push_back(token);
        parse_str.erase(0, pos + 1);
    }
    if (output.size() == 1)
        output.push_back(parse_str);

    if (output.size() == 0 or output.size() > 2)
        return -1;

    return 0;
}

//*Check if space is enough to add files
int checkFreeSp(uint8_t* data, size_t data_size, vector<ccos_inode_t*> inodeList,
                QList<QTableWidgetItem *> calledElems, size_t* needs){ //*For copy
    size_t free = ccos_calc_free_space(data, data_size);
    *needs = 0;
    for (int i = 0; i < calledElems.size(); i+=6)
        *needs += ccos_get_file_size(inodeList[calledElems[i]->row()]);

    if (*needs > free)
        return -1;
    else
        return 0;
}

int checkFreeSp(uint8_t* data, size_t data_size, QStringList files, size_t* needs){ //*For add
    size_t free = ccos_calc_free_space(data, data_size);
    *needs = 0;
    for (int i = 0; i < files.size(); i++) {
        ifstream in(files[i].toStdString(), ifstream::ate | ifstream::binary);
        *needs += in.tellg();
    }
    if (*needs > free)
        return -1;
    else
        return 0;
}

//*Get directory listing and fill it to table
void fillTable(ccos_inode_t* directory, bool noRoot, uint8_t* dat, size_t siz, bool curdisk, Ui::MainWindow* ui){
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
    for(int row= tableWidget->rowCount(); 0<=row; row--)
        tableWidget-> removeRow(row);
    char* labd = ccos_get_image_label(dat, siz);
    msg = "Disk %1 - %2%3";
    box->setTitle(msg.arg(disk, (strlen(labd) != 0) ? labd : "No label", isch[curdisk] ? "*" : ""));
    char basename[CCOS_MAX_FILE_NAME];
    char type[CCOS_MAX_FILE_NAME];
    int fcount = 0;
    if (noRoot){
        addFile(tableWidget, 2);
        inodeon[curdisk].insert(inodeon[curdisk].begin(), 0);
        fcount = 1;
    }
    for(int c = 0; c < fils; c++){
        memset(basename, 0, CCOS_MAX_FILE_NAME);
        memset(type, 0, CCOS_MAX_FILE_NAME);
        ccos_parse_file_name(dirdata[c], basename, type, NULL, NULL);
        QString qtype = type;
        if (qtype.toLower() == "subject")
            qtype = qtype + " <DIR>";
        QString text[] = {basename, qtype, QString::number(ccos_get_file_size(dirdata[c])),
                         ccosGetFileVersionQstr(dirdata[c]),
                         ccosDateToQstr(ccos_get_creation_date(dirdata[c])),
                         ccosDateToQstr(ccos_get_mod_date(dirdata[c])),
                         ccosDateToQstr(ccos_get_exp_date(dirdata[c]))};
        if (ccos_is_dir(dirdata[c])){
            inodeon[curdisk].insert(inodeon[curdisk].begin() + fcount, dirdata[c]);
            addFile(tableWidget, text, fcount);
            fcount++;
        }
        else{
            inodeon[curdisk].push_back(dirdata[c]);
            addFile(tableWidget, text, tableWidget->rowCount());
        }
    }
    size_t free_space = ccos_calc_free_space(dat, siz);
    msg = "Free space: %1 bytes.";
    label->setText(msg.arg(free_space));
    if (inodeon[curdisk].size()==0){
        addFile(tableWidget, 1);
        inodeon[curdisk].insert(inodeon[curdisk].begin(), 0);
    }

    free(dirdata);
}

//*Ask if user wants to save a file
int saveBox(QString disk){
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString("The Disk %1 has been modified.").arg(disk));
    msgBox.setInformativeText("Do you want to save your changes?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();
    return ret == QMessageBox::Save ? 1 : ret == QMessageBox::Discard ? -1 : 0;
}
//[Service functions]

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
    focused(0)
{
    ui->setupUi(this);

    trace_init(0); //Set this to 1 for debug trace


    QMainWindow::setWindowTitle(QString("GRiDISK Commander ")+_PVER_);
    QTableWidget* twig[] = {ui->tableWidget, ui->tableWidget_2};
    for (int i = 0; i < 2; i++){
        twig[i]->horizontalHeader()->resizeSection(0, 155);
        twig[i]->horizontalHeader()->resizeSection(2, szcor);
        twig[i]->horizontalHeader()->resizeSection(3, 80);
        twig[i]->horizontalHeader()->resizeSection(4, 80);
        twig[i]->horizontalHeader()->resizeSection(5, 80);
        twig[i]->horizontalHeader()->resizeSection(6, 80);
        addFile(twig[i], 0);
        twig[i]->verticalHeader()->hide();
        twig[i]->setSelectionBehavior(QAbstractItemView::SelectRows);
    }
    QFont diskfont;
    diskfont.setFamily(QString::fromUtf8("Arial"));
    diskfont.setPointSize(9);
    diskfont.setUnderline(false);
    diskfont.setWeight(50);
    diskfont.setBold(true);
    ui->groupBox->setFont(diskfont);
    ui->tableWidget->setFont(diskfont);
    diskfont.setBold(false);
    ui->groupBox_2->setFont(diskfont);
    ui->tableWidget_2->setFont(diskfont);
    //  Buttons connecting
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(OpenImg()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(CloseImg()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(Save()));
    connect(ui->pushButton_4, SIGNAL(clicked()), this, SLOT(Add()));
    connect(ui->pushButton_5, SIGNAL(clicked()), this, SLOT(Copy()));
    connect(ui->pushButton_6, SIGNAL(clicked()), this, SLOT(Rename()));
    connect(ui->pushButton_7, SIGNAL(clicked()), this, SLOT(Delete()));
    connect(ui->pushButton_8, SIGNAL(clicked()), this, SLOT(Extract()));
    connect(ui->pushButton_9, SIGNAL(clicked()), this, SLOT(MakeDir()));
    connect(ui->pushButton_10, SIGNAL(clicked()), this, SLOT(ExtractAll()));
    //  Context menus connecting
    connect(ui->actionAdd, SIGNAL(triggered()), this, SLOT(Add()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(AboutShow()));
    connect(ui->actionChange_date, SIGNAL(triggered()), this, SLOT(Date()));
    connect(ui->actionChange_label, SIGNAL(triggered()), this, SLOT(Label()));
    connect(ui->actionChange_version, SIGNAL(triggered()), this, SLOT(Version()));
    connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(CloseImg()));
    connect(ui->actionCopy, SIGNAL(triggered()), this, SLOT(Copy()));
    connect(ui->actionDelete, SIGNAL(triggered()), this, SLOT(Delete()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(Extract()));
    connect(ui->actionExtract_all, SIGNAL(triggered()), this, SLOT(ExtractAll()));
    connect(ui->actionMake_dir, SIGNAL(triggered()), this, SLOT(MakeDir()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(New()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(OpenImg()));
    connect(ui->actionRename, SIGNAL(triggered()), this, SLOT(Rename()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(Save()));
    connect(ui->actionSave_as, SIGNAL(triggered()), this, SLOT(SaveAs()));
    //  Cell activating connect
    connect(ui->tableWidget, SIGNAL(cellActivated(int,int)), this, SLOT(OpenDir()));
    connect(ui->tableWidget_2, SIGNAL(cellActivated(int,int)), this, SLOT(OpenDir()));

}

void MainWindow::AboutShow(){
    abss = new AbDlg(this);
    abss->exec();
}

void MainWindow::Add(){
    QMessageBox msgBox;
    if (isop[acdisk]){
        if (nrot[acdisk] == 0){
            msgBox.information(this, tr("Add file(s)"),
                               tr("GRiD supports files only in directories!"));
            return;
        }
        QStringList files = QFileDialog::getOpenFileNames(
                    this, "Select files to add");
        AddFiles(files, curdir[acdisk]);
    }
}

void MainWindow::AddDirs(QStringList dirs){
    ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
    for (int i = 0; i < dirs.size(); i++){
        string dname = dirs[i].toStdString();
        const size_t last_slash_idx = dname.find_last_of("/");
        if (string::npos != last_slash_idx)
            dname.erase(0, last_slash_idx + 1);
        size_t frees = ccos_calc_free_space(dat[acdisk], siz[acdisk]);
        if (frees < 1024) {
            QMessageBox errBox;
            errBox.critical(0,"Not enough space",
                            QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frees));
            break;
        }
        ccos_inode_t* newdir = ccos_create_dir(root, dname.c_str(), dat[acdisk], siz[acdisk]);
        if (newdir == NULL){
            QMessageBox errBox;
            errBox.critical(0,"Failed to create folder",
                            "Program can't create a folder in the image!");
            break;
        }
        QDir scandir(dirs[i]);
        QFileInfoList filesInfo = scandir.entryInfoList(QDir::Files);
        QStringList files;
        for (int c = 0; c < filesInfo.size(); c++){
            files.append(filesInfo[c].absoluteFilePath());
        }
        if (AddFiles(files, newdir) == -1)
          break;
    }
    if (dirs.size() != 0){
        isch[acdisk]= 1;
        fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

int MainWindow::AddFiles(QStringList files, ccos_inode_t* copyTo){
    QMessageBox msgBox;
    size_t needs= 0;
    if (checkFreeSp(dat[acdisk], siz[acdisk], files, &needs) == -1) {
        size_t free = ccos_calc_free_space(dat[acdisk], siz[acdisk]);
        QMessageBox errBox;
        errBox.critical(0,"Not enough space",
                        QString("Requires %1 bytes of additional disk space to add!").arg(needs-free));
        return -1;
    }
    for (int i = 0; i < files.size(); i++){
        uint8_t* fdat = NULL;
        size_t fsiz = 0;
        string fname = files[i].toStdString();
        const size_t last_slash_idx = fname.find_last_of("/");
        if (string::npos != last_slash_idx)
            fname.erase(0, last_slash_idx + 1);
        if (tildaCheck(fname) == -1){
            rnam = new RenDlg(this);
            rnam->setInfo((QString("Set correct name and type for %1:").arg(fname.c_str())));
            while (true){
                bool ret = rnam->exec();
                if (ret == 1){
                    if (rnam->getType().toLower() == "subject"){
                        msgBox.critical(0,"Incorrect Type",
                                        "Can't set directory type for file!");
                    }
                    else if (rnam->getName() == "" or rnam->getType() == ""){
                        msgBox.critical(0,"Incorrect Name or Type",
                                        "File name or type can't be empty!");
                    }
                    else{
                        QString crnam = "%1~%2~";
                        fname = crnam.arg(rnam->getName(), rnam->getType()).toStdString();
                        break;
                    }
                }
                else
                    return -1;
            }
        }
        if (read_file(files[i].toStdString().c_str(), &fdat, &fsiz) == -1 ||
                ccos_add_file(copyTo, fdat, fsiz, fname.c_str(), dat[acdisk], siz[acdisk]) == NULL){
            msgBox.critical(0,"Error",
                            QString("Can't add \"%1\" to the image! Skipping...").arg(fname.c_str()));
        }
    }
    if (files.size() != 0){
        isch[acdisk]= 1;
        fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
    return 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool backdstat = acdisk;
    if (isch[0]){
        int ret = saveBox("I");
        if (ret == 1){
            acdisk = 0;
            MainWindow::Save();
            acdisk = backdstat;
        }
        else if (ret == 0)
            event->ignore();
    }
    if (isch[1]){
        int ret = saveBox("II");
        if (ret == 1){
            acdisk = 1;
            MainWindow::Save();
            acdisk = backdstat;
        }
        else if (ret == 0)
            event->ignore();
    }
}

int MainWindow::CloseImg(){
    if (isch[acdisk]){
        int ret = saveBox((acdisk == 0) ? "I" : "II");
        if (ret == 1)
            MainWindow::Save();
        else if (ret == 0)
            return 0;
    }

    QTableWidget* tableWidget;
    inodeon[acdisk].clear();
    isch[acdisk] = 0;
    isop[acdisk] = 0;
    name[acdisk] = "";

    if (dat[acdisk] != nullptr)
        free(dat[acdisk]);

    dat[acdisk] = nullptr;
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
    for(int row = tableWidget->rowCount(); 0<=row; row--)
        tableWidget-> removeRow(row);
    addFile(tableWidget, 0);
    return 1;
}

void MainWindow::Copy(){
    QTableWidget* tw;
    if (isop[acdisk] and isop[!acdisk]){
        QMessageBox msgBox;
        if (acdisk == 0)
            tw= ui->tableWidget;
        else
            tw= ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 6 and inodeon[acdisk][called[0]->row()]==0)
            return;
        bool selpar = 0;
        for (int t = 0; t < called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0){
                selpar = 1;
                break;
            }
        }
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to copy %1 file(s)?").arg((called.size()- selpar)/6));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        if (ret != QMessageBox::Yes)
            return;
        size_t needs= 0;
        if (checkFreeSp(dat[!acdisk], siz[!acdisk], inodeon[acdisk], called, &needs) == -1) {
            size_t free = ccos_calc_free_space(dat[!acdisk], siz[!acdisk]);
            QMessageBox errBox;
            errBox.critical(0,"Not enough space",
                            QString("Requires %1 bytes of additional disk space to copy").arg(needs-free));
            return;
        }
        for (int t = 0; t < called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0)
                continue;
            if (ccos_is_dir(inodeon[acdisk][called[t]->row()])) {
                if (ccos_file_id(curdir[!acdisk]) != ccos_file_id(ccos_get_root_dir(dat[!acdisk], siz[!acdisk]))) {
                    QMessageBox errBox;
                    errBox.critical(0,"Copying to non-root",
                                    "Folders can be copied only to root folder!");
                    return;
                }
                char newname[CCOS_MAX_FILE_NAME];
                memset(newname, 0, CCOS_MAX_FILE_NAME);
                ccos_parse_file_name(inodeon[acdisk][called[t]->row()], newname, NULL, NULL, NULL);
                ccos_inode_t* newdir = ccos_create_dir(ccos_get_root_dir(dat[!acdisk], siz[!acdisk]), newname,
                        dat[!acdisk], siz[!acdisk]);
                if (newdir == NULL){
                            QMessageBox errBox;
                            errBox.critical(0,"Failed to create folder",
                                            "Program can't create a folder in the image!");
                            return;
                }
                uint16_t fils = 0;
                ccos_inode_t** dirdata = NULL;
                ccos_get_dir_contents(inodeon[acdisk][called[t]->row()], dat[acdisk], &fils, &dirdata);
                for (int c = 0; c < fils; c++)
                    ccos_copy_file(dat[!acdisk], siz[!acdisk], newdir, dat[acdisk], dirdata[c]);
            }
            else {
                if (ccos_file_id(curdir[!acdisk]) == ccos_file_id(ccos_get_root_dir(dat[!acdisk], siz[!acdisk]))) {
                    QMessageBox errBox;
                    errBox.critical(0,"Copying to root",
                                    "Files can be copied only to non-root folder!");
                    return;
                }
                ccos_copy_file(dat[!acdisk], siz[!acdisk], curdir[!acdisk], dat[acdisk],
                        inodeon[acdisk][called[t]->row()]);
            }
        }
        isch[!acdisk] = 1;
        fillTable(curdir[!acdisk], nrot[!acdisk], dat[!acdisk], siz[!acdisk], !acdisk, ui);
        fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::Date(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw= ui->tableWidget;
        else
            tw= ui->tableWidget_2;
        ccos_inode_t* file = inodeon[acdisk][tw->currentItem()->row()];
        if (file != 0){
            ccos_date_t cre = ccos_get_creation_date(file);
            ccos_date_t mod = ccos_get_mod_date(file);
            ccos_date_t exp = ccos_get_exp_date(file);
            datd = new DateDlg(this);
            datd->init(file->name, cre, mod, exp);
            if (datd->exec()){
                datd->retDates(&cre, &mod, &exp);
                ccos_set_creation_date(file, cre);
                ccos_set_mod_date(file, mod);
                ccos_set_exp_date(file, exp);
                isch[acdisk] = 1;
                fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
            }
        }
    }
}

void MainWindow::Delete(){
    QTableWidget* tw;
    if (isop[acdisk]){
        QMessageBox msgBox;
        if (acdisk == 0)
            tw= ui->tableWidget;
        else
            tw= ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 6 and inodeon[acdisk][called[0]->row()]==0)
            return;
        bool selpar = 0;
        for (int t = 0; t< called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0){
                selpar = 1;
                break;
            }
        }
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to delete %1 file(s)?").arg((called.size()- selpar)/6));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        if (ret != QMessageBox::Yes)
            return;
        for (int t = 0; t< called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0)
                continue;
            ccos_delete_file(dat[acdisk], siz[acdisk], inodeon[acdisk][called[t]->row()]);
        }
        isch[acdisk] = 1;
        fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event){
   if (event->mimeData()->hasUrls())
       event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event){
   QStringList FilesList, DirsList;
   const QMimeData* mimeData = event->mimeData();
   if (mimeData->hasUrls()){
     QList<QUrl> urlList = mimeData->urls();
     for (int i = 0; i < urlList.size() && i < 32; ++i){
       stat(urlList.at(i).toLocalFile().toStdString().c_str(), &info);
       if (info.st_mode & S_IFDIR)
           DirsList.append(urlList.at(i).toLocalFile());
       else
           FilesList.append(urlList.at(i).toLocalFile());
     }
   }
   if (FilesList.length() == 1 && DirsList.length() == 0) {
       string ext = FilesList[0].toStdString().substr(FilesList[0].toStdString().find_last_of(".") + 1);
       transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return tolower(c); });
       if (ext == "img"){
           LoadImg(FilesList[0]);
       }
       else
           if (isop[acdisk] && nrot[acdisk])
               AddFiles(FilesList, curdir[acdisk]);
   }
   else if (!nrot[acdisk] && isop[acdisk]) {
       AddDirs(DirsList);

   }
   else
       if (isop[acdisk] && nrot[acdisk])
           AddFiles(FilesList, curdir[acdisk]);
}

void MainWindow::Extract(){
    QTableWidget* tw;
    if (isop[acdisk]){
        if (acdisk == 0)
            tw= ui->tableWidget;
        else
            tw= ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 6 and inodeon[acdisk][called[0]->row()]==0)
            return;
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == "")
            return;
        for (int t = 0; t < called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0)
                continue;
            if (ccos_is_dir(inodeon[acdisk][called[t]->row()]))
                dump_dir_to(name[acdisk].toStdString().c_str(), inodeon[acdisk][called[t]->row()],
                        dat[acdisk], todir.toStdString().c_str());
            else
                dump_file(todir.toStdString().c_str(), inodeon[acdisk][called[t]->row()], dat[acdisk]);
        }
    }
}

void MainWindow::ExtractAll(){
    if (isop[acdisk]){
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == "")
            return;
        dump_image_to(name[acdisk].toStdString().c_str(), dat[acdisk], siz[acdisk], todir.toStdString().c_str());
    }
}

void MainWindow::focusChanged(QWidget *, QWidget *now)
{
    if (now == ui->tableWidget || now == ui->tableWidget_2)
        setFocused(now == ui->tableWidget_2);
}

void MainWindow::Label(){
    if (isop[acdisk]){
        QString dsk;
        if (acdisk == 0)
            dsk= "I";
        else
            dsk= "II";
        char* fname = ccos_get_image_label(dat[acdisk], siz[acdisk]);
        QString nameQ = QInputDialog::getText(this, tr("New label"),
                                              QString("Set new label for the disk %1:").arg(dsk), QLineEdit::Normal, fname);
        if (nameQ == "")
            return;
        ccos_set_image_label(dat[acdisk], siz[acdisk], nameQ.toStdString().c_str());
        isch[acdisk]= 1;
        fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::LoadImg(QString path){
    if (path == "")
        return;

    if (dat[acdisk] != nullptr)
        if (!CloseImg()) return;

    read_file(path.toStdString().c_str(), &dat[acdisk], &siz[acdisk]);

    ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
    if (root == NULL){
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
    name[acdisk] = path;
    curdir[acdisk] = root;
    fillTable(root, 0, dat[acdisk], siz[acdisk], acdisk, ui);
}

void MainWindow::MakeDir(){
    if (isop[acdisk]){
        if (nrot[acdisk]){
            QMessageBox msgBox;
            msgBox.information(0,"Make dir",
                               "GRiD supports directories only in root!");
            return;
        }
        QString name = QInputDialog::getText(this, tr("Make dir"),
                                             tr("New directory name:"), QLineEdit::Normal,"");
        if (name == "")
            return;
        size_t free = ccos_calc_free_space(dat[acdisk], siz[acdisk]);
        if (free < 1024) {
            QMessageBox errBox;
            errBox.critical(0,"Not enough space",
                            QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-free));
            return;
        }
        ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
        if (ccos_create_dir(root, name.toStdString().c_str(), dat[acdisk], siz[acdisk]) == NULL){
            QMessageBox errBox;
            errBox.critical(0,"Failed to create folder",
                            "Program can't create a folder in the image!");
            return;
        }
        isch[acdisk] = 1;
        fillTable(root, nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::New(){
    if (dat[acdisk] != nullptr)
        if (!CloseImg()) return;

    //ccos_make_new_image(&dat[acdisk], &siz[acdisk]);

    inodeon[acdisk].clear();
    isch[acdisk] = 0;
    isop[acdisk] = 1;
    name[acdisk] = nullptr;
    ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
    curdir[acdisk] = root;
    fillTable(root, 0, dat[acdisk], siz[acdisk], acdisk, ui);
}

void MainWindow::OpenDir(){
    QTableWidget* tw;
    if (isop[acdisk]){
        if (acdisk == 0)
            tw= ui->tableWidget;
        else
            tw= ui->tableWidget_2;
        QTableWidgetItem* called = tw->currentItem();
        ccos_inode_t* dir;
        dir = inodeon[acdisk][called->row()];
        if (dir == 0 and nrot[acdisk]== 0)
            return;
        if (called -> row() == 0 and nrot[acdisk]){
            ccos_inode_t* root = ccos_get_root_dir(dat[acdisk], siz[acdisk]);
            curdir[acdisk] = (ccos_get_parent_dir(curdir[acdisk], dat[acdisk]));
            if (curdir[acdisk] == root)
                nrot[acdisk] = 0;
            fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
        else if (ccos_is_dir(dir)){
            curdir[acdisk] = dir;
            nrot[acdisk] = 1;
            fillTable(dir, nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
    }
}

void MainWindow::OpenImg(){
    QString Qname = QFileDialog::getOpenFileName(this, "Open Image", "", "GRiD Image Files (*.img)");
    //QString Qname = "/home/bs0dd/Pobrane/MODIF.img";
    if (Qname != "")
        LoadImg(Qname);
}

void  MainWindow::Rename(){
    if (isop[acdisk]){
        ccos_inode_t* reninode;
        if (acdisk == 0){
            if (ui->tableWidget->currentRow() == -1)
                return;
            reninode = inodeon[acdisk][ui->tableWidget->currentRow()];
        }
        else{
            if (ui->tableWidget_2->currentRow() == -1)
                return;
            reninode = inodeon[acdisk][ui->tableWidget_2->currentRow()];
        }
        if (reninode == 0){
            return;
        }
        char basename[CCOS_MAX_FILE_NAME];
        char type[CCOS_MAX_FILE_NAME];
        memset(basename, 0, CCOS_MAX_FILE_NAME);
        memset(type, 0, CCOS_MAX_FILE_NAME);
        ccos_parse_file_name(reninode, basename, type, NULL, NULL);
        rnam = new RenDlg(this);
        rnam->setName(basename);
        rnam->setType(type);
        rnam->setInfo((QString("Set new name and type for %1:").arg(reninode->name)));
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
            else if (rnam->getName() == "" or rnam->getType() == ""){
                QMessageBox msgBox;
                msgBox.critical(0,"Incorrect Name or Type",
                                "File name or type can't be empty!");
                return;
            }
            QString newname = rnam->getName();
            QString newtype = rnam->getType();
            ccos_rename_file(reninode, newname.toStdString().c_str(), newtype.toStdString().c_str());
            isch[acdisk] = 1;
            fillTable(ccos_get_parent_dir(reninode, dat[acdisk]),
                        nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
    }
}

void MainWindow::Save(){
    if (name[acdisk] == nullptr)
        return SaveAs();

    QGroupBox* gb;
    if (isch[acdisk]){
        if (acdisk == 0)
            gb= ui->groupBox;
        else
            gb= ui->groupBox_2;
        save_image(name[acdisk].toStdString().c_str(), dat[acdisk], siz[acdisk], true);
        isch[acdisk] = 0;
        gb->setTitle(gb->title().left(gb->title().size()-1));
    }
}

void MainWindow::SaveAs(){
    QGroupBox* gb;
    if (isop[acdisk]){
        if (acdisk == 0)
            gb= ui->groupBox;
        else
            gb= ui->groupBox_2;
        QString nameQ = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
        if (nameQ == "")
            return;
        save_image(nameQ.toStdString().c_str(), dat[acdisk], siz[acdisk], true);
        name[acdisk] = nameQ;
        if (isch[acdisk]){
            gb->setTitle(gb->title().left(gb->title().size()-1));
            isch[acdisk] = 0;
        }
    }
}

void MainWindow::setFocused(qint8 focused){
    this->focused = focused;
    acdisk = focused;

    QFont font = ui->groupBox->font();
    font.setBold(!focused);
    ui->groupBox->setFont(font);
    ui->tableWidget->setFont(font);

    font = ui->groupBox_2->font();
    font.setBold(focused);
    ui->groupBox_2->setFont(font);
    ui->tableWidget_2->setFont(font);
}

void MainWindow::Version(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw= ui->tableWidget;
        else
            tw= ui->tableWidget_2;
        ccos_inode_t* file = inodeon[acdisk][tw->currentItem()->row()];
        if (file != 0){
            version_t ver = ccos_get_file_version(file);
            vdlg = new VerDlg(this);
            vdlg->init(file->name, ver);
            if (vdlg->exec()){
                ver = vdlg->retVer();
                ccos_set_file_version(file, ver);
                isch[acdisk] = 1;
                fillTable(curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
            }
        }
    }
}

MainWindow::~MainWindow()
{
    if (dat[0] != nullptr) {
        free(dat[0]);
        dat[0] = nullptr;
    }
    if (dat[1] != nullptr) {
        free(dat[1]);
        dat[1] = nullptr;
    }
    delete ui;
}
