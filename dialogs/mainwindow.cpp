#include "mainwindow.h"

vector<ccos_inode_t*> inodeon[2];
ccos_inode_t* curdir[2] = {NULL};
bool isch[2] = {0};

//[Service functions]

//*Get file version and convert to QString ("A.B.C")
QString ccosGetFileVersionQstr(ccos_inode_t* file){
    version_t ver = ccos_get_file_version(file);
    return QString("%1.%2.%3").arg(QString::number(ver.major), QString::number(ver.minor),
                                   QString::number(ver.patch));
}

//*Convert date ("dd.MM.yyyy")
QString ccosDateToQstr(ccos_date_t date){
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

void addFile(QTableWidget* tableWidget, QString text[]){ //*For normal
    QTableWidgetItem* rows[7];
    int row = tableWidget->rowCount();
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
int checkFreeSp(ccfs_handle ctx, uint8_t* data, size_t data_size, vector<ccos_inode_t*> inodeList,
                QList<QTableWidgetItem *> calledElems, size_t* needs){ //*For copy

    size_t frsp = ccos_calc_free_space(ctx, data, data_size);
    *needs = 0;
    for (int i = 0; i < calledElems.size(); i+=6){
        ccos_inode_t* file = inodeList[calledElems[i]->row()];
        if (file != 0){
            if (ccos_is_dir(file)){
                uint16_t fils = 0;
                ccos_inode_t** dirdata = NULL;
                ccos_get_dir_contents(ctx, file, data, &fils, &dirdata);

                for(int i = 0; i < fils; i++){
                    *needs += ccos_get_file_size(dirdata[i]);
                }
                free(dirdata);
            }

            *needs += ccos_get_file_size(file);
        }
    }

    if (*needs > frsp)
        return -1;
    else
        return 0;
}

int checkFreeSp(ccfs_handle ctx, uint8_t* data, size_t data_size, QStringList files, size_t* needs){ //*For add
    size_t frsp = ccos_calc_free_space(ctx, data, data_size);
    *needs = 0;
    for (int i = 0; i < files.size(); i++) {
        *needs += QFileInfo(files[i]).size();
    }
    if (*needs > frsp)
        return -1;
    else
        return 0;
}

//*Read file to uint8_t* array
int readFileQt(QString path, uint8_t** file_data, size_t* file_size, QWidget* parent){
    QFileInfo ilfile(path);
    if (!ilfile.exists()){
        QMessageBox::critical(parent, "Invalid path",
                        QString("The object on the path \"%1\" does not exist!").arg(path));
        return -1;
    }

    if (!ilfile.isFile()){
        QMessageBox::critical(parent, "Invalid path",
                        QString("The object on the path \"%1\" is not file!").arg(path));
        return -1;
    }

    QFile lfile(path);
    if (!lfile.open(QIODevice::ReadOnly)){
        QMessageBox::critical(parent, "Unable to open file",
                        QString("Unable to open file \"%1\". Please check the file.").arg(path));
        return -1;
    }

    *file_size = ilfile.size();

    *file_data = (uint8_t*)calloc(*file_size, sizeof(uint8_t));

    if (file_data == nullptr){
        QMessageBox::critical(parent, "Failed to allocate memory",
                        QString("Failed to allocate %1 bytes for image. Please free up some RAM.").arg(*file_size));
        return -1;
    }

    qint64 readed = lfile.read((char*)(*file_data), *file_size);

    if (readed == -1){
        QMessageBox::critical(parent, "Failed to read data",
                        QString("Failed to read data from file \"%1\". Please check the file.").arg(path));

        free(*file_data);
        *file_data = nullptr;
        return -1;
    }

    return 0;
}

//*Write file from uint8_t* array
int saveFileQt(QString path, uint8_t* file_data, size_t file_size, QWidget* parent){
    QFile lfile(path);
    if (!lfile.open(QIODevice::WriteOnly)){
        QMessageBox::critical(parent, "Unable to open/create file",
                        QString("Unable to open/create file \"%1\". Please check the file.").arg(path));
        return -1;
    }

    qint64 written = lfile.write((char*)(file_data), file_size);

    if (written == -1){
        QMessageBox::critical(parent, "Failed to write data",
                        QString("Failed to write data to file \"%1\"!").arg(path));
        return -1;
    }

    return 0;
}

//*Dump file from image to path
int dumpFileQt(ccfs_handle ctx, ccos_inode_t* file, uint8_t* data, QString path, QWidget* parent){
    char* fnam = short_string_to_string(ccos_get_file_name(file));
    replace_char_in_place(fnam, '/', '_');

    QString fpath = QDir(path).filePath(fnam);

    size_t file_size = 0;
    uint8_t* file_data = NULL;

    if (ccos_read_file(ctx, file, data, &file_data, &file_size) == -1){
        QMessageBox::critical(parent, "Failed to read file from image",
                      QString("Unable to read file \"%1\": Unable to get file contents!").arg(fnam));
        free(fnam);
        return -1;
    }

    free(fnam);

    if (saveFileQt(fpath, file_data, file_size, parent) == -1){
        free(file_data);
        return -1;
    }

    free(file_data);
    return 0;
}

//*Dump dir from image to path
int dumpDirQt(ccfs_handle ctx, ccos_inode_t* dir, uint8_t* data, QString path, QWidget* parent){
    char name[CCOS_MAX_FILE_NAME];
    memset(name, 0, CCOS_MAX_FILE_NAME);
    ccos_parse_file_name(dir, name, NULL, NULL, NULL);
    replace_char_in_place(name, '/', '_');

    QString dpath = QDir(path).filePath(name);

    if (!QDir(dpath).exists() && !QDir().mkdir(dpath)){
            QMessageBox::critical(parent, "Failed to create directory",
                          QString("Failed to create directory \"%1\"!").arg(name));
            return -1;
    }

    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(ctx, dir, data, &fils, &dirdata);

    for(int i = 0; i < fils; i++){
        if (ccos_is_dir(dirdata[i]))
            dumpDirQt(ctx, dirdata[i], data, dpath, parent);
        else
            dumpFileQt(ctx, dirdata[i], data, dpath, parent);
    }

    free(dirdata);
    return 0;
}

//*Dump full image to path
int dumpImgQt(ccfs_handle ctx, uint8_t* data, size_t data_size, QString path, QString altname, QWidget* parent){
    ccos_inode_t* root_dir = ccos_get_root_dir(ctx, data, data_size);
    if (root_dir == NULL) {
        QMessageBox::critical(parent, "Failed to dump image",
                      "Unable to dump image: Unable to get root directory!");
        return -1;
    }

    char* name = short_string_to_string(ccos_get_file_name(root_dir));
    QString dnam;

    if (strcmp(name, "") == 0){
        dnam = altname;
    }
    else{
        replace_char_in_place(name, '/', '_');
        dnam = name;
    }
    free(name);

    QString dpath = QDir(path).filePath(dnam);

    if (!QDir(dpath).exists() && !QDir().mkdir(dpath)){
            QMessageBox::critical(parent, "Failed to create directory",
                          QString("Failed to create directory \"%1\"!").arg(dnam));
            return -1;
    }

    return dumpDirQt(ctx, root_dir, data, dpath, parent);
}

//*Check if string is valid for CCOS (does not contain unicode and reserved characters)
int validString(QString string, bool ifpath, QWidget* parent){
    for (int i = 0; i < string.size(); i++){
        if (string[i] > 256 || (ifpath && (string[i] == '`' || string[i] == '|' || string[i] == '~'))){
            QMessageBox::critical(parent, "Incorrect characters", "Invalid character(s) were found! Take them away.");
            return -1;
        }
    }
    return 0;
}

//*Get directory listing and fill it to table
void fillTable(ccfs_handle ctx, ccos_inode_t* directory, bool noRoot, uint8_t* dat, size_t siz, bool curdisk, Ui::MainWindow* ui){
    QTableWidget* tableWidget;
    QLabel* label;
    QGroupBox* box;
    QString disk, msg;
    inodeon[curdisk].clear();
    uint16_t fils = 0;
    ccos_inode_t** dirdata = NULL;
    ccos_get_dir_contents(ctx, directory, dat, &fils, &dirdata);
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
    for (int row = tableWidget->rowCount(); 0<=row; row--)
        tableWidget->removeRow(row);
    char* labd = ccos_get_image_label(ctx, dat, siz);
    msg = "Disk %1 - %2%3";
    box->setTitle(msg.arg(disk, (strlen(labd) != 0) ? labd : "No label", isch[curdisk] ? "*" : ""));
    free(labd);
    char basename[CCOS_MAX_FILE_NAME];
    char type[CCOS_MAX_FILE_NAME];
    if (noRoot){
        addFile(tableWidget, 2);
        inodeon[curdisk].insert(inodeon[curdisk].begin(), 0);
    }
    for(int c = 0; c < fils; c++){
        memset(basename, 0, CCOS_MAX_FILE_NAME);
        memset(type, 0, CCOS_MAX_FILE_NAME);
        ccos_parse_file_name(dirdata[c], basename, type, NULL, NULL);
        QString qtype = type;
        if (!noRoot) //All files in the root are directories
            qtype = qtype + " <DIR>";
        QString text[] = {basename, qtype, QString::number(ccos_get_file_size(dirdata[c])),
                         ccosGetFileVersionQstr(dirdata[c]),
                         ccosDateToQstr(ccos_get_creation_date(dirdata[c])),
                         ccosDateToQstr(ccos_get_mod_date(dirdata[c])),
                         ccosDateToQstr(ccos_get_exp_date(dirdata[c]))};
        inodeon[curdisk].push_back(dirdata[c]);
        addFile(tableWidget, text);
    }
    size_t free_space = ccos_calc_free_space(ctx, dat, siz);
    msg = "Free space: %1 bytes.";
    label->setText(msg.arg(free_space));
    if (inodeon[curdisk].size()==0){
        addFile(tableWidget, 1);
        inodeon[curdisk].insert(inodeon[curdisk].begin(), 0);
    }

    free(dirdata);
}

//*Parse Hard Disk MBR
int parseMbr(uint8_t* hdddata, mbr_part_t parts[]){
    uint8_t* mbrtab = hdddata + 0x1BE;

    int grids = 0;
    for (int i = 0; i < 64; i+=16){
        if (mbrtab[i+4] != 0x47) //47h - GRiD partition type (the only known)
            continue; //If the type is different - not interested

        parts[i/16].isgrid = true;
        grids++;

        if (mbrtab[i] == 0x80){
            parts[i/16].active = true;
        }

        parts[i/16].offset = (mbrtab[i+8] | mbrtab[i+9] << 8 | mbrtab[i+10] << 16 | mbrtab[i+11] << 24) * 512;
        parts[i/16].size = (mbrtab[i+12] | mbrtab[i+13] << 8 | mbrtab[i+14] << 16 | mbrtab[i+15] << 24) * 512;
    }
    return grids;
}

//*Ask if user wants to save a file
int saveBox(QString disk, QWidget* parent){
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString("The Disk %1 has been modified.").arg(disk));
    msgBox.setInformativeText("Do you want to save your changes?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();
    return ret == QMessageBox::Save ? 1 : ret == QMessageBox::Discard ? -1 : 0;
}
//[Service functions]

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    trace_init(false);

    QMainWindow::setWindowTitle(QString("GRiDISK Commander v")+_PVER_);
    QTableWidget* twig[] = {ui->tableWidget, ui->tableWidget_2};
    for (int i = 0; i < 2; i++){
        twig[i]->horizontalHeader()->resizeSection(0, 155);
        twig[i]->horizontalHeader()->resizeSection(2, 45);
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

    QStringList argv = QCoreApplication::arguments();
    int argc = argv.size();

    if (argc > 1){
        for (int i = 1; i < argc; i++){
            QString arg = argv[i];
            if (arg == "--trace") {
                if (!ui->actionDebtrace->isChecked()){
                    ui->actionDebtrace->setChecked(true);
                    DebTrace();
                }
            }
            else if (!isop[0] || !isop[1]){
                QFileInfo fil(arg);
                if (fil.suffix().toLower() == "img" && fil.exists()){
                    acdisk = isop[0];
                    LoadImg(arg);
                }
            }
        }
    }

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
    connect(ui->actionAct_part, SIGNAL(triggered()), this, SLOT(SetActivePart()));
    connect(ui->actionAno_part, SIGNAL(triggered()), this, SLOT(AnPartMenu()));
    connect(ui->actionChange_date, SIGNAL(triggered()), this, SLOT(Date()));
    connect(ui->actionChange_label, SIGNAL(triggered()), this, SLOT(Label()));
    connect(ui->actionChange_version, SIGNAL(triggered()), this, SLOT(Version()));
    connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(CloseImg()));
    connect(ui->actionCopy, SIGNAL(triggered()), this, SLOT(Copy()));
    connect(ui->actionCopy_locally, SIGNAL(triggered()), this, SLOT(CopyLoc()));
    connect(ui->actionDebtrace, SIGNAL(triggered()), this, SLOT(DebTrace()));
    connect(ui->actionDelete, SIGNAL(triggered()), this, SLOT(Delete()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(Extract()));
    connect(ui->actionExtract_all, SIGNAL(triggered()), this, SLOT(ExtractAll()));
    connect(ui->actionMake_dir, SIGNAL(triggered()), this, SLOT(MakeDir()));
    connect(ui->actionNewImage, SIGNAL(triggered()), this, SLOT(NewImage()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(OpenImg()));
    connect(ui->actionRename, SIGNAL(triggered()), this, SLOT(Rename()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(Save()));
    connect(ui->actionSave_as, SIGNAL(triggered()), this, SLOT(SaveAs()));
    connect(ui->actionSep_save, SIGNAL(triggered()), this, SLOT(SavePart()));
    //  Cell activating connect
    connect(ui->tableWidget, SIGNAL(cellActivated(int,int)), this, SLOT(OpenDir()));
    connect(ui->tableWidget_2, SIGNAL(cellActivated(int,int)), this, SLOT(OpenDir()));

}

void MainWindow::AboutShow(){
    abss = new AbDlg(this);
    abss->exec();
}

void MainWindow::Add(){
    if (isop[acdisk]){
        if (nrot[acdisk] == 0){
            QMessageBox::information(this, tr("Add file(s)"),
                               tr("GRiD supports files only in directories!"));
            return;
        }
        QStringList files = QFileDialog::getOpenFileNames(
                    this, "Select files to add");
        AddFiles(files, curdir[acdisk]);
    }
}

void MainWindow::AddDirs(QStringList dirs){
    ccos_inode_t* root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
    for (int i = 0; i < dirs.size(); i++){
        size_t frees = ccos_calc_free_space(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
        if (frees < 1024) {
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frees));
            break;
        }
        ccos_inode_t* newdir = ccos_create_dir(ccdesc[acdisk], root, QFileInfo(dirs[i]).fileName().toStdString().c_str(),
                                               dat[acdisk], siz[acdisk]);
        if (newdir == NULL){
            QMessageBox::critical(this, "Failed to create folder",
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
    if (!dirs.empty()){
        isch[acdisk] = true;
        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

int MainWindow::AddFiles(QStringList files, ccos_inode_t* copyTo){
    size_t needs = 0;
    if (checkFreeSp(ccdesc[acdisk], dat[acdisk], siz[acdisk], files, &needs) == -1) {
        size_t free = ccos_calc_free_space(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
        QMessageBox::critical(this, "Not enough space",
                        QString("Requires %1 bytes of additional disk space to add!").arg(needs-free));
        return -1;
    }
    for (int i = 0; i < files.size(); i++){
        uint8_t* fdat = NULL;
        size_t fsiz = 0;
        string fname = QFileInfo(files[i]).fileName().toStdString();
        if (tildaCheck(fname) == -1){
            rnam = new RenDlg(this);
            rnam->setInfo((QString("Set correct name and type for %1:").arg(fname.c_str())));
            while (true){
                if (rnam->exec() == 1){
                    if (rnam->getType().toLower().contains("subject")){
                        QMessageBox::critical(this, "Incorrect Type",
                                        "Can't set directory type for file!");
                    }
                    else if (rnam->getName() == "" or rnam->getType() == ""){
                        QMessageBox::critical(this, "Incorrect Name or Type",
                                        "File name or type can't be empty!");
                    }
                    else{
                        if (validString(rnam->getName(), true, this) != -1 && validString(rnam->getType(), true, this) != -1){
                            QString crnam = "%1~%2~";
                            fname = crnam.arg(rnam->getName(), rnam->getType()).toStdString();
                            break;
                        }
                    }
                }
                else
                    return -1;
            }
        }
        if (readFileQt(files[i], &fdat, &fsiz, this) == 0){
            if (ccos_add_file(ccdesc[acdisk], copyTo, fdat, fsiz, fname.c_str(), dat[acdisk], siz[acdisk]) == NULL){
                QMessageBox::critical(this, "Error",
                                QString("Can't add \"%1\" to the image! Skipping...").arg(fname.c_str()));
            }
        }
    }
    if (files.size() != 0){
        isch[acdisk] = 1;
        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
    return 0;
}

void MainWindow::AnPartMenu(){
    AnotherPart(true);
}

void MainWindow::AnotherPart(bool fromMenu){
    bool usedisk;
    if (fromMenu)
        usedisk = acdisk;
    else
        usedisk = !acdisk;

    mbr_part_t parts[4] = {0, 0, 0, 0};
    parseMbr(hdddat[usedisk], parts);

    chsd = new ChsDlg(this);
    chsd->setName("Select disk partition");
    chsd->setInfo("Select the GRiD disk partition you want to work with:");

    if (fromMenu)
        chsd->enCheckBox();

    for (int i = 0; i < 4; i++){
        if (parts[i].isgrid && parts[i].active){
            chsd->addItem(QString("Partition %1, active").arg(i+1));
        }
        else if (parts[i].isgrid){
            chsd->addItem(QString("Partition %1").arg(i+1));
        }
    }

    if (chsd->exec() == 1){
        int selctd = chsd->getIndex();

        bool topan = (fromMenu && chsd->isChecked()) ? !acdisk : acdisk;

        if (usedisk != topan && isop[topan])
            CloseImg();

        siz[topan] = parts[selctd].size;
        dat[topan] = hdddat[usedisk]+parts[selctd].offset;

        ccos_inode_t* root = ccos_get_root_dir(ccdesc[topan], dat[topan], siz[topan]);
        if (root == NULL){
            QMessageBox::critical(this, "Incorrect Image File",
                                  "Image broken or have non-GRiD format!");
            dat[topan] = nullptr;
            if (usedisk == topan){
                hddmode[topan] = false;
                free(hdddat[topan]);
                hdddat[topan] = nullptr;
            }
            return;
        }
        oneimg = true;
        isop[topan] = true;
        hddmode[topan] = true;
        curdir[topan] = root;
        if (!fromMenu || chsd->isChecked()){
            name[topan] = name[!topan];
            hdddat[topan] = hdddat[!topan];
        }

        fillTable(ccdesc[topan], root, 0, dat[topan], siz[topan], topan, ui);
    }
}

void MainWindow::closeEvent(QCloseEvent *event){
    bool backdstat = acdisk;
    for (int i = 0; i < 2; i++){
        if (isch[i]){
            int ret = saveBox(i == 0 ? "I" : "II", this);
            if (ret == 1){
                acdisk = i;
                Save();
                acdisk = backdstat;
            }
            else if (ret == 0)
                event->ignore();
        }
    }
}

int MainWindow::CloseImg(){
    if (isch[acdisk]){
        int ret = saveBox((acdisk == 0) ? "I" : "II", this);
        if (ret == 1)
            MainWindow::Save();
        else if (ret == 0)
            return 0;
    }

    QTableWidget* tableWidget;
    inodeon[acdisk].clear();
    isch[acdisk] = 0;
    isop[acdisk] = 0;
    hddmode[acdisk] = false;
    HDDMenu(false);
    name[acdisk] = "";

    if (hdddat[acdisk] != nullptr){
        if (!oneimg)
            free(hdddat[acdisk]);

        hdddat[acdisk] = nullptr;
        dat[acdisk] = nullptr;
    }
    else if (dat[acdisk] != nullptr){
        free(dat[acdisk]);
        dat[acdisk] = nullptr;
    }

    oneimg = false;
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
    if (isop[acdisk] && isop[!acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 7 && inodeon[acdisk][called[0]->row()] == 0)
            return;
        bool selpar = (inodeon[acdisk][called[0]->row()] == 0) ? 1 : 0;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to copy %1 file(s)?").arg((called.size()/7)-selpar));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
        size_t needs = 0;
        if (checkFreeSp(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk], inodeon[acdisk], called, &needs) == -1) {
            size_t frsp = ccos_calc_free_space(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk]);
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to copy").arg(needs-frsp));
            return;
        }
        for (int t = 0; t < called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0)
                continue;
            if (ccos_is_dir(inodeon[acdisk][called[t]->row()])) {
                if (ccos_file_id(curdir[!acdisk]) != ccos_file_id(ccos_get_root_dir(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk]))) {
                    QMessageBox::critical(this, "Copying to non-root",
                                    "Folders can be copied only to root folder!");
                    return;
                }
                char newname[CCOS_MAX_FILE_NAME] = {};
                ccos_parse_file_name(inodeon[acdisk][called[t]->row()], newname, NULL, NULL, NULL);
                ccos_inode_t* newdir = ccos_create_dir(ccdesc[!acdisk], ccos_get_root_dir(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk]), newname,
                        dat[!acdisk], siz[!acdisk]);
                if (newdir == NULL){
                            QMessageBox::critical(this, "Failed to create folder",
                                            "Program can't create a folder in the image!");
                            return;
                }
                uint16_t fils = 0;
                ccos_inode_t** dirdata = NULL;
                ccos_get_dir_contents(ccdesc[acdisk], inodeon[acdisk][called[t]->row()], dat[acdisk], &fils, &dirdata);
                for (int c = 0; c < fils; c++) {
                    ccos_copy_file(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk], newdir, dat[acdisk], dirdata[c]);
                }
            }
            else {
                if (ccos_file_id(curdir[!acdisk]) == ccos_file_id(ccos_get_root_dir(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk]))) {
                    QMessageBox::critical(this, "Copying to root",
                                    "Files can be copied only to non-root folder!");
                    return;
                }
                ccos_copy_file(ccdesc[!acdisk], dat[!acdisk], siz[!acdisk], curdir[!acdisk], dat[acdisk],
                        inodeon[acdisk][called[t]->row()]);
            }
        }
        isch[!acdisk] = true;
        fillTable(ccdesc[!acdisk], curdir[!acdisk], nrot[!acdisk], dat[!acdisk], siz[!acdisk], !acdisk, ui);
        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::CopyLoc(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;

        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 7 && inodeon[acdisk][called[0]->row()] == 0)
            return;

        size_t needs = 0;
        if (checkFreeSp(ccdesc[acdisk], dat[acdisk], siz[acdisk], inodeon[acdisk], called, &needs) == -1) {
            size_t frsp = ccos_calc_free_space(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to copy").arg(needs-frsp));
            return;
        }

        if (nrot[acdisk]){
            chsd = new ChsDlg(this);
            chsd->setName("Select the directory");
            chsd->setInfo("Select the directory where the file(s) will be copied:");

            ccos_inode_t* root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);

            uint16_t fils = 0;
            ccos_inode_t** dirdata = NULL;
            ccos_get_dir_contents(ccdesc[acdisk], root, dat[acdisk], &fils, &dirdata);

            char basename[CCOS_MAX_FILE_NAME];

            for(int i = 0; i < fils; i++){
                memset(basename, 0, CCOS_MAX_FILE_NAME);
                ccos_parse_file_name(dirdata[i], basename, NULL, NULL, NULL);
                chsd->addItem(basename);
            }
            chsd->exec();

            ccos_inode_t* firfil = inodeon[acdisk][called[0]->row()] == NULL ?
                        inodeon[acdisk][called[6]->row()] : inodeon[acdisk][called[0]->row()];

            if (dirdata[chsd->getIndex()]->header.file_id == firfil->desc.dir_file_id){
                QMessageBox::critical(this, "Copy to parent dir",
                                      "Can't copy files to it's parent dir!");
                return;
            }

            for (int t = 0; t < called.size(); t+=7){
                if (inodeon[acdisk][called[t]->row()]==0)
                    continue;
                ccos_copy_file(ccdesc[acdisk], dat[acdisk], siz[acdisk], dirdata[chsd->getIndex()],
                        dat[acdisk], inodeon[acdisk][called[t]->row()]);
            }
            isch[acdisk] = true;
            fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
        else{
            for (int t = 0; t < called.size(); t+=7){
                char basename[CCOS_MAX_FILE_NAME];
                memset(basename, 0, CCOS_MAX_FILE_NAME);
                ccos_parse_file_name(inodeon[acdisk][called[t]->row()], basename, NULL, NULL, NULL);

                QString name;
                while (true){
                    name = QInputDialog::getText(this, tr("Copy dir"),
                                                 tr("Name for \"%1\" copy:").arg(basename), QLineEdit::Normal, name);
                    if (name == "")
                        break;
                    else if (validString(name, true, this) != -1){
                        ccos_inode_t* root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);

                        ccos_inode_t* newdir = ccos_create_dir(ccdesc[acdisk], root, name.toStdString().c_str(), dat[acdisk], siz[acdisk]);
                        if (newdir == NULL){
                            QMessageBox::critical(this, "Failed to create folder",
                                            "Program can't create a folder in the image!");
                            break;
                        }

                        uint16_t fils = 0;
                        ccos_inode_t** dirdata = NULL;
                        ccos_get_dir_contents(ccdesc[acdisk], inodeon[acdisk][called[t]->row()], dat[acdisk], &fils, &dirdata);

                        for(int i = 0; i < fils; i++){
                            ccos_copy_file(ccdesc[acdisk], dat[acdisk], siz[acdisk], newdir,
                                    dat[acdisk], dirdata[i]);
                        }
                        isch[acdisk] = true;
                        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
                        break;
                    }
                }
            }
        }
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
            datd->init(file->desc.name, cre, mod, exp);
            if (datd->exec()){
                datd->retDates(&cre, &mod, &exp);
                ccos_set_creation_date(ccdesc[acdisk], file, cre);
                ccos_set_mod_date(ccdesc[acdisk], file, mod);
                ccos_set_exp_date(ccdesc[acdisk], file, exp);
                isch[acdisk] = true;
                fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
            }
        }
    }
}

void MainWindow::DebTrace(){
    if (!ui->actionDebtrace->isChecked()){
        TRACE("ccos_image debug trace disabled");
    }

    trace_init(ui->actionDebtrace->isChecked());

    if (ui->actionDebtrace->isChecked()){
        TRACE("ccos_image debug trace enabled");
    }
}

void MainWindow::Delete(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 7 && inodeon[acdisk][called[0]->row()] == 0)
            return;
        bool selpar = (inodeon[acdisk][called[0]->row()] == 0) ? 1 : 0;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to delete %1 file(s)?").arg((called.size()/7)-selpar));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
        for (int t = 0; t< called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0)
                continue;
            ccos_delete_file(ccdesc[acdisk], dat[acdisk], siz[acdisk], inodeon[acdisk][called[t]->row()]);
        }
        isch[acdisk] = 1;
        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
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
        for (int i = 0; i < urlList.size(); ++i){
            QString file = urlList.at(i).toLocalFile();
            if (QFileInfo(file).isDir())
                DirsList.append(file);
            else
                FilesList.append(file);
        }
    }
    if (FilesList.size() == 1 && DirsList.size() == 0) {
        QString ext = QFileInfo(FilesList[0]).suffix().toLower();
        if (ext == "img"){
            LoadImg(FilesList[0]);
        }
        else if (isop[acdisk] && nrot[acdisk])
            AddFiles(FilesList, curdir[acdisk]);
    }
    else if (!nrot[acdisk] && isop[acdisk]) {
        AddDirs(DirsList);

    }
    else if (isop[acdisk] && nrot[acdisk])
            AddFiles(FilesList, curdir[acdisk]);
}

void MainWindow::Extract(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 7 && inodeon[acdisk][called[0]->row()] == 0)
            return;
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == "")
            return;
        for (int t = 0; t < called.size(); t+=7){
            if (inodeon[acdisk][called[t]->row()]==0)
                continue;
            if (ccos_is_dir(inodeon[acdisk][called[t]->row()]))
                dumpDirQt(ccdesc[acdisk], inodeon[acdisk][called[t]->row()], dat[acdisk], todir, this);
            else
                dumpFileQt(ccdesc[acdisk], inodeon[acdisk][called[t]->row()], dat[acdisk], todir, this);
        }
    }
}

void MainWindow::ExtractAll(){
    if (isop[acdisk]){
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == "")
            return;
        int res = dumpImgQt(ccdesc[acdisk], dat[acdisk], siz[acdisk], todir, QFileInfo(name[acdisk]).baseName(), this);
        if (res == -1){
            QMessageBox::critical(this, "Unable to extract image", "Unable to extract image. Please check the path.");
        }
    }
}

void MainWindow::FocusChanged(QWidget *, QWidget *now){
    if (now == ui->tableWidget || now == ui->tableWidget_2){
        acdisk = (now == ui->tableWidget_2);

        QFont font = ui->groupBox->font();
        font.setBold(!acdisk);
        ui->groupBox->setFont(font);
        ui->tableWidget->setFont(font);

        font = ui->groupBox_2->font();
        font.setBold(acdisk);
        ui->groupBox_2->setFont(font);
        ui->tableWidget_2->setFont(font);

        HDDMenu(hddmode[acdisk]);
    }
}

void MainWindow::HDDMenu(bool enab){
    ui->actionAct_part->setEnabled(enab);
    ui->actionAno_part->setEnabled(enab);
    ui->actionSep_save->setEnabled(enab);
}

void MainWindow::Label(){
    if (isop[acdisk]){
        QString dsk;
        if (acdisk == 0)
            dsk= "I";
        else
            dsk= "II";
        char* fname = ccos_get_image_label(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
        QString nameQ = QInputDialog::getText(this, tr("New label"),
                                              QString("Set new label for the disk %1:").arg(dsk), QLineEdit::Normal, fname);

        if (validString(nameQ, false, this) == -1)
            return;

        ccos_set_image_label(ccdesc[acdisk], dat[acdisk], siz[acdisk], nameQ.toStdString().c_str());
        isch[acdisk] = true;
        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::LoadImg(QString path){
    if (path == "")
        return;

    if (dat[acdisk] != NULL || hdddat[acdisk] != NULL)
        if (!CloseImg()) return;

    if (hddmode[!acdisk] && path == name[!acdisk]){
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText("You are trying to open a hard disk image that is already\nopen in another panel.\n"
                       "Would you like to just open another partition of this disk?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::Yes){
            AnotherPart(false);
            return;
        }
    }

    if (readFileQt(path, &dat[acdisk], &siz[acdisk], this) == -1)
        return;

    ccos_inode_t* root;

    if (siz[acdisk] > 0x200 && dat[acdisk][0x1FE] == 0x55 && dat[acdisk][0x1FF] == 0xAA){ // MBR detected - HDD
        hdddat[acdisk] = dat[acdisk];
        hddsiz[acdisk] = siz[acdisk];

        mbr_part_t parts[4] = {0, 0, 0, 0};
        int grids = parseMbr(hdddat[acdisk], parts);

        if (grids == 0){
            QMessageBox::critical(this, "MBR: No GRiD partitions",
                                  "No GRiD partitions found on the disk!");
            free(hdddat[acdisk]);
            hdddat[acdisk] = nullptr;
            return;
        }

        chsd = new ChsDlg(this);
        chsd->setName("MBR: Select disk partition");
        chsd->setInfo("Hard disk with MBR detected.\nSelect the GRiD disk partition you want to work with:");

        for (int i = 0; i < 4; i++){
            if (parts[i].isgrid && parts[i].active){
                chsd->addItem(QString("Partition %1, active").arg(i+1));
            }
            else if (parts[i].isgrid){
                chsd->addItem(QString("Partition %1").arg(i+1));
            }
        }

        if (chsd->exec() == 1){
            int selctd = chsd->getIndex();
            siz[acdisk] = parts[selctd].size;
            dat[acdisk] = hdddat[acdisk]+parts[selctd].offset;

            ccdesc[acdisk] = new ccfs_context_t({ 512, 0x121, 0x120 }); // Standard Floppy / HDD

            root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
            if (root == NULL){
                QMessageBox::critical(this, "Incorrect partition",
                                      "Bad partition or non-GRiD format!");
                free(hdddat[acdisk]);
                hdddat[acdisk] = nullptr;
                dat[acdisk] = nullptr;
                return;
            }
            hddmode[acdisk] = true;
            HDDMenu(true);
        }
        else{
            return;
        }
    }
    else { // Floppy or Bubble
        ccdesc[acdisk] = new ccfs_context_t({ 512, 0x121, 0x120 }); // Standard Floppy / HDD
        root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
        if (root == NULL){ // Falied? Maybe it's a Bubble?
            ccdesc[acdisk] = new ccfs_context_t({ 256, 0x3FE, 0x3FD }); // Standard Bubble
            root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
            if (root == NULL){ // Unknown disk!
                QMessageBox msgBox(this);
                msgBox.setIcon(QMessageBox::Question);
                msgBox.setText("Failed to automatically detect image type!\n"
                               "Image may be broken, have non-GRiD format or custom parameters.\n"
                               "Do you want to set these parameters manually or cancel operation?");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                if (msgBox.exec() != QMessageBox::Yes)
                    return;

                cstdlg = new CustDlg(this);
                cstdlg->OpenMode();

                if (cstdlg->exec() == 1) {
                    uint16_t isize, sect, subl;
                    cstdlg->GetParams(&isize, &sect, &subl);

                    ccdesc[acdisk] = new ccfs_context_t({ sect, subl, static_cast<uint16_t>(subl-1) });

                    root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
                    if (root == NULL){
                        QMessageBox::critical(this, "Failed to open",
                                              "Failed to open this file with specified parameters!");
                        free(hdddat[acdisk]);
                        dat[acdisk] = nullptr;
                        return;
                    }
                }
                else {
                    free(dat[acdisk]);
                    dat[acdisk] = nullptr;
                    return;
                }
            }
        }
    }

    isop[acdisk] = 1;
    name[acdisk] = path;
    curdir[acdisk] = root;
    fillTable(ccdesc[acdisk], root, 0, dat[acdisk], siz[acdisk], acdisk, ui);
}

void MainWindow::MakeDir(){
    if (isop[acdisk]){
        if (nrot[acdisk]){
            QMessageBox::information(this, "Make dir",
                               "GRiD supports directories only in root!");
            return;
        }
        QString name;
        while (true){
            name = QInputDialog::getText(this, tr("Make dir"),
                                         tr("New directory name:"), QLineEdit::Normal, name);
            if (name == "")
                break;
            else if (validString(name, true, this) != -1){
                size_t frsp = ccos_calc_free_space(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
                if (frsp < 1024) {
                    QMessageBox::critical(this, "Not enough space",
                                    QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frsp));
                    break;
                }
                ccos_inode_t* root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
                if (ccos_create_dir(ccdesc[acdisk], root, name.toStdString().c_str(), dat[acdisk], siz[acdisk]) == NULL){
                    QMessageBox::critical(this, "Failed to create folder",
                                    "Program can't create a folder in the image!");
                    break;
                }
                isch[acdisk] = true;
                fillTable(ccdesc[acdisk], root, nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
                break;
            }
        }
    }
}

void MainWindow::NewImage(){
    if (dat[acdisk] != NULL)
        if (!CloseImg()) return;

    cstdlg = new CustDlg(this);

    if (cstdlg->exec() == 1) {
        uint16_t isize, sect, subl;
        cstdlg->GetParams(&isize, &sect, &subl);

        ccdesc[acdisk] = new ccfs_context_t({ sect, subl, static_cast<uint16_t>(subl-1) });

        siz[acdisk] = isize * 1024;
        dat[acdisk] = ccos_create_new_image(ccdesc[acdisk], (isize * (1024 / sect)));

        isch[acdisk] = true;
        isop[acdisk] = true;
        ccos_inode_t* root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
        curdir[acdisk] = root;
        fillTable(ccdesc[acdisk], root, false, dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::OpenDir(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QTableWidgetItem* called = tw->currentItem();
        ccos_inode_t *dir = inodeon[acdisk][called->row()];
        if (dir == NULL && nrot[acdisk]== 0)
            return;
        if (called->row() == 0 && nrot[acdisk]){
            ccos_inode_t* root = ccos_get_root_dir(ccdesc[acdisk], dat[acdisk], siz[acdisk]);
            curdir[acdisk] = (ccos_get_parent_dir(ccdesc[acdisk], curdir[acdisk], dat[acdisk]));
            if (curdir[acdisk] == root)
                nrot[acdisk] = false;
            fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
        else if (!nrot[acdisk]){ //All files in the root are directories
            curdir[acdisk] = dir;
            nrot[acdisk] = true;
            fillTable(ccdesc[acdisk], dir, nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
        }
    }
}

void MainWindow::OpenImg(){
    QString Qname = QFileDialog::getOpenFileName(this, "Open Image", "",
                                                 "GRiD image files (*.img);;"
                                                 "All files (*)");
    if (Qname != "")
        LoadImg(Qname);
}

void  MainWindow::Rename(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.empty())
            return;
        if (called.size() == 7 && inodeon[acdisk][called[0]->row()] == NULL)
            return;

        for (int i = 0; i < called.size(); i+=7){
            ccos_inode_t* reninode = inodeon[acdisk][called[i]->row()];
            if (reninode == NULL){
                continue;
            }

            char basename[CCOS_MAX_FILE_NAME];
            char type[CCOS_MAX_FILE_NAME];
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            memset(type, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(reninode, basename, type, NULL, NULL);
            rnam = new RenDlg(this);
            rnam->setName(basename);
            rnam->setType(type);
            rnam->setInfo((QString("Set new name and type for %1:").arg(reninode->desc.name)));
            if (!nrot[acdisk]){ //All files in the root are directories
                rnam->lockType(1);
            }
            while (true){
                if (rnam->exec() == 1){
                    QString newname = rnam->getName();
                    QString newtype = rnam->getType();
                    if (newtype.contains("subject", Qt::CaseInsensitive) && nrot[acdisk]){
                        QMessageBox::critical(this, "Incorrect Type",
                                              "Can't set directory type for file!");
                    }
                    else if (newname == "" or newtype == ""){
                        QMessageBox::critical(this, "Incorrect Name or Type",
                                              "File name or type can't be empty!");
                    }
                    else if (validString(newname, true, this) != -1 && validString(newtype, true, this) != -1){
                        ccos_rename_file(ccdesc[acdisk], dat[acdisk], siz[acdisk], reninode, newname.toStdString().c_str(),
                                         newtype.toStdString().c_str());
                        isch[acdisk] = true;
                        fillTable(ccdesc[acdisk], ccos_get_parent_dir(ccdesc[acdisk], reninode, dat[acdisk]),
                                  nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
                        break;
                    }
                }
                else{
                    break;
                }
            }
        }
    }
}

void MainWindow::Save(){
    if (name[acdisk] == "")
        return SaveAs();

    QGroupBox* gb;
    if (isch[acdisk]){
        if (acdisk == 0)
            gb = ui->groupBox;
        else
            gb = ui->groupBox_2;

        int res;
        if (hddmode[acdisk]){
            res = saveFileQt(name[acdisk], hdddat[acdisk], hddsiz[acdisk], this);
        }
        else{
            res = saveFileQt(name[acdisk], dat[acdisk], siz[acdisk], this);
        }

        if (res == -1){
            QMessageBox::critical(this, "Unable to save file",
                            QString("Unable to save file \"%1\". Please check the path.").arg(name[acdisk]));
            return;
        }
        isch[acdisk] = 0;
        if (oneimg)
            isch[!acdisk] = 0;
        gb->setTitle(gb->title().left(gb->title().size()-1));
    }
}

void MainWindow::SaveAs(){
    QGroupBox* gb;
    if (isop[acdisk]){
        if (acdisk == 0)
            gb = ui->groupBox;
        else
            gb = ui->groupBox_2;
        QString nameQ = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
        if (nameQ == "")
            return;

        int res;
        if (hddmode[acdisk]){
            res = saveFileQt(nameQ, hdddat[acdisk], hddsiz[acdisk], this);
        }
        else{
            res = saveFileQt(nameQ, dat[acdisk], siz[acdisk], this);
        }

        if (res == -1){
            QMessageBox::critical(this, "Unable to save file",
                                  QString("Unable to save file \"%1\". Please check the path.").arg(nameQ));
            return;
        }
        name[acdisk] = nameQ;
        if (isch[acdisk]){
            gb->setTitle(gb->title().left(gb->title().size()-1));
            isch[acdisk] = 0;
            if (oneimg)
                isch[!acdisk] = 0;
        }
    }
}

void MainWindow::SavePart(){
    hddmode[acdisk] = 0;
    bool oldch = isch[acdisk];
    isch[acdisk] = 1;
    QGroupBox* gb;
    if (acdisk == 0)
        gb = ui->groupBox;
    else
        gb = ui->groupBox_2;
    gb->setTitle(gb->title()+' ');
    SaveAs();

    if (!isch[acdisk]){
        uint8_t* imdat = (uint8_t*)calloc(siz[acdisk], sizeof(uint8_t));
        memcpy(imdat, dat[acdisk], siz[acdisk]);
        dat[acdisk] = imdat;

        free(hdddat[acdisk]);
        hdddat[acdisk] = nullptr;
    }
    else{
        hddmode[acdisk] = 1;
        isch[acdisk] = oldch;
        gb->setTitle(gb->title().left(gb->title().size()-1));
    }
}

void MainWindow::SetActivePart(){
    uint8_t* mbrtab = hdddat[acdisk] + 0x1BE;

    mbr_part_t parts[4] = {0, 0, 0, 0};
    int grids = parseMbr(hdddat[acdisk], parts);

    if (grids == 0){
        QMessageBox::critical(this, "No GRiD partitions",
                              "No GRiD partitions found on the disk!");
        return;
    }

    chsd = new ChsDlg(this);
    chsd->setName("Select disk partition");
    chsd->setInfo("Select the GRiD disk partition to make it active:");

    chsd->addItem("No active partition");

    for (int i = 0; i < 4; i++){
        if (parts[i].isgrid)
            chsd->addItem(QString("Partition %1").arg(i+1));
    }

    if (chsd->exec() == 1){
        int selctd = chsd->getIndex();

        mbrtab[0] = 0x0;
        mbrtab[16] = 0x0;
        mbrtab[32] = 0x0;
        mbrtab[48] = 0x0;
        if (selctd > 0)
            mbrtab[(selctd-1)*16] = 0x80;

        isch[acdisk] = true;
        fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
    }
}

void MainWindow::Version(){
    if (isop[acdisk]){
        QTableWidget* tw;
        if (acdisk == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        ccos_inode_t* file = inodeon[acdisk][tw->currentItem()->row()];
        if (file != 0){
            version_t ver = ccos_get_file_version(file);
            vdlg = new VerDlg(this);
            vdlg->init(file->desc.name, ver);
            if (vdlg->exec() == 1){
                ver = vdlg->retVer();
                ccos_set_file_version(ccdesc[acdisk], file, ver);
                isch[acdisk] = true;
                fillTable(ccdesc[acdisk], curdir[acdisk], nrot[acdisk], dat[acdisk], siz[acdisk], acdisk, ui);
            }
        }
    }
}

MainWindow::~MainWindow(){
    for (int i = 0; i < 2; i++){
        if (hdddat[i] != nullptr){
            if (!oneimg){
                free(hdddat[i]);

            }
            oneimg = false;
            hdddat[i] = nullptr;
            dat[i] = nullptr;
        }
        else if (dat[i] != nullptr) {
            free(dat[i]);
            dat[i] = nullptr;
        }
    }
    delete ui;
}
