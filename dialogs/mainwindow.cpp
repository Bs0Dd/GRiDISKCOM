#include "mainwindow.h"

// GRiD disk/firmware "magic" constants for geometry/superblock/bitmap

// Extracted from bubble image.
#define GRID_BUBBLE_SECTOR_SIZE         256
#define GRID_BUBBLE_SUPERBLOCK_FID      0x3FE
#define GRID_BUBBLE_ALT_SUPERBLOCK_FID  0x121
#define GRID_BUBBLE_BITMAP_FID          0x3FD
#define GRID_BUBBLE_ALT_BITMAP_FID      0x120

// Extracted from 2102 firmware.
#define GRID_FLOPPY_SECTOR_SIZE     512
#define GRID_FLOPPY_SUPERBLOCK_FID  0x121
#define GRID_FLOPPY_BITMAP_FID      0x120

// Extracted from 2101 firmware.
#define GRID_HDD_SECTOR_SIZE        512
#define GRID_HDD_SUPERBLOCK_FID     0x2420
#define GRID_HDD_BITMAP_FID         0x2400

//[Service functions]

void replace_char_in_place(char* src, char from, char to) {
  for (int i = 0; i < strlen(src); ++i) {
    if (src[i] == from) {
      src[i] = to;
    }
  }
}

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
int tildaCheck(std::string parse_str){
    std::vector<std::string> output;

    size_t pos = 0;
    std::string token;
    while ((pos = parse_str.find("~")) != std::string::npos) {
        token = parse_str.substr(0, pos);
        output.push_back(token);
        parse_str.erase(0, pos + 1);
    }
    if (output.size() == 1)
        output.push_back(parse_str);

    if (output.empty() || output.size() > 2)
        return -1;

    return 0;
}

//*Check if space is enough to add files
int checkFreeSp(DiskPanel& from, DiskPanel& to,
                QList<QTableWidgetItem *> calledElems,
                size_t* needs){ //*For copy

    size_t frsp = ccos_calc_free_space(&to.disk);
    if (frsp == -1){
        return -2;
    }
    *needs = 0;
    for (int i = 0; i < calledElems.size(); i+=7){
        ccos_inode_t* file = from.inodes[calledElems[i]->row()];
        if (file != nullptr){
            if (ccos_is_dir(file)){
                uint16_t fils = 0;
                ccos_inode_t** dirdata = nullptr;
                ccos_get_dir_contents(&from.disk, file, &fils, &dirdata);

                for(int j = 0; j < fils; j++){
                    *needs += ccos_get_file_size(dirdata[j]);
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

int checkFreeSp(ccos_disk_t* disk, QStringList files, size_t* needs){ //*For add
    size_t frsp = ccos_calc_free_space(disk);
    if (frsp == -1){
        return -2;
    }
    *needs = 0;
    for (const auto& file : files) {
        *needs += QFileInfo(file).size();
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
int dumpFileQt(ccos_disk_t* disk, ccos_inode_t* file, QString path, QWidget* parent){
    char* fnam = short_string_to_string(ccos_get_file_name(file));
    replace_char_in_place(fnam, '/', '_');

    QString fpath = QDir(path).filePath(fnam);

    size_t file_size = 0;
    uint8_t* file_data = nullptr;

    if (ccos_read_file(disk, file, &file_data, &file_size) == -1){
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
int dumpDirQt(ccos_disk_t* disk, ccos_inode_t* dir, QString path, QWidget* parent){
    char name[CCOS_MAX_FILE_NAME];
    memset(name, 0, CCOS_MAX_FILE_NAME);
    ccos_parse_file_name(dir, name, nullptr, nullptr, nullptr);
    replace_char_in_place(name, '/', '_');

    QString dpath = QDir(path).filePath(name);

    if (!QDir(dpath).exists() && !QDir().mkdir(dpath)){
            QMessageBox::critical(parent, "Failed to create directory",
                          QString("Failed to create directory \"%1\"!").arg(name));
            return -1;
    }

    uint16_t fils = 0;
    ccos_inode_t** dirdata = nullptr;
    ccos_get_dir_contents(disk, dir,&fils, &dirdata);

    for(int i = 0; i < fils; i++){
        if (ccos_is_dir(dirdata[i]))
            dumpDirQt(disk, dirdata[i], dpath, parent);
        else
            dumpFileQt(disk, dirdata[i], dpath, parent);
    }

    free(dirdata);
    return 0;
}

//*Dump full image to path
int dumpImgQt(ccos_disk_t* disk, QString path, QString altname, QWidget* parent){
    ccos_inode_t* root_dir = ccos_get_root_dir(disk);
    if (root_dir == nullptr) {
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

    return dumpDirQt(disk, root_dir, dpath, parent);
}

//*Check if string is valid for CCOS (does not contain unicode and reserved characters)
int validString(QString string, bool ifpath, QWidget* parent){
    for (const auto& ch : string){
        if (ch > 256 || (ifpath && (ch == '`' || ch == '|' || ch == '~'))){
            QMessageBox::critical(parent, "Incorrect characters", "Invalid character(s) were found! Remove them.");
            return -1;
        }
    }
    return 0;
}

//*Get directory listing and fill it to table
void MainWindow::fillTable(int panel_idx, ccos_inode_t* directory, bool noRoot) {
    auto& panel = *panels[panel_idx];
    QTableWidget* tableWidget;
    QLabel* label;
    QGroupBox* box;
    QString disk_name, msg;
    panel.inodes.clear();
    uint16_t fils = 0;
    ccos_inode_t** dirdata = nullptr;
    ccos_get_dir_contents(&panel.disk, directory, &fils, &dirdata);
    if (panel_idx == 0){
        tableWidget = ui->tableWidget;
        label = ui->label;
        box = ui->groupBox;
        disk_name = "I";
    }
    else{
        tableWidget = ui->tableWidget_2;
        label = ui->label_2;
        box = ui->groupBox_2;
        disk_name = "II";
    }
    for (int row = tableWidget->rowCount(); 0<=row; row--)
        tableWidget->removeRow(row);
    char* labd = ccos_get_image_label(&panel.disk);
    msg = "Disk %1 - %2%3";
    box->setTitle(msg.arg(disk_name, (strlen(labd) != 0) ? labd : "No label", panel.modified ? "*" : ""));
    free(labd);
    char basename[CCOS_MAX_FILE_NAME];
    char type[CCOS_MAX_FILE_NAME];
    if (noRoot){
        addFile(tableWidget, 2);
        panel.inodes.insert(panel.inodes.begin(), nullptr);
    }
    for(int c = 0; c < fils; c++){
        memset(basename, 0, CCOS_MAX_FILE_NAME);
        memset(type, 0, CCOS_MAX_FILE_NAME);
        ccos_parse_file_name(dirdata[c], basename, type, nullptr, nullptr);
        QString qtype = type;
        if (!noRoot) //All files in the root are directories
            qtype = qtype + " <DIR>";
        QString text[] = {basename, qtype, QString::number(ccos_get_file_size(dirdata[c])),
                         ccosGetFileVersionQstr(dirdata[c]),
                         ccosDateToQstr(ccos_get_creation_date(dirdata[c])),
                         ccosDateToQstr(ccos_get_mod_date(dirdata[c])),
                         ccosDateToQstr(ccos_get_exp_date(dirdata[c]))};
        panel.inodes.push_back(dirdata[c]);
        addFile(tableWidget, text);
    }

    size_t free_space = ccos_calc_free_space(&panel.disk);
    if (free_space == -1){
        label->setText("Free space: FAILED TO CALCULATE!");
        free(dirdata);
        return;
    }

    msg = "Free space: %1 bytes.";
    label->setText(msg.arg(free_space));
    if (panel.inodes.empty()) {
        addFile(tableWidget, 1);
        panel.inodes.insert(panel.inodes.begin(), nullptr);
    }

    free(dirdata);
}

//*Parse Hard Disk MBR
int parseMbr(const uint8_t* hdddata, std::array<mbr_part_t, 4>& parts){
    const uint8_t* mbrtab = hdddata + 0x1BE;

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

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(std::make_unique<Ui::MainWindow>()){
    ui->setupUi(this);

    // FIXME: How to enable trace in new version of ccos-disk-utils?
    // trace_init(false);

    QMainWindow::setWindowTitle(QString("GRiDISK Commander v")+_PVER_);
    for (auto* tw : {ui->tableWidget, ui->tableWidget_2}) {
        tw->horizontalHeader()->resizeSection(0, 155);
        tw->horizontalHeader()->resizeSection(2, 45);
        tw->horizontalHeader()->resizeSection(3, 80);
        tw->horizontalHeader()->resizeSection(4, 80);
        tw->horizontalHeader()->resizeSection(5, 80);
        tw->horizontalHeader()->resizeSection(6, 80);
        addFile(tw, 0);
        tw->verticalHeader()->hide();
        tw->setSelectionBehavior(QAbstractItemView::SelectRows);
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
            else if (!panels[0] || !panels[1]){
                QFileInfo fil(arg);
                if (fil.suffix().toLower() == "img" && fil.exists()){
                    active_panel = panels[0] ? 1 : 0;
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
    AbDlg dlg(this);
    dlg.exec();
}

void MainWindow::Add(){
    if (panels[active_panel]){
        if (!panels[active_panel]->in_subdir){
            QMessageBox::information(this, tr("Add file(s)"),
                               tr("GRiD supports files only in directories!"));
            return;
        }
        QStringList files = QFileDialog::getOpenFileNames(
                    this, "Select files to add");
        AddFiles(files, panels[active_panel]->current_dir);
    }
}

void MainWindow::AddDirs(QStringList dirs){
    auto& panel = *panels[active_panel];
    ccos_inode_t* root = ccos_get_root_dir(&panel.disk);
    for (const auto& dir : dirs) {
        size_t frees = ccos_calc_free_space(&panel.disk);
        if (frees == -1){
            QMessageBox::critical(this, "Calculation error",
                            "Program can't calculate free space in the image!");
            break;
        }

        if (frees < 1024) {
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frees));
            break;
        }
        ccos_inode_t* newdir = ccos_create_dir(&panel.disk, root, QFileInfo(dir).fileName().toStdString().c_str());
        if (newdir == nullptr){
            QMessageBox::critical(this, "Failed to create folder",
                            "Program can't create a folder in the image!");
            break;
        }
        QDir scandir(dir);
        QFileInfoList filesInfo = scandir.entryInfoList(QDir::Files);
        QStringList files;
        for (const auto& fileInfo : filesInfo){
            files.append(fileInfo.absoluteFilePath());
        }
        if (AddFiles(files, newdir) == -1) {
          break;
        }
    }
    if (!dirs.empty()){
        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
}

int MainWindow::AddFiles(QStringList files, ccos_inode_t* copyTo){
    auto& panel = *panels[active_panel];
    size_t needs = 0;
    int retop = checkFreeSp(&panel.disk, files, &needs);
    if (retop == -2) {
        QMessageBox::critical(this, "Calculation error",
                        "Program can't calculate free space in the image!");
        return -1;
    }
    else if (retop == -1) {
        size_t free = ccos_calc_free_space(&panel.disk);
        QMessageBox::critical(this, "Not enough space",
                        QString("Requires %1 bytes of additional disk space to add!").arg(needs-free));
        return -1;
    }
    for (const auto& file : files){
        uint8_t* fdat = nullptr;
        size_t fsiz = 0;
        std::string fname = QFileInfo(file).fileName().toStdString();
        if (tildaCheck(fname) == -1){
            RenDlg dlg(this);
            dlg.setInfo(QString("Set correct name and type for %1:").arg(fname.c_str()));
            while (true){
                if (dlg.exec() == 1){
                    if (dlg.getType().toLower().contains("subject")){
                        QMessageBox::critical(this, "Incorrect Type",
                                        "Can't set directory type for file!");
                    }
                    else if (dlg.getName() == "" or dlg.getType() == ""){
                        QMessageBox::critical(this, "Incorrect Name or Type",
                                        "File name or type can't be empty!");
                    }
                    else{
                        if (validString(dlg.getName(), true, this) != -1 && validString(dlg.getType(), true, this) != -1){
                            QString crnam = "%1~%2~";
                            fname = crnam.arg(dlg.getName(), dlg.getType()).toStdString();
                            break;
                        }
                    }
                }
                else
                    return -1;
            }
        }
        if (readFileQt(file, &fdat, &fsiz, this) == 0){
            if (ccos_add_file(&panel.disk, copyTo, fdat, fsiz, fname.c_str()) == nullptr){
                QMessageBox::critical(this, "Error",
                                QString("Can't add \"%1\" to the image! Skipping...").arg(fname.c_str()));
            }
        }
    }
    if (files.size() != 0){
        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
    return 0;
}

void MainWindow::AnPartMenu(){
    AnotherPart(true);
}

void MainWindow::AnotherPart(bool fromMenu){
    int usedisk;
    if (fromMenu)
        usedisk = active_panel;
    else
        usedisk = !active_panel;

    auto& src = *panels[usedisk];

    std::array<mbr_part_t, 4> parts;
    parseMbr(src.hdd_data->data(), parts);

    ChsDlg dlg(this);
    dlg.setName("Select disk partition");
    dlg.setInfo("Select the GRiD disk partition you want to work with:");

    if (fromMenu)
        dlg.enCheckBox();

    for (int i = 0; i < 4; i++){
        if (parts[i].isgrid && parts[i].active){
            dlg.addItem(QString("Partition %1, active").arg(i+1));
        }
        else if (parts[i].isgrid){
            dlg.addItem(QString("Partition %1").arg(i+1));
        }
    }

    if (dlg.exec() == 1){
        int selctd = dlg.getIndex();

        int topan = (fromMenu && dlg.isChecked()) ? !active_panel : active_panel;

        if (usedisk != topan && panels[topan])
            CloseImg();

        if (!panels[topan])
            panels[topan].emplace();

        auto& dst = *panels[topan];
        dst.disk = src.disk;
        dst.disk.size = parts[selctd].size;
        dst.disk.data = src.hdd_data->data() + parts[selctd].offset;

        ccos_inode_t* root = ccos_get_root_dir(&dst.disk);
        if (root == nullptr){
            QMessageBox::critical(this, "Incorrect Image File",
                                  "Image broken or have non-GRiD format!");
            if (usedisk == topan){
                src.hdd_data.reset();
                src.hdd_mode = false;
            }
            panels[topan].reset();
            return;
        }
        dst.hdd_mode = true;
        dst.current_dir = root;
        if (!fromMenu || dlg.isChecked()){
            dst.path = src.path;
            dst.hdd_data = src.hdd_data;
        }

        fillTable(topan, root, false);
    }
}

void MainWindow::closeEvent(QCloseEvent *event){
    int backdstat = active_panel;
    for (int i = 0; i < 2; i++){
        if (panels[i] && panels[i]->modified){
            int ret = saveBox(i == 0 ? "I" : "II", this);
            if (ret == 1){
                active_panel = i;
                Save();
                active_panel = backdstat;
            }
            else if (ret == 0)
                event->ignore();
        }
    }
}

int MainWindow::CloseImg(){
    if (panels[active_panel] && panels[active_panel]->modified){
        int ret = saveBox((active_panel == 0) ? "I" : "II", this);
        if (ret == 1)
            MainWindow::Save();
        else if (ret == 0)
            return 0;
    }

    panels[active_panel].reset();
    HDDMenu(false);

    QTableWidget* tableWidget;
    if (active_panel == 0){
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
    int other = !active_panel;
    if (panels[active_panel] && panels[other]){
        auto& src = *panels[active_panel];
        auto& dst = *panels[other];
        QTableWidget* tw;
        if (active_panel == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.isEmpty())
            return;
        if (called.size() == 7 && src.inodes[called[0]->row()] == nullptr)
            return;
        bool selpar = (src.inodes[called[0]->row()] == nullptr) ? true : false;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to copy %1 file(s)?").arg((called.size()/7)-selpar));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
        size_t needs = 0;
        int retop = checkFreeSp(src, dst, called, &needs);
        if (retop == -2) {
            QMessageBox::critical(this, "Calculation error",
                            "Program can't calculate free space in the image!");
            return;
        }
        else if (retop == -1) {
            size_t frsp = ccos_calc_free_space(&dst.disk);
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to copy").arg(needs-frsp));
            return;
        }
        for (int t = 0; t < called.size(); t+=7){
            if (src.inodes[called[t]->row()]==nullptr)
                continue;
            if (ccos_is_dir(src.inodes[called[t]->row()])) {
                if (ccos_file_id(dst.current_dir) != ccos_file_id(ccos_get_root_dir(&dst.disk))) {
                    QMessageBox::critical(this, "Copying to non-root",
                                    "Folders can be copied only to root folder!");
                    return;
                }
                char newname[CCOS_MAX_FILE_NAME] = {};
                ccos_parse_file_name(src.inodes[called[t]->row()], newname, nullptr, nullptr, nullptr);
                ccos_inode_t* newdir = ccos_create_dir(&dst.disk, ccos_get_root_dir(&dst.disk), newname);
                if (newdir == nullptr){
                            QMessageBox::critical(this, "Failed to create folder",
                                            "Program can't create a folder in the image!");
                            return;
                }
                uint16_t fils = 0;
                ccos_inode_t** dirdata = nullptr;
                ccos_get_dir_contents(&src.disk, src.inodes[called[t]->row()], &fils, &dirdata);
                for (int c = 0; c < fils; c++) {
                    ccos_copy_file(&src.disk, dirdata[c], &dst.disk, newdir);
                }
            }
            else {
                if (ccos_file_id(dst.current_dir) == ccos_file_id(ccos_get_root_dir(&dst.disk))) {
                    QMessageBox::critical(this, "Copying to root",
                                    "Files can be copied only to non-root folder!");
                    return;
                }
                ccos_copy_file(&src.disk, src.inodes[called[t]->row()],
                        &dst.disk, dst.current_dir);
            }
        }
        dst.modified = true;
        fillTable(other, dst.current_dir, dst.in_subdir);
        fillTable(active_panel, src.current_dir, src.in_subdir);
    }
}

void MainWindow::CopyLoc() {
    if (!panels[active_panel]) {
        return;
    }

    auto& panel = *panels[active_panel];

    QTableWidget const* tw;
    if (active_panel == 0)
        tw = ui->tableWidget;
    else
        tw = ui->tableWidget_2;

    QList<QTableWidgetItem *> called = tw->selectedItems();
    if (called.isEmpty()) {
        return;
    }

    if (called.size() == 7 && panel.inodes[called[0]->row()] == nullptr) {
        return;
    }

    size_t needs = 0;
    int retop = checkFreeSp(panel, panel, called, &needs);
    if (retop == -2) {
        QMessageBox::critical(this, "Calculation error",
                        "Program can't calculate free space in the image!");
        return;
    }
    else if (retop == -1) {
        size_t frsp = ccos_calc_free_space(&panel.disk);
        QMessageBox::critical(this, "Not enough space",
                        QString("Requires %1 bytes of additional disk space to copy").arg(needs-frsp));
        return;
    }

    if (panel.in_subdir){
        ChsDlg dlg(this);
        dlg.setName("Select the directory");
        dlg.setInfo("Select the directory where the file(s) will be copied:");

        ccos_inode_t* root = ccos_get_root_dir(&panel.disk);

        uint16_t fils = 0;
        ccos_inode_t** dirdata = nullptr;
        ccos_get_dir_contents(&panel.disk, root, &fils, &dirdata);

        char basename[CCOS_MAX_FILE_NAME];

        for(int i = 0; i < fils; i++){
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(dirdata[i], basename, nullptr, nullptr, nullptr);
            dlg.addItem(basename);
        }
        dlg.exec();

        ccos_inode_t* firfil = panel.inodes[called[0]->row()] == nullptr ?
                    panel.inodes[called[6]->row()] : panel.inodes[called[0]->row()];

        if (dirdata[dlg.getIndex()]->header.file_id == firfil->desc.dir_file_id){
            QMessageBox::critical(this, "Copy to parent dir",
                                    "Can't copy files to it's parent dir!");
            return;
        }

        for (int t = 0; t < called.size(); t+=7){
            if (panel.inodes[called[t]->row()]==nullptr)
                continue;
            ccos_copy_file(&panel.disk, panel.inodes[called[t]->row()],
                    &panel.disk, dirdata[dlg.getIndex()]);
        }
        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
    else{
        for (int t = 0; t < called.size(); t+=7){
            char basename[CCOS_MAX_FILE_NAME];
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(panel.inodes[called[t]->row()], basename, nullptr, nullptr, nullptr);

            QString name;
            while (true){
                name = QInputDialog::getText(this, tr("Copy dir"),
                                                tr("Name for \"%1\" copy:").arg(basename), QLineEdit::Normal, name);
                if (name == "")
                    break;
                else if (validString(name, true, this) != -1){
                    ccos_inode_t* root = ccos_get_root_dir(&panel.disk);

                    ccos_inode_t* newdir = ccos_create_dir(&panel.disk, root, name.toStdString().c_str());
                    if (newdir == nullptr){
                        QMessageBox::critical(this, "Failed to create folder",
                                        "Program can't create a folder in the image!");
                        break;
                    }

                    uint16_t fils = 0;
                    ccos_inode_t** dirdata = nullptr;
                    ccos_get_dir_contents(&panel.disk, panel.inodes[called[t]->row()], &fils, &dirdata);

                    for(int i = 0; i < fils; i++){
                        ccos_copy_file(&panel.disk, dirdata[i],
                                &panel.disk, newdir);
                    }
                    panel.modified = true;
                    fillTable(active_panel, panel.current_dir, panel.in_subdir);
                    break;
                }
            }
        }
    }
}

void MainWindow::Date(){
    if (!panels[active_panel]) {
        return;
    }

    auto& panel = *panels[active_panel];

    QTableWidget const* tw;
    if (active_panel == 0)
        tw = ui->tableWidget;
    else
        tw = ui->tableWidget_2;

    ccos_inode_t* file = panel.inodes[tw->currentItem()->row()];
    if (file == nullptr) {
        return;
    }

    ccos_date_t cre = ccos_get_creation_date(file);
    ccos_date_t mod = ccos_get_mod_date(file);
    ccos_date_t exp = ccos_get_exp_date(file);
    DateDlg dlg(this);
    dlg.init(file->desc.name, cre, mod, exp);
    if (dlg.exec()){
        dlg.retDates(&cre, &mod, &exp);
        ccos_set_creation_date(&panel.disk, file, cre);
        ccos_set_mod_date(&panel.disk, file, mod);
        ccos_set_exp_date(&panel.disk, file, exp);
        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
}

void MainWindow::DebTrace(){
    if (!ui->actionDebtrace->isChecked()){
        TRACE("ccos_image debug trace disabled");
    }

    // FIXME: How to enable trace in new version of ccos-disk-utils?
    // trace_init(ui->actionDebtrace->isChecked());

    if (ui->actionDebtrace->isChecked()){
        TRACE("ccos_image debug trace enabled");
    }
}

void MainWindow::Delete(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QTableWidget* tw;
        if (active_panel == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 7 && panel.inodes[called[0]->row()] == nullptr)
            return;
        bool selpar = (panel.inodes[called[0]->row()] == nullptr) ? true : false;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to delete %1 file(s)?").arg((called.size()/7)-selpar));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
        for (int t = 0; t< called.size(); t+=7){
            if (panel.inodes[called[t]->row()]==nullptr)
                continue;
            ccos_delete_file(&panel.disk, panel.inodes[called[t]->row()]);
        }
        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event){
   if (event->mimeData()->hasUrls())
       event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event){
    QStringList FilesList;
    QStringList DirsList;

    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()){
        QList<QUrl> urlList = mimeData->urls();
        for (const auto& url : urlList) {
            QString file = url.toLocalFile();
            if (QFileInfo(file).isDir())
                DirsList.append(file);
            else
                FilesList.append(file);
        }
    }

    if (FilesList.size() == 1 && DirsList.isEmpty()) {
        QString ext = QFileInfo(FilesList[0]).suffix().toLower();
        if (ext == "img") {
            LoadImg(FilesList[0]);
        }
        else if (panels[active_panel] && panels[active_panel]->in_subdir) {
            AddFiles(FilesList, panels[active_panel]->current_dir);
        }
    }
    else if (panels[active_panel] && !panels[active_panel]->in_subdir) {
        AddDirs(DirsList);
    }
    else if (panels[active_panel] && panels[active_panel]->in_subdir) {
        AddFiles(FilesList, panels[active_panel]->current_dir);
    }
}

void MainWindow::Extract(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QTableWidget* tw;
        if (active_panel == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.size() == 0)
            return;
        if (called.size() == 7 && panel.inodes[called[0]->row()] == nullptr)
            return;
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == "")
            return;
        for (int t = 0; t < called.size(); t+=7){
            if (panel.inodes[called[t]->row()]==nullptr)
                continue;
            if (ccos_is_dir(panel.inodes[called[t]->row()]))
                dumpDirQt(&panel.disk, panel.inodes[called[t]->row()], todir, this);
            else
                dumpFileQt(&panel.disk, panel.inodes[called[t]->row()], todir, this);
        }
    }
}

void MainWindow::ExtractAll(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (todir == "")
            return;
        int res = dumpImgQt(&panel.disk, todir, QFileInfo(panel.path).baseName(), this);
        if (res == -1){
            QMessageBox::critical(this, "Unable to extract image", "Unable to extract image. Please check the path.");
        }
    }
}

void MainWindow::FocusChanged(QWidget *, QWidget *now){
    if (now == ui->tableWidget || now == ui->tableWidget_2){
        active_panel = (now == ui->tableWidget_2) ? 1 : 0;

        QFont font = ui->groupBox->font();
        font.setBold(active_panel == 0);
        ui->groupBox->setFont(font);
        ui->tableWidget->setFont(font);

        font = ui->groupBox_2->font();
        font.setBold(active_panel == 1);
        ui->groupBox_2->setFont(font);
        ui->tableWidget_2->setFont(font);

        HDDMenu(panels[active_panel] && panels[active_panel]->hdd_mode);
    }
}

void MainWindow::HDDMenu(bool enab){
    ui->actionAct_part->setEnabled(enab);
    ui->actionAno_part->setEnabled(enab);
    ui->actionSep_save->setEnabled(enab);
}

void MainWindow::Label(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QString dsk;
        if (active_panel == 0)
            dsk= "I";
        else
            dsk= "II";
        char* fname = ccos_get_image_label(&panel.disk);
        QString nameQ = QInputDialog::getText(this, tr("New label"),
                                              QString("Set new label for the disk %1:").arg(dsk), QLineEdit::Normal, fname);

        if (validString(nameQ, false, this) == -1)
            return;

        ccos_set_image_label(&panel.disk, nameQ.toStdString().c_str());
        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
}

bool MainWindow::isFileAlreadyOpened(const QString& path) {
    int other_panel = !active_panel;
    return panels[other_panel] && panels[other_panel]->path == path;
}

void MainWindow::handleAlreadyOpenedImg(QString path) {
    Q_ASSERT(isFileAlreadyOpened(path));

    if (!panels[!active_panel]->hdd_mode) {
        QMessageBox::critical(this, "Image already open",
                              "This image is already open in the other panel!");
        return;
    }

    if (!suggestSelectAnotherPartition()) {
        return;
    }

    if (!CloseImg()) {
        return;
    }

    AnotherPart(false);
}

bool MainWindow::suggestSelectAnotherPartition() {
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("You are trying to open a hard disk image that is already\nopen in another panel.\n"
                   "Would you like to just open another partition of this disk?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    return msgBox.exec() == QMessageBox::Yes;
}

static std::optional<ccos_disk_t> tryOpenAs(
    uint8_t* data, size_t size, uint16_t sector_size, uint16_t superblock_fid, uint16_t bitmap_fid
) {
    ccos_disk_t disk = { sector_size, superblock_fid, bitmap_fid, size, data };

    // HACK:
    // For some reason, ccos_get_root_dir reads the root directory
    // not from our addresses in the disk structure, but from the image's own data.
    // To prevent this, we temporarily zero out some fields of the zero sector
    // while reading the root directory.
    uint16_t zero_sector_superblock = *(uint16_t*)&data[CCOS_SUPERBLOCK_ADDR_OFFSET];
    *(uint16_t*)&data[CCOS_SUPERBLOCK_ADDR_OFFSET] = 0;

    uint16_t zero_sector_bitmap = *(uint16_t*)&data[CCOS_BITMASK_ADDR_OFFSET];
    *(uint16_t*)&data[CCOS_BITMASK_ADDR_OFFSET] = 0;

    // Open the root directory using values from the disk structure.
    ccos_inode_t* root = ccos_get_root_dir(&disk);
    
    // Restore the values.
    *(uint16_t*)&data[CCOS_SUPERBLOCK_ADDR_OFFSET] = zero_sector_superblock;
    *(uint16_t*)&data[CCOS_BITMASK_ADDR_OFFSET] = zero_sector_bitmap;
    
    if (root == nullptr) {
        return std::nullopt;
    } else {
        return std::optional<ccos_disk_t>(disk);
    }
}

static std::optional<ccos_disk_t> tryFromBootsector(uint8_t* data, size_t size) {
    if (size < 512) {
        return std::nullopt;
    }

    uint16_t superblock = *(uint16_t*)&data[CCOS_SUPERBLOCK_ADDR_OFFSET];
    uint16_t bitmap = *(uint16_t*)&data[CCOS_BITMASK_ADDR_OFFSET];

    if (superblock == 0 || bitmap == 0) {
        return std::nullopt;
    }

    return tryOpenAs(data, size, 512, superblock, bitmap);
}

static std::optional<ccos_disk_t> tryDetectBySize(uint8_t* data, size_t size) {
    if (size == 384 * 1024) {
        return tryOpenAs(data, size,
            GRID_BUBBLE_SECTOR_SIZE, GRID_BUBBLE_SUPERBLOCK_FID, GRID_BUBBLE_BITMAP_FID);
    }
    if (size == 360 * 1024 || size == 720 * 1024) {
        return tryOpenAs(data, size,
            GRID_FLOPPY_SECTOR_SIZE, GRID_FLOPPY_SUPERBLOCK_FID, GRID_FLOPPY_BITMAP_FID);
    }

    if (size == 10 * 1024 * 1024 || size == 20 * 1024 * 1024) {
        return tryOpenAs(data, size,
            GRID_HDD_SECTOR_SIZE, GRID_HDD_SUPERBLOCK_FID, GRID_HDD_BITMAP_FID);
    }

    return std::nullopt;
}

void MainWindow::openValidNonMbrDisk(QString path, ccos_disk_t disk) {
    ccos_inode_t* root = ccos_get_root_dir(&disk);
    Q_ASSERT(root != nullptr);

    panels[active_panel].emplace();

    auto& panel = *panels[active_panel];
    panel.path = path;
    panel.disk = disk;
    panel.current_dir = root;

    fillTable(active_panel, root, false);
    HDDMenu(false);
}

static bool isMbrDisk(const uint8_t* data, size_t size) {
    return size > 0x200 && data[0x1FE] == 0x55 && data[0x1FF] == 0xAA;
}

static std::optional<ccos_disk_t> tryOpenMbrPartition(uint8_t* data, mbr_part_t& partition) {
    std::optional<ccos_disk_t> disk = tryFromBootsector(data + partition.offset, partition.size);
    if (disk) {
        return disk;
    }

    return tryOpenAs(data + partition.offset, partition.size,
        GRID_HDD_SECTOR_SIZE, GRID_HDD_SUPERBLOCK_FID, GRID_HDD_BITMAP_FID);
}

void MainWindow::tryToOpenValidMbrDisk(QString path, uint8_t* data, size_t size) {
    Q_ASSERT(isMbrDisk(data, size));

    std::vector<uint8_t> hdddata(data, data+size);
    free(data);

    std::array<mbr_part_t, 4> parts;

    int grids = parseMbr(hdddata.data(), parts);
    if (grids == 0) {
        QMessageBox::critical(this, "MBR: No GRiD partitions",
                                        "No GRiD partitions found on the disk!");
        return;
    }

    ChsDlg dlg(this);
    dlg.setName("MBR: Select disk partition");
    dlg.setInfo("Hard disk with MBR detected.\nSelect the GRiD disk partition you want to work with:");

    for (int i = 0; i < 4; i++){
        if (parts[i].isgrid && parts[i].active){
            dlg.addItem(QString("Partition %1, active").arg(i+1));
        }
        else if (parts[i].isgrid){
            dlg.addItem(QString("Partition %1").arg(i+1));
        }
    }

    while (true) {
        if (dlg.exec() != 1) {
            break;
        }

        int selected = dlg.getIndex();

        std::optional<ccos_disk_t> disk = tryOpenMbrPartition(hdddata.data(), parts[selected]);
        if (disk) {
            openValidMbrPartition(path, std::move(hdddata), selected, *disk);
            break;
        }

        QMessageBox::warning(this, "MBR: Bad GRiD partition",
                                        "Failed to open selected partition");
    }
}

void MainWindow::openValidMbrPartition(QString path, std::vector<uint8_t> hdddata, int partition_index, ccos_disk_t disk) {
    ccos_inode_t* root = ccos_get_root_dir(&disk);
    Q_ASSERT(root != nullptr);

    panels[active_panel].emplace();

    auto& panel = *panels[active_panel];
    panel.path = path;
    panel.disk = disk;
    panel.current_dir = root;
    panel.hdd_mode = true;
    panel.hdd_data = std::make_shared<std::vector<uint8_t>>(std::move(hdddata));
    panel.hdd_partition = partition_index;

    fillTable(active_panel, root, false);
    HDDMenu(true);
}

void MainWindow::loadCustomImg(QString path, uint8_t* data, size_t size) {
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Failed to automatically detect image type!\n"
                    "Image may be broken, have non-GRiD format or custom parameters.\n"
                    "Do you want to set these parameters manually or cancel operation?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    if (msgBox.exec() != QMessageBox::Yes) {
        free(data);
        return;
    }

    CustDlg cdlg(this, true);
    while (true) {
        if (cdlg.exec() != 1) {
            free(data);
            break;
        }

        uint16_t sector_size, superblock;
        cdlg.GetParams(&sector_size, &superblock, nullptr, nullptr);

        // TODO: Allow to select bitmask.
        std::optional<ccos_disk_t> disk = tryOpenAs(data, size, sector_size, superblock, superblock-1);
        if (disk) {
            openValidNonMbrDisk(path, *disk);
        }

        QMessageBox::critical(this, "Failed to open",
                                              "Failed to open this file with specified parameters!");
    }
}

void MainWindow::LoadImg(QString path) {
    if (path.isEmpty()) {
        return;
    }

    if (isFileAlreadyOpened(path)) {
        handleAlreadyOpenedImg(path);
        return;
    }

    if (!CloseImg()) {
        return;
    }

    uint8_t* data;
    size_t size;
    if (readFileQt(path, &data, &size, this)) {
        return;
    }

    std::optional<ccos_disk_t> disk = tryFromBootsector(data, size);
    if (disk) {
        openValidNonMbrDisk(path, *disk);
        return;
    }

    disk = tryDetectBySize(data, size);
    if (disk) {
        openValidNonMbrDisk(path, *disk);
        return;
    }

    if (isMbrDisk(data, size)) {
        tryToOpenValidMbrDisk(path, data, size);
        return;
    }

    loadCustomImg(path, data, size);
}

void MainWindow::MakeDir(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        if (panel.in_subdir){
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
                size_t frsp = ccos_calc_free_space(&panel.disk);
                if (frsp == -1) {
                    QMessageBox::critical(this, "Calculation error",
                                    "Program can't calculate free space in the image!");
                    break;
                }
                if (frsp < 1024) {
                    QMessageBox::critical(this, "Not enough space",
                                    QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frsp));
                    break;
                }
                ccos_inode_t* root = ccos_get_root_dir(&panel.disk);
                if (ccos_create_dir(&panel.disk, root, name.toStdString().c_str()) == nullptr){
                    QMessageBox::critical(this, "Failed to create folder",
                                    "Program can't create a folder in the image!");
                    break;
                }
                panel.modified = true;
                fillTable(active_panel, root, panel.in_subdir);
                break;
            }
        }
    }
}

void MainWindow::NewImage(){
    if (panels[active_panel])
        if (!CloseImg()) return;

    CustDlg dlg(this);

    while (true){
        if (dlg.exec() == 1) {
            uint16_t sect, subl, isize;
            QString labl;
            dlg.GetParams(&sect, &subl, &isize, &labl);

            if (labl != "" && validString(labl, false, this) == -1) {
                continue;
            }

            panels[active_panel].emplace();
            auto& panel = *panels[active_panel];

            disk_format_t format = sect == 256 ? CCOS_DISK_FORMAT_BUBMEM : CCOS_DISK_FORMAT_COMPASS;

            if (ccos_new_disk_image(format, isize * 1024, &panel.disk) != 0) {
                QMessageBox::critical(this, "Creation error",
                                "Program can't create new image!");
                panels[active_panel].reset();
                return;
            }

            if (labl != "") {
                ccos_set_image_label(&panel.disk, labl.toStdString().c_str());
            }

            panel.modified = true;
            ccos_inode_t* root = ccos_get_root_dir(&panel.disk);
            panel.current_dir = root;
            fillTable(active_panel, root, false);
            break;
        }
        else{
            break;
        }
    }
}

void MainWindow::OpenDir(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QTableWidget* tw;
        if (active_panel == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QTableWidgetItem* called = tw->currentItem();
        ccos_inode_t *dir = panel.inodes[called->row()];
        if (dir == nullptr && !panel.in_subdir)
            return;
        if (called->row() == 0 && panel.in_subdir){
            ccos_inode_t* root = ccos_get_root_dir(&panel.disk);
            panel.current_dir = ccos_get_parent_dir(&panel.disk, panel.current_dir);
            if (panel.current_dir == root)
                panel.in_subdir = false;
            fillTable(active_panel, panel.current_dir, panel.in_subdir);
        }
        else if (!panel.in_subdir){ //All files in the root are directories
            panel.current_dir = dir;
            panel.in_subdir = true;
            fillTable(active_panel, dir, panel.in_subdir);
        }
    }
}

void MainWindow::OpenImg(){
    QString path = QFileDialog::getOpenFileName(this, "Open Image", "",
                                                 "GRiD image files (*.img);;"
                                                 "All files (*)");
    LoadImg(path);
}

void  MainWindow::Rename(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QTableWidget* tw;
        if (active_panel == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        QList<QTableWidgetItem *> called = tw->selectedItems();
        if (called.empty())
            return;
        if (called.size() == 7 && panel.inodes[called[0]->row()] == nullptr)
            return;

        for (int i = 0; i < called.size(); i+=7){
            ccos_inode_t* reninode = panel.inodes[called[i]->row()];
            if (reninode == nullptr){
                continue;
            }

            char basename[CCOS_MAX_FILE_NAME];
            char type[CCOS_MAX_FILE_NAME];
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            memset(type, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(reninode, basename, type, nullptr, nullptr);
            RenDlg dlg(this, !panel.in_subdir); //All files in the root are directories
            dlg.setName(basename);
            dlg.setType(type);
            dlg.setInfo((QString("Set new name and type for %1:").arg(reninode->desc.name)));
            while (true){
                if (dlg.exec() == 1){
                    QString newname = dlg.getName();
                    QString newtype = dlg.getType();
                    if (newtype.contains("subject", Qt::CaseInsensitive) && panel.in_subdir){
                        QMessageBox::critical(this, "Incorrect Type",
                                              "Can't set directory type for file!");
                    }
                    else if (newname == "" || newtype == ""){
                        QMessageBox::critical(this, "Incorrect Name or Type",
                                              "File name or type can't be empty!");
                    }
                    else if (validString(newname, true, this) != -1 && validString(newtype, true, this) != -1){
                        ccos_rename_file(&panel.disk, reninode, newname.toStdString().c_str(),
                                         newtype.toStdString().c_str());
                        panel.modified = true;
                        fillTable(active_panel, ccos_get_parent_dir(&panel.disk, reninode),
                                  panel.in_subdir);
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
    if (!panels[active_panel] || panels[active_panel]->path == "")
        return SaveAs();

    auto& panel = *panels[active_panel];
    QGroupBox* gb;
    if (panel.modified){
        if (active_panel == 0)
            gb = ui->groupBox;
        else
            gb = ui->groupBox_2;

        int res;
        if (panel.hdd_mode){
            res = saveFileQt(panel.path, panel.hdd_data->data(), panel.hdd_data->size(), this);
        }
        else{
            res = saveFileQt(panel.path, panel.disk.data, panel.disk.size, this);
        }

        if (res == -1){
            QMessageBox::critical(this, "Unable to save file",
                            QString("Unable to save file \"%1\". Please check the path.").arg(panel.path));
            return;
        }
        panel.modified = false;
        int other = !active_panel;
        if (panel.hdd_data && panel.hdd_data.use_count() > 1 && panels[other])
            panels[other]->modified = false;
        gb->setTitle(gb->title().left(gb->title().size()-1));
    }
}

void MainWindow::SaveAs(){
    if (!panels[active_panel])
        return;

    auto& panel = *panels[active_panel];
    QGroupBox* gb;
    if (active_panel == 0)
        gb = ui->groupBox;
    else
        gb = ui->groupBox_2;
    QString nameQ = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
    if (nameQ == "")
        return;

    int res;
    if (panel.hdd_mode){
        res = saveFileQt(nameQ, panel.hdd_data->data(), panel.hdd_data->size(), this);
    }
    else{
        res = saveFileQt(nameQ, panel.disk.data, panel.disk.size, this);
    }

    if (res == -1){
        QMessageBox::critical(this, "Unable to save file",
                              QString("Unable to save file \"%1\". Please check the path.").arg(nameQ));
        return;
    }
    panel.path = nameQ;
    if (panel.modified){
        gb->setTitle(gb->title().left(gb->title().size()-1));
        panel.modified = false;
        int other = !active_panel;
        if (panel.hdd_data && panel.hdd_data.use_count() > 1 && panels[other])
            panels[other]->modified = false;
    }
}

void MainWindow::SavePart(){
    auto& panel = *panels[active_panel];
    panel.hdd_mode = false;
    bool oldch = panel.modified;
    panel.modified = true;
    QGroupBox* gb;
    if (active_panel == 0)
        gb = ui->groupBox;
    else
        gb = ui->groupBox_2;
    gb->setTitle(gb->title()+' ');
    SaveAs();

    if (!panel.modified){
        uint8_t* imdat = (uint8_t*)calloc(panel.disk.size, sizeof(uint8_t));
        memcpy(imdat, panel.disk.data, panel.disk.size);
        panel.disk.data = imdat;
        panel.hdd_data.reset();
    }
    else{
        panel.hdd_mode = true;
        panel.modified = oldch;
        gb->setTitle(gb->title().left(gb->title().size()-1));
    }
}

void MainWindow::SetActivePart(){
    auto& panel = *panels[active_panel];
    uint8_t* mbrtab = panel.hdd_data->data() + 0x1BE;

    std::array<mbr_part_t, 4> parts;
    int grids = parseMbr(panel.hdd_data->data(), parts);

    if (grids == 0){
        QMessageBox::critical(this, "No GRiD partitions",
                              "No GRiD partitions found on the disk!");
        return;
    }

    ChsDlg dlg(this);
    dlg.setName("Select disk partition");
    dlg.setInfo("Select the GRiD disk partition to make it active:");

    dlg.addItem("No active partition");

    for (int i = 0; i < 4; i++){
        if (parts[i].isgrid)
            dlg.addItem(QString("Partition %1").arg(i+1));
    }

    if (dlg.exec() == 1){
        int selctd = dlg.getIndex();

        mbrtab[0] = 0x0;
        mbrtab[16] = 0x0;
        mbrtab[32] = 0x0;
        mbrtab[48] = 0x0;
        if (selctd > 0)
            mbrtab[(selctd-1)*16] = 0x80;

        panel.modified = true;
        fillTable(active_panel, panel.current_dir, panel.in_subdir);
    }
}

void MainWindow::Version(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QTableWidget* tw;
        if (active_panel == 0)
            tw = ui->tableWidget;
        else
            tw = ui->tableWidget_2;
        ccos_inode_t* file = panel.inodes[tw->currentItem()->row()];
        if (file != nullptr){
            version_t ver = ccos_get_file_version(file);
            VerDlg dlg(this);
            dlg.init(file->desc.name, ver);
            if (dlg.exec() == 1){
                ver = dlg.retVer();
                ccos_set_file_version(&panel.disk, file, ver);
                panel.modified = true;
                fillTable(active_panel, panel.current_dir, panel.in_subdir);
            }
        }
    }
}

MainWindow::~MainWindow() = default;
