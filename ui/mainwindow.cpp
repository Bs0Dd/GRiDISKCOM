#include "mainwindow.h"
#include "preview/dialog.h"
#include "diskconstants.h"
#include "mbr.h"
#include "imd2raw.h"

#include <QDir>
#include <QRegularExpression>
#include <QTemporaryFile>

#include <cstdio>
#include <ctime>


//[Service functions]

static ccos_disk_t* tryOpenMbrPartition(uint8_t* data, MbrPartition& partition);
static QString mbrPartitionLabel(uint8_t* hdd_data, const MbrPartition& part);

namespace {
struct SearchQuery {
    QStringList directoryPatterns;
    QString filePattern;
};

SearchQuery parseSearchQuery(const QString& query) {
    const QStringList parts = query.split('`', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return {};

    SearchQuery result;
    result.filePattern = parts.constLast();
    result.directoryPatterns = parts.mid(0, parts.size() - 1);
    return result;
}

bool wildcardMatches(const QString& value, const QString& pattern) {
    if (!pattern.contains('*'))
        return value.contains(pattern, Qt::CaseInsensitive);

    QString expression = "^";
    for (const QChar character : pattern) {
        expression += character == '*' ? ".*" : QRegularExpression::escape(QString(character));
    }
    expression += '$';
    return QRegularExpression(expression, QRegularExpression::CaseInsensitiveOption).match(value).hasMatch();
}

QString inodeBaseName(const ccos_inode_t* inode) {
    char name[CCOS_MAX_FILE_NAME] = {};
    if (ccos_parse_file_name(inode, name, nullptr, nullptr, nullptr) != CCOS_OK)
        return {};
    return QString::fromLatin1(name);
}

QString inodeType(const ccos_inode_t* inode) {
    char type[CCOS_MAX_FILE_NAME] = {};
    if (ccos_parse_file_name(inode, nullptr, type, nullptr, nullptr) != CCOS_OK)
        return {};
    return QString::fromLatin1(type);
}
}  // namespace

ccos_date_t ccos_get_datetime(void) {
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  tm* time_struct = localtime(&tp.tv_sec);

  return (ccos_date_t){
    .year = static_cast<uint16_t>(time_struct->tm_year + 1900),
    .month = static_cast<uint8_t>(time_struct->tm_mon + 1),
    .day = static_cast<uint8_t>(time_struct->tm_mday),
    .hour = static_cast<uint8_t>(time_struct->tm_hour),
    .minute = static_cast<uint8_t>(time_struct->tm_min),
    .second = static_cast<uint8_t>(time_struct->tm_sec),
    .tenthOfSec = static_cast<uint8_t>(tp.tv_nsec / 100000000),
    .dayOfWeek = static_cast<uint8_t>(time_struct->tm_wday + 1),
    .dayOfYear = static_cast<uint16_t>(time_struct->tm_yday + 1),
  };
}

void replace_char_in_place(char* src, char from, char to) {
  for (int i = 0; i < strlen(src); ++i) {
    if (src[i] == from) {
      src[i] = to;
    }
  }
}

QString short_string_to_qstring(const short_string_t* short_string) {
  if (short_string == nullptr) {
    return {};
  }
  return QString::fromLatin1(short_string->data, short_string->length);
}

//*Get file version and convert to QString ("A.B.C")
QString ccosGetFileVersionQstr(ccos_inode_t* file){
    ccos_version_t ver = ccos_get_file_version(file);
    return QString("%1.%2.%3").arg(QString::number(ver.major), QString::number(ver.minor),
                                   QString::number(ver.patch));
}

//*Convert ccos_date_t to QDate
static QDate ccosDateToQDate(ccos_date_t date){
    return QDate(date.year, date.month, date.day);
}

//*Format a byte count as a human-readable "X MB (Y bytes)" string.
static QString formatFreeSpace(size_t bytes) {
    const double KB = 1024.0;
    const double MB = 1024.0 * 1024.0;
    const double GB = 1024.0 * 1024.0 * 1024.0;
    QString human;
    if (bytes >= GB)
        human = QString::number(bytes / GB, 'f', 2) + " GB";
    else if (bytes >= MB)
        human = QString::number(bytes / MB, 'f', 2) + " MB";
    else if (bytes >= KB)
        human = QString::number(bytes / KB, 'f', 1) + " KB";
    else
        human = QString::number(bytes) + " B";
    return QString("Free space: %1 (%2 bytes)").arg(human, QString::number(bytes));
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
                const QVector<int>& fileIndices,
                size_t* needs){ //*For copy

    size_t frsp = 0;
    if (ccos_calc_free_space(to.disk, &frsp) != CCOS_OK){
        return -2;
    }
    *needs = 0;
    for (int idx : fileIndices){
        ccos_inode_t* file = from.inodes[idx];
        if (file != nullptr){
            if (ccos_is_dir(file)){
                uint16_t fils = 0;
                ccos_inode_t** dirdata = nullptr;
                ccos_get_dir_contents(from.disk, file, &fils, &dirdata);

                for(int j = 0; j < fils; j++){
                    *needs += dirdata[j]->desc.file_size;
                }
                free(dirdata);
            }

            *needs += file->desc.file_size;
        }
    }

    if (*needs > frsp)
        return -1;
    else
        return 0;
}

int checkFreeSp(ccos_disk_t* disk, QStringList files, size_t* needs){ //*For add
    size_t frsp = 0;
    if (ccos_calc_free_space(disk, &frsp) != CCOS_OK){
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

//*Convert an .IMD file into a temporary raw .img and return its path.
// Returns an empty string on failure (an error dialog is shown to the user).
QString convertImdToTempImg(const QString& imdPath, QWidget* parent){
    QTemporaryFile tempFile(QDir::tempPath() + "/gridiskcom_XXXXXX.img");
    tempFile.setAutoRemove(false);
    if (!tempFile.open()){
        QMessageBox::critical(parent, "Unable to create temporary file",
                        QString("Unable to create a temporary file for IMD conversion in \"%1\".")
                            .arg(QDir::tempPath()));
        return "";
    }
    QString tempPath = tempFile.fileName();
    tempFile.close(); // release so imd2raw can open it for writing

    QByteArray inBytes = imdPath.toLocal8Bit();
    QByteArray outBytes = tempPath.toLocal8Bit();
    int res = imd2raw_convert(inBytes.constData(), outBytes.constData());
    if (res != 0){
        QFile::remove(tempPath);
        QMessageBox::critical(parent, "IMD conversion failed",
                        QString("Failed to convert IMD file \"%1\" to a raw image (code %2).\n"
                                "The file may be corrupted or not a valid ImageDisk image.")
                            .arg(imdPath).arg(res));
        return "";
    }
    return tempPath;
}

//*Dump file from image to path
int dumpFileQt(ccos_disk_t* disk, ccos_inode_t* file, QString path, QWidget* parent){
    QString fnam = short_string_to_qstring(ccos_get_file_name(file));
    fnam.replace('/', '_');

    QString fpath = QDir(path).filePath(fnam);

    size_t file_size = 0;
    uint8_t* file_data = nullptr;

    if (ccos_read_file(disk, file, &file_data, &file_size) != CCOS_OK){
        QMessageBox::critical(parent, "Failed to read file from image",
                      QString("Unable to read file \"%1\": Unable to get file contents!").arg(fnam));
        return -1;
    }

    if (saveFileQt(fpath, file_data, file_size, parent) == -1){
        free(file_data);
        return -1;
    }

    free(file_data);
    return 0;
}

//*Dump dir from image to path
int dumpDirQt(ccos_disk_t* disk, ccos_inode_t* dir, QString path, QWidget* parent){
    char name[CCOS_MAX_FILE_NAME] = {};
    if (dir->header.file_id == dir->desc.dir_file_id) {
        // A root directory is named by the disk label, without a file type.
        const short_string_t* label = ccos_get_file_name(dir);
        memcpy(name, label->data, label->length);
    } else {
        ccos_parse_file_name(dir, name, nullptr, nullptr, nullptr);
    }
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

    if (ccos_get_file_name(root_dir)->length == 0) {
        altname.replace('/', '_');
        return dumpDirQt(disk, root_dir, QDir(path).filePath(altname), parent);
    }

    return dumpDirQt(disk, root_dir, path, parent);
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

// --- Panel helpers ----------------------------------------------------------

FilePanelWidget* MainWindow::panelWidget(int panel_idx) {
    return (panel_idx == 0) ? ui->panel1 : ui->panel2;
}

// Builds the UI-facing entry list for a directory and keeps panel.inodes in
// sync (real files only, no nullptr sentinels).
QVector<PanelFileEntry> MainWindow::buildFileEntries(int panel_idx, ccos_inode_t* directory) {
    auto& panel = *panels[panel_idx];
    panel.inodes.clear();

    uint16_t fils = 0;
    ccos_inode_t** dirdata = nullptr;
    ccos_get_dir_contents(panel.disk, directory, &fils, &dirdata);

    QVector<PanelFileEntry> entries;
    entries.reserve(fils);

    for (int c = 0; c < fils; c++) {
        const QString name = inodeBaseName(dirdata[c]);
        const QString type = inodeType(dirdata[c]);

        PanelFileEntry entry;
        entry.name = name;
        entry.type = type;
        entry.size = dirdata[c]->desc.file_size;
        entry.version = ccosGetFileVersionQstr(dirdata[c]);
        entry.creationDate = ccosDateToQDate(dirdata[c]->desc.creation_date);
        entry.modificationDate = ccosDateToQDate(dirdata[c]->desc.mod_date);
        entry.expirationDate = ccosDateToQDate(dirdata[c]->desc.expiration_date);

        panel.inodes.push_back(dirdata[c]);
        entries.append(std::move(entry));
    }

    free(dirdata);
    return entries;
}

QVector<PanelFileEntry> MainWindow::buildSearchResults(int panel_idx, const QString& query) {
    auto& panel = *panels[panel_idx];
    panel.inodes.clear();

    const SearchQuery search = parseSearchQuery(query);
    if (search.filePattern.isEmpty())
        return {};

    struct SearchDirectory {
        ccos_inode_t* inode;
        QString path;
    };

    ccos_inode_t* root = ccos_get_root_dir(panel.disk);
    if (root == nullptr)
        return {};

    QStringList currentPathParts;
    ccos_inode_t* current = panel.current_dir;
    while (current != nullptr && current->header.file_id != current->desc.dir_file_id) {
        currentPathParts.prepend(inodeBaseName(current));
        current = ccos_get_parent_dir(panel.disk, current);
    }
    const QString currentPath = currentPathParts.join('`');
    const bool startsFromRoot = !search.directoryPatterns.isEmpty() &&
                                search.directoryPatterns.constFirst() == "*";
    QVector<SearchDirectory> directories {{startsFromRoot ? root : panel.current_dir,
                                            startsFromRoot ? QString() : currentPath}};
    auto appendSubdirectories = [&panel](const SearchDirectory& directory,
                                         QVector<SearchDirectory>* children) {
        uint16_t count = 0;
        ccos_inode_t** entries = nullptr;
        if (ccos_get_dir_contents(panel.disk, directory.inode, &count, &entries) != CCOS_OK)
            return;

        for (int index = 0; index < count; ++index) {
            ccos_inode_t* entry = entries[index];
            if (!ccos_is_dir(entry))
                continue;
            const QString name = inodeBaseName(entry);
            if (!name.isEmpty()) {
                children->append({entry, directory.path.isEmpty()
                    ? name : directory.path + '`' + name});
            }
        }
        free(entries);
    };

    for (const QString& pattern : search.directoryPatterns) {
        QVector<SearchDirectory> next;
        if (pattern == "*") {
            // A wildcard path component searches every descendant directory,
            // with the root included so root-level files are considered too.
            next = directories;
            for (int index = 0; index < next.size(); ++index) {
                const SearchDirectory directory = next[index];
                appendSubdirectories(directory, &next);
            }
        } else {
            for (const SearchDirectory& directory : directories) {
                QVector<SearchDirectory> children;
                appendSubdirectories(directory, &children);
                for (const SearchDirectory& child : children) {
                    const QString name = child.path.section('`', -1);
                    if (wildcardMatches(name, pattern))
                        next.append(child);
                }
            }
        }
        directories = std::move(next);
        if (directories.isEmpty())
            return {};
    }

    QVector<PanelFileEntry> results;
    for (const SearchDirectory& directory : directories) {
        uint16_t count = 0;
        ccos_inode_t** entries = nullptr;
        if (ccos_get_dir_contents(panel.disk, directory.inode, &count, &entries) != CCOS_OK)
            continue;

        for (int index = 0; index < count; ++index) {
            ccos_inode_t* entry = entries[index];
            const QString name = inodeBaseName(entry);
            const QString type = inodeType(entry);
            const QString fullName = type.isEmpty() ? name : name + '~' + type + '~';
            if (!wildcardMatches(fullName, search.filePattern))
                continue;

            PanelFileEntry result;
            result.name = directory.path.isEmpty() ? name : directory.path + '`' + name;
            result.type = type;
            result.size = entry->desc.file_size;
            result.version = ccosGetFileVersionQstr(entry);
            result.creationDate = ccosDateToQDate(entry->desc.creation_date);
            result.modificationDate = ccosDateToQDate(entry->desc.mod_date);
            result.expirationDate = ccosDateToQDate(entry->desc.expiration_date);
            panel.inodes.push_back(entry);
            results.append(std::move(result));
        }
        free(entries);
    }

    return results;
}

void MainWindow::showSearchResults(int panel_idx) {
    auto& panel = *panels[panel_idx];
    panelWidget(panel_idx)->setFiles(buildSearchResults(panel_idx, panel.search_query), false);
}

//*Get directory listing and push it to the panel widget
void MainWindow::fillTable(int panel_idx, ccos_inode_t* directory, bool noRoot) {
    auto& panel = *panels[panel_idx];
    FilePanelWidget* pw = panelWidget(panel_idx);

    pw->setDiskPresent(true);
    pw->setHddMode(panel.hdd_mode);
    if (panel.search_query.isEmpty()) {
        auto entries = buildFileEntries(panel_idx, directory);
        pw->setFiles(entries, panel.in_subdir);
    } else {
        showSearchResults(panel_idx);
    }

    updatePanelTitle(panel_idx);

    size_t free_space = 0;
    if (ccos_calc_free_space(panel.disk, &free_space) != CCOS_OK) {
        pw->setStatusText("Free space: FAILED TO CALCULATE!");
    } else {
        pw->setStatusText(formatFreeSpace(free_space));
    }

    // Re-establish whatever view state we remembered for this directory
    // (selection + scroll). For a brand-new directory there is none, and the
    // widget falls back to the top row.
    applyViewState(panel_idx, directory);

    if (panel_idx == active_panel)
        updateActionStates();
}

void MainWindow::saveCurrentViewState(int panel_idx) {
    if (!panels[panel_idx] || panels[panel_idx]->current_dir == nullptr)
        return;
    auto& panel = *panels[panel_idx];
    FilePanelWidget* pw = panelWidget(panel_idx);
    DirViewState st;
    st.scroll = pw->verticalScrollValue();
    st.selectIndex = pw->currentFileIndex();
    panel.view_state.insert(reinterpret_cast<quintptr>(panel.current_dir), st);
}

void MainWindow::applyViewState(int panel_idx, ccos_inode_t* directory) {
    if (!panels[panel_idx] || directory == nullptr)
        return;
    auto& panel = *panels[panel_idx];
    const quintptr key = reinterpret_cast<quintptr>(directory);
    auto it = panel.view_state.constFind(key);
    if (it != panel.view_state.constEnd())
        panelWidget(panel_idx)->restoreSelection(it->selectIndex, it->scroll);
    else
        panelWidget(panel_idx)->restoreSelection(-1, -1);
}

void MainWindow::refreshPanel(int panel_idx) {
    if (!panels[panel_idx])
        return;
    saveCurrentViewState(panel_idx);
    auto& panel = *panels[panel_idx];
    if (panel.search_query.isEmpty())
        fillTable(panel_idx, panel.current_dir, panel.in_subdir);
    else
        showSearchResults(panel_idx);
}

void MainWindow::updatePanelTitle(int panel_idx) {
    if (!panels[panel_idx] || panels[panel_idx]->disk == nullptr)
        return;
    auto& panel = *panels[panel_idx];
    QString disk_name = (panel_idx == 0) ? "I" : "II";
    QString labd = short_string_to_qstring(ccos_get_disk_label(panel.disk));
    panelWidget(panel_idx)->setTitle(
        QString("Disk %1 - %2%3").arg(disk_name,
                                       !labd.isEmpty() ? labd : "No label",
                                       panel.modified ? "*" : ""));
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

    QMainWindow::setWindowTitle(QString("GRiDISK Commander v")+_PVER_);

    // Each panel widget manages its own table, empty placeholder, context
    // menu and keyboard handling; we just wire up the callbacks here.
    FilePanelWidget* pws[2] = {ui->panel1, ui->panel2};
    for (int i = 0; i < 2; ++i) {
        FilePanelWidget* pw = pws[i];
        pw->setFiles({}, false);
        connect(pw, &FilePanelWidget::panelActivated, this, [this, i]() { onPanelActivated(i); });
        connect(pw, &FilePanelWidget::fileDoubleClicked, this,
                [this, i](int idx) { onFileDoubleClicked(i, idx); });
        connect(pw, &FilePanelWidget::fileRightClicked, this,
                [this, i](int idx, const QPoint& pos) { onFileRightClicked(i, idx, pos); });
        connect(pw, &FilePanelWidget::goUpToParent, this, [this, i]() { onGoUpToParent(i); });
        connect(pw, &FilePanelWidget::urlsDropped, this,
                [this, i](const QStringList& files) { onUrlsDropped(i, files, {}); });
        connect(pw, &FilePanelWidget::openRequested, this, [this, i]() { onOpenRequested(i); });
        connect(pw, &FilePanelWidget::partitionSwitchRequested, this, &MainWindow::AnPartMenu);
        connect(pw, &FilePanelWidget::searchRequested, this,
                [this, i](const QString& query) { onSearchRequested(i, query); });
    }
    refreshActivePanelUI();

    QStringList argv = QCoreApplication::arguments();
    int argc = argv.size();

    if (argc > 1){
        for (int i = 1; i < argc; i++){
            QString arg = argv[i];
            if (arg == "--debug") {
                if (!ui->actionDebtrace->isChecked()){
                    ui->actionDebtrace->setChecked(true);
                    DebTrace();
                }
            }
            else if (!panels[0] || !panels[1]){
                QFileInfo fil(arg);
                if ((fil.suffix().toLower() == "img" || fil.suffix().toLower() == "imd") && fil.exists()){
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
    connect(ui->actionPreview, SIGNAL(triggered()), this, SLOT(ShowPreview()));
    connect(ui->actionRename, SIGNAL(triggered()), this, SLOT(Rename()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(Save()));
    connect(ui->actionSave_as, SIGNAL(triggered()), this, SLOT(SaveAs()));
    connect(ui->actionSearch, SIGNAL(triggered()), this, SLOT(Search()));
    connect(ui->actionSep_save, SIGNAL(triggered()), this, SLOT(SavePart()));
}

void MainWindow::AboutShow(){
    AboutDlg dlg(this);
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
    ccos_inode_t* root = ccos_get_root_dir(panel.disk);
    for (const auto& dir : dirs) {
        size_t frees = 0;
        if (ccos_calc_free_space(panel.disk, &frees) != CCOS_OK){
            QMessageBox::critical(this, "Calculation error",
                            "Program can't calculate free space in the image!");
            break;
        }

        if (frees < 1024) {
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frees));
            break;
        }
        ccos_inode_t* newdir = ccos_create_dir(panel.disk, root, QFileInfo(dir).fileName().toStdString().c_str());
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
        refreshPanel(active_panel);
    }
}

int MainWindow::AddFiles(QStringList files, ccos_inode_t* copyTo){
    auto& panel = *panels[active_panel];
    size_t needs = 0;
    int retop = checkFreeSp(panel.disk, files, &needs);
    if (retop == -2) {
        QMessageBox::critical(this, "Calculation error",
                        "Program can't calculate free space in the image!");
        return -1;
    }
    else if (retop == -1) {
        size_t free_space = 0;
        ccos_calc_free_space(panel.disk, &free_space);
        QMessageBox::critical(this, "Not enough space",
                        QString("Requires %1 bytes of additional disk space to add!").arg(needs-free_space));
        return -1;
    }
    for (const auto& file : files){
        uint8_t* fdat = nullptr;
        size_t fsiz = 0;
        std::string fname = QFileInfo(file).fileName().toStdString();
        if (tildaCheck(fname) == -1){
            RenameDlg dlg(this);
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
            if (ccos_add_file(panel.disk, copyTo, fdat, fsiz, fname.c_str()) == nullptr){
                QMessageBox::critical(this, "Error",
                                QString("Can't add \"%1\" to the image! Skipping...").arg(fname.c_str()));
            }
        }
    }
    if (files.size() != 0){
        panel.modified = true;
        refreshPanel(active_panel);
    }
    return 0;
}

void MainWindow::AnPartMenu(){
    AnotherPart(true);
}

void MainWindow::AnotherPart(bool fromMenu){
    const int panel_at_entry = active_panel;
    const int usedisk = fromMenu ? panel_at_entry : !panel_at_entry;
    openAnotherPartition(panel_at_entry, usedisk);
}

void MainWindow::openAnotherPartition(int targetPanel, int sourcePanel) {
    if (!panels[sourcePanel] || !panels[sourcePanel]->hdd_mode ||
        !panels[sourcePanel]->hdd_data) {
        return;
    }

    const int panel_at_entry = targetPanel;
    const int usedisk = sourcePanel;
    auto& src = *panels[usedisk];

    std::vector<MbrPartition> parts = parseMbr(src.hdd_data->data(), src.hdd_data->size());

    QStringList labels;
    for (const auto& p : parts)
        labels << mbrPartitionLabel(src.hdd_data->data(), p);

    QSet<int> alreadyOpen;
    for (int i = 0; i < 2; ++i) {
        if (panels[i] && panels[i]->hdd_mode && panels[i]->path == src.path &&
            panels[i]->hdd_partition.has_value())
            alreadyOpen.insert(panels[i]->hdd_partition.value());
    }

    PartitionDlg dlg(this);
    dlg.setTitle("Select disk partition");
    dlg.setInfo("Select the GRiD disk partition you want to work with:");
    dlg.setPartitions(parts, labels, alreadyOpen);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    int selctd = dlg.selectedIndex();

    const int topan = panel_at_entry;

    if (usedisk != topan && panels[topan]) {
        int saved_active = active_panel;
        active_panel = topan;
        int closed = CloseImg();
        active_panel = saved_active;
        if (!closed) {
            refreshActivePanelUI();
            return;
        }
    }

    bool fresh_target = !panels[topan];
    if (fresh_target)
        panels[topan].emplace();

    auto& dst = *panels[topan];

    ccos_disk_t* new_disk = tryOpenMbrPartition(src.hdd_data->data(), parts[selctd]);
    ccos_inode_t* root = (new_disk != nullptr) ? ccos_get_root_dir(new_disk) : nullptr;
    if (root == nullptr){
        QMessageBox::critical(this, "Incorrect Image File",
                                "Image broken or have non-GRiD format!");
        if (new_disk)
            free(new_disk);
        if (fresh_target)
            panels[topan].reset();
        if ((!panels[panel_at_entry] || panels[panel_at_entry]->disk == nullptr)
            && panels[usedisk] && panels[usedisk]->disk != nullptr) {
            active_panel = usedisk;
        }
        refreshActivePanelUI();
        return;
    }

    if (dst.disk != nullptr)
        free(dst.disk);
    dst.disk = new_disk;
    dst.hdd_mode = true;
    dst.hdd_partition = int(parts[selctd].index);
    dst.current_dir = root;
    dst.view_state.clear();  // stale keys belong to the previous disk
    if (usedisk != topan) {
        dst.path = src.path;
        dst.hdd_data = src.hdd_data;
    }

    fillTable(topan, root, false);
    active_panel = topan;
    refreshActivePanelUI();
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

    FilePanelWidget* pw = panelWidget(active_panel);
    pw->setDiskPresent(false);
    pw->setHddMode(false);
    pw->setFiles({}, false);
    pw->setTitle({});
    pw->setStatusText({});
    return 1;
}

void MainWindow::Copy(){
    int other = !active_panel;
    if (panels[active_panel] && panels[other]){
        auto& src = *panels[active_panel];
        auto& dst = *panels[other];
        QVector<int> selected = panelWidget(active_panel)->selectedFileIndices();
        if (selected.isEmpty())
            return;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to copy %1 file(s)?").arg(selected.size()));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
        size_t needs = 0;
        int retop = checkFreeSp(src, dst, selected, &needs);
        if (retop == -2) {
            QMessageBox::critical(this, "Calculation error",
                            "Program can't calculate free space in the image!");
            return;
        }
        else if (retop == -1) {
            size_t frsp = 0;
            ccos_calc_free_space(dst.disk, &frsp);
            QMessageBox::critical(this, "Not enough space",
                            QString("Requires %1 bytes of additional disk space to copy").arg(needs-frsp));
            return;
        }
        for (int idx : selected){
            ccos_inode_t* file = src.inodes[idx];
            if (ccos_is_dir(file)) {
                if (dst.current_dir->header.file_id != ccos_get_root_dir(dst.disk)->header.file_id) {
                    QMessageBox::critical(this, "Copying to non-root",
                                    "Folders can be copied only to root folder!");
                    return;
                }
                char newname[CCOS_MAX_FILE_NAME] = {};
                ccos_parse_file_name(file, newname, nullptr, nullptr, nullptr);
                ccos_inode_t* newdir = ccos_create_dir(dst.disk, ccos_get_root_dir(dst.disk), newname);
                if (newdir == nullptr){
                            QMessageBox::critical(this, "Failed to create folder",
                                            "Program can't create a folder in the image!");
                            return;
                }
                uint16_t fils = 0;
                ccos_inode_t** dirdata = nullptr;
                ccos_get_dir_contents(src.disk, file, &fils, &dirdata);
                for (int c = 0; c < fils; c++) {
                    ccos_copy_file(src.disk, dirdata[c], dst.disk, newdir);
                }
            }
            else {
                if (dst.current_dir->header.file_id == ccos_get_root_dir(dst.disk)->header.file_id) {
                    QMessageBox::critical(this, "Copying to root",
                                    "Files can be copied only to non-root folder!");
                    return;
                }
                ccos_copy_file(src.disk, file, dst.disk, dst.current_dir);
            }
        }
        dst.modified = true;
        refreshPanel(other);
        refreshPanel(active_panel);
    }
}

void MainWindow::CopyLoc() {
    if (!panels[active_panel]) {
        return;
    }

    auto& panel = *panels[active_panel];

    QVector<int> selected = panelWidget(active_panel)->selectedFileIndices();
    if (selected.isEmpty()) {
        return;
    }

    size_t needs = 0;
    int retop = checkFreeSp(panel, panel, selected, &needs);
    if (retop == -2) {
        QMessageBox::critical(this, "Calculation error",
                        "Program can't calculate free space in the image!");
        return;
    }
    else if (retop == -1) {
        size_t frsp = 0;
        ccos_calc_free_space(panel.disk, &frsp);
        QMessageBox::critical(this, "Not enough space",
                        QString("Requires %1 bytes of additional disk space to copy").arg(needs-frsp));
        return;
    }

    if (panel.in_subdir){
        ccos_inode_t* root = ccos_get_root_dir(panel.disk);

        uint16_t fils = 0;
        ccos_inode_t** dirdata = nullptr;
        ccos_get_dir_contents(panel.disk, root, &fils, &dirdata);

        QStringList items;
        for (int i = 0; i < fils; i++){
            char basename[CCOS_MAX_FILE_NAME];
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(dirdata[i], basename, nullptr, nullptr, nullptr);
            items << QString::fromLatin1(basename);
        }

        bool ok = false;
        QString chosen = QInputDialog::getItem(this, tr("Select the directory"),
                                               tr("Select the directory where the file(s) will be copied:"),
                                               items, 0, false, &ok);
        if (!ok || chosen.isEmpty())
            return;

        int chosenIdx = items.indexOf(chosen);
        ccos_inode_t* firfil = panel.inodes[selected[0]];

        if (dirdata[chosenIdx]->header.file_id == firfil->desc.dir_file_id){
            QMessageBox::critical(this, "Copy to parent dir",
                                    "Can't copy files to it's parent dir!");
            return;
        }

        for (int idx : selected){
            ccos_copy_file(panel.disk, panel.inodes[idx],
                    panel.disk, dirdata[chosenIdx]);
        }
        panel.modified = true;
        refreshPanel(active_panel);
    }
    else{
        for (int idx : selected){
            char basename[CCOS_MAX_FILE_NAME];
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(panel.inodes[idx], basename, nullptr, nullptr, nullptr);

            QString name;
            while (true){
                name = QInputDialog::getText(this, tr("Copy dir"),
                                                tr("Name for \"%1\" copy:").arg(basename), QLineEdit::Normal, name);
                if (name == "")
                    break;
                else if (validString(name, true, this) != -1){
                    ccos_inode_t* root = ccos_get_root_dir(panel.disk);

                    ccos_inode_t* newdir = ccos_create_dir(panel.disk, root, name.toStdString().c_str());
                    if (newdir == nullptr){
                        QMessageBox::critical(this, "Failed to create folder",
                                        "Program can't create a folder in the image!");
                        break;
                    }

                    uint16_t fils = 0;
                    ccos_inode_t** dirdata = nullptr;
                    ccos_get_dir_contents(panel.disk, panel.inodes[idx], &fils, &dirdata);

                    for(int i = 0; i < fils; i++){
                        ccos_copy_file(panel.disk, dirdata[i],
                                panel.disk, newdir);
                    }
                    panel.modified = true;
                    refreshPanel(active_panel);
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

    int idx = panelWidget(active_panel)->currentFileIndex();
    if (idx < 0)
        return;
    ccos_inode_t* file = panel.inodes[idx];

    ccos_date_t cre = file->desc.creation_date;
    ccos_date_t mod = file->desc.mod_date;
    ccos_date_t exp = file->desc.expiration_date;
    DateDlg dlg(this);
    dlg.init(file->desc.name, cre, mod, exp);
    if (dlg.exec()){
        dlg.retDates(&cre, &mod, &exp);
        ccos_set_creation_date(panel.disk, file, cre);
        ccos_set_mod_date(panel.disk, file, mod);
        ccos_set_exp_date(panel.disk, file, exp);
        panel.modified = true;
        refreshPanel(active_panel);
    }
}

void MainWindow::DebTrace(){
    if (ui->actionDebtrace->isChecked()){
        trace = fprintf;
        TRACE("ccos_image debug trace enabled");
    }
    else{
        TRACE("ccos_image debug trace disabled");
        trace = nullptr;
    }
}

void MainWindow::Delete(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QVector<int> selected = panelWidget(active_panel)->selectedFileIndices();
        if (selected.isEmpty())
            return;
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("Do you want to delete %1 file(s)?").arg(selected.size()));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes)
            return;
        for (int idx : selected){
            ccos_delete_file(panel.disk, panel.inodes[idx]);
        }
        panel.modified = true;
        refreshPanel(active_panel);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event){
   if (event->mimeData()->hasUrls())
       event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event){
    // Fallback for drops that land on the window background rather than on
    // a panel: route them to the active panel.
    QStringList files, dirs;
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        for (const auto& url : mimeData->urls()) {
            const QString path = url.toLocalFile();
            if (path.isEmpty())
                continue;
            if (QFileInfo(path).isDir())
                dirs.append(path);
            else
                files.append(path);
        }
    }
    if (!files.isEmpty() || !dirs.isEmpty())
        onUrlsDropped(active_panel, files, dirs);
    event->acceptProposedAction();
}

void MainWindow::onUrlsDropped(int panel_idx, const QStringList& FilesList, const QStringList& DirsList) {
    // LoadImg / Add* / CloseImg all operate on the active panel, so redirect
    // there before dispatching. This is always safe and makes sure an .img
    // dropped onto an empty panel opens in that exact panel.
    active_panel = panel_idx;

    if (FilesList.size() == 1 && DirsList.isEmpty()) {
        QString ext = QFileInfo(FilesList[0]).suffix().toLower();
        if (ext == "img" || ext == "imd") {
            LoadImg(FilesList[0]);
        }
        else if (panels[panel_idx] && panels[panel_idx]->in_subdir) {
            AddFiles(FilesList, panels[panel_idx]->current_dir);
        }
    }
    else if (panels[panel_idx] && !panels[panel_idx]->in_subdir) {
        AddDirs(DirsList);
    }
    else if (panels[panel_idx] && panels[panel_idx]->in_subdir) {
        AddFiles(FilesList, panels[panel_idx]->current_dir);
    }

    refreshActivePanelUI();
}

void MainWindow::onOpenRequested(int panel_idx) {
    // The empty placeholder was clicked while no disk is loaded -- behave as
    // if the user had pushed the "open" button for this specific panel.
    active_panel = panel_idx;
    refreshActivePanelUI();
    OpenImg();
}

void MainWindow::Extract(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QVector<int> selected = panelWidget(active_panel)->selectedFileIndices();
        if (selected.isEmpty())
            return;
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract to"), "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);
        if (todir == "")
            return;
        for (int idx : selected){
            ccos_inode_t* file = panel.inodes[idx];
            if (ccos_is_dir(file))
                dumpDirQt(panel.disk, file, todir, this);
            else
                dumpFileQt(panel.disk, file, todir, this);
        }
    }
}

void MainWindow::ExtractAll(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QString todir = QFileDialog::getExistingDirectory(this, tr("Extract all to"), "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);
        if (todir == "")
            return;
        int res = dumpImgQt(panel.disk, todir, QFileInfo(panel.path).baseName(), this);
        if (res == -1){
            QMessageBox::critical(this, "Unable to extract image", "Unable to extract image. Please check the path.");
        }
    }
}

void MainWindow::refreshActivePanelUI(){
    // Focus drives the active panel: setFocus() forwards to the table, whose
    // FocusIn bolds the title and emits panelActivated (which in turn sets
    // active_panel). HDDMenu still needs the logical active_panel here.
    panelWidget(active_panel)->setFocus();
    HDDMenu(panels[active_panel] && panels[active_panel]->hdd_mode);
    updateActionStates();
}

void MainWindow::updateActionStates() {
    const bool has_panel = panels[active_panel].has_value();
    const bool in_subdir = has_panel && panels[active_panel]->in_subdir;

    // "Add Files" is only meaningful inside a directory; "Create Directory"
    // only in the root. Both share the same shortcut and slot position, so
    // toggling visibility makes the menu show exactly one of them.
    ui->actionAdd->setVisible(has_panel && in_subdir);
    ui->actionMake_dir->setVisible(has_panel && !in_subdir);

    // Disk vs HDD-partition wording for the label action.
    const bool hdd = has_panel && panels[active_panel]->hdd_mode;
    ui->actionChange_label->setText(hdd ? tr("Change Partition Name...")
                                         : tr("Change Disk Label..."));
}

void MainWindow::Search() {
    panelWidget(active_panel)->toggleSearch();
}

void MainWindow::onSearchRequested(int panel_idx, const QString& query) {
    if (!panels[panel_idx])
        return;

    auto& panel = *panels[panel_idx];
    panel.search_query = query;
    if (query.isEmpty()) {
        fillTable(panel_idx, panel.current_dir, panel.in_subdir);
    } else {
        showSearchResults(panel_idx);
    }
}

void MainWindow::onPanelActivated(int panel_idx) {
    active_panel = panel_idx;
    refreshActivePanelUI();
}

void MainWindow::onGoUpToParent(int panel_idx) {
    goToParentDir(panel_idx);
}

void MainWindow::onFileDoubleClicked(int panel_idx, int fileIndex) {
    auto& panel = *panels[panel_idx];
    ccos_inode_t* entry = panel.inodes[fileIndex];
    if (ccos_is_dir(entry)) {
        saveCurrentViewState(panel_idx);  // remember the root view for when we come back
        panel.current_dir = entry;
        panel.in_subdir = true;
        fillTable(panel_idx, entry, panel.in_subdir);
    } else {
        doPreview(panel_idx, entry);
    }
}

void MainWindow::onFileRightClicked(int panel_idx, int fileIndex, const QPoint& /*globalPos*/) {
    doPreview(panel_idx, panels[panel_idx]->inodes[fileIndex]);
}

void MainWindow::HDDMenu(bool enab){
    ui->actionAct_part->setEnabled(enab);
    ui->actionAno_part->setEnabled(enab);
    ui->actionSep_save->setEnabled(enab);
}

void MainWindow::doPreview(int panel_idx, ccos_inode_t* file){
    if (!panels[panel_idx] || file == nullptr)
        return;
    if (ccos_is_dir(file))
        return;
    PreviewDlg dlg(panels[panel_idx]->disk, file, this);
    dlg.exec();
}

void MainWindow::ShowPreview(){
    int panel_idx = active_panel;
    if (!panels[panel_idx])
        return;
    int idx = panelWidget(panel_idx)->currentFileIndex();
    if (idx < 0)
        return;
    doPreview(panel_idx, panels[panel_idx]->inodes[idx]);
}

void MainWindow::Label(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QString dsk;
        if (active_panel == 0)
            dsk= "I";
        else
            dsk= "II";
        QString fname = short_string_to_qstring(ccos_get_disk_label(panel.disk));
        bool ok = false;
        QString nameQ = QInputDialog::getText(this, tr("New label"),
                                              QString("Set new label for the disk %1:").arg(dsk),
                                              QLineEdit::Normal, fname, &ok);

        if (!ok || validString(nameQ, false, this) == -1)
            return;

        ccos_set_disk_label(panel.disk, nameQ.toStdString().c_str());
        panel.modified = true;
        refreshPanel(active_panel);
    }
}

bool MainWindow::isFileAlreadyOpened(const QString& path) {
    int other_panel = !active_panel;
    return panels[other_panel] && panels[other_panel]->path == path;
}

void MainWindow::handleAlreadyOpenedImg(const QString& path, int targetPanel) {
    const int sourcePanel = !targetPanel;
    Q_ASSERT(panels[sourcePanel] && panels[sourcePanel]->path == path);

    if (!panels[sourcePanel]->hdd_mode) {
        QMessageBox::critical(this, "Image already open",
                              "This image is already open in the other panel!");
        return;
    }

    if (!suggestSelectAnotherPartition()) {
        return;
    }

    // Modal dialogs can change focus, and therefore active_panel. Preserve the
    // source and target selected before showing them.
    openAnotherPartition(targetPanel, sourcePanel);
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

static ccos_disk_t* tryOpenAs(
    uint8_t* data, size_t size, uint16_t sector_size, uint16_t superblock_fid, uint16_t bitmap_fid
) {
    ccos_disk_t* disk = (sector_size == GRID_BUBBLE_SECTOR_SIZE)
        ? ccos_disk_new_bubble(data, size, superblock_fid, bitmap_fid)
        : ccos_disk_new_extdisk(data, size, superblock_fid, bitmap_fid);
    if (disk == nullptr) {
        return nullptr;
    }

    ccos_inode_t* root = ccos_get_root_dir(disk);
    if (root == nullptr) {
        free(disk);
        return nullptr;
    }

    return disk;
}

static ccos_disk_t* tryFromBootsector(uint8_t* data, size_t size) {
    if (size < sizeof(ccos_boot_sector_t)) {
        return nullptr;
    }

    const ccos_boot_sector_t* boot = (const ccos_boot_sector_t*)data;
    uint16_t superblock = boot->superblock_fid;
    uint16_t bitmap = boot->bitmap_fid;

    if (superblock == 0 || bitmap == 0) {
        return nullptr;
    }

    return tryOpenAs(data, size, GRID_FLOPPY_SECTOR_SIZE, superblock, bitmap);
}

static ccos_disk_t* tryDetectBySize(uint8_t* data, size_t size) {
    ccos_disk_t* disk = tryOpenAs(data, size,
        GRID_BUBBLE_SECTOR_SIZE, GRID_BUBBLE_SUPERBLOCK_FID, GRID_BUBBLE_BITMAP_FID);
    if (disk) {
        return disk;
    }

    disk = tryOpenAs(data, size,
        GRID_FLOPPY_SECTOR_SIZE, GRID_FLOPPY_SUPERBLOCK_FID, GRID_FLOPPY_BITMAP_FID);
    if (disk) {
        return disk;
    }

    return tryOpenAs(data, size,
        GRID_HDD_SECTOR_SIZE, GRID_HDD_SUPERBLOCK_FID, GRID_HDD_BITMAP_FID);
}

void MainWindow::openValidNonMbrDisk(QString path, ccos_disk_t* disk) {
    ccos_inode_t* root = ccos_get_root_dir(disk);
    Q_ASSERT(root != nullptr);

    panels[active_panel].emplace();

    auto& panel = *panels[active_panel];
    panel.path = path;
    panel.disk = disk;
    panel.current_dir = root;

    fillTable(active_panel, root, false);
    HDDMenu(false);
}

static ccos_disk_t* tryOpenMbrPartition(uint8_t* data, MbrPartition& partition) {
    ccos_disk_t* disk = tryFromBootsector(data + partition.offset, partition.size);
    if (disk) {
        return disk;
    }

    return tryOpenAs(data + partition.offset, partition.size,
        GRID_HDD_SECTOR_SIZE, GRID_HDD_SUPERBLOCK_FID, GRID_HDD_BITMAP_FID);
}

static QString mbrPartitionLabel(uint8_t* hdd_data, const MbrPartition& part) {
    if (!part.isGRiD) {
        return {};
    }
    MbrPartition mut = part;
    ccos_disk_t* disk = tryOpenMbrPartition(hdd_data, mut);
    if (!disk) {
        return {};
    }
    QString label = short_string_to_qstring(ccos_get_disk_label(disk));
    free(disk);
    return label;
}

void MainWindow::tryToOpenValidMbrDisk(QString path, uint8_t* data, size_t size) {
    Q_ASSERT(isMbrDisk(data, size));

    std::vector<uint8_t> hdd_data(data, data+size);
    free(data);

    std::vector<MbrPartition> parts = parseMbr(hdd_data.data(), hdd_data.size());

    int countOfGRIDParts = 0;
    for (const auto& part : parts) {
        if (part.isGRiD) {
            countOfGRIDParts += 1;
        }
    }

    if (countOfGRIDParts == 0) {
        QMessageBox::critical(this, "MBR: No GRiD partitions",
                                    "No GRiD partitions found on the disk!");
        return;
    }

    QStringList labels;
    for (const auto& p : parts)
        labels << mbrPartitionLabel(hdd_data.data(), p);

    PartitionDlg dlg(this);
    dlg.setTitle("MBR: Select disk partition");
    dlg.setInfo("Hard disk with MBR detected.\nSelect the GRiD disk partition you want to work with:");
    dlg.setPartitions(parts, labels);

    while (true) {
        if (dlg.exec() != QDialog::Accepted) {
            break;
        }

        int selected = dlg.selectedIndex();

        ccos_disk_t* disk = tryOpenMbrPartition(hdd_data.data(), parts[selected]);
        if (disk) {
            if (!CloseImg()) {
                free(disk);
                break;
            }
            openValidMbrPartition(path, std::move(hdd_data), selected, disk);
            break;
        }

        QMessageBox::warning(this, "MBR: Bad GRiD partition",
                                        "Failed to open selected partition");
    }
}

void MainWindow::openValidMbrPartition(QString path, std::vector<uint8_t> hdd_data, int partition_index, ccos_disk_t* disk) {
    ccos_inode_t* root = ccos_get_root_dir(disk);
    Q_ASSERT(root != nullptr);

    panels[active_panel].emplace();

    auto& panel = *panels[active_panel];
    panel.path = path;
    panel.disk = disk;
    panel.current_dir = root;
    panel.hdd_mode = true;
    panel.hdd_data = std::make_shared<std::vector<uint8_t>>(std::move(hdd_data));
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

    CustomDiskDlg cdlg(this, true);
    while (true) {
        if (cdlg.exec() != 1) {
            free(data);
            break;
        }

        uint16_t sector_size, superblock, bitmap;
        cdlg.GetParams(&sector_size, &superblock, &bitmap, nullptr, nullptr);

        ccos_disk_t* disk = tryOpenAs(data, size, sector_size, superblock, bitmap);
        if (disk) {
            if (!CloseImg()) {
                free(data);
                free(disk);
                return;
            }
            openValidNonMbrDisk(path, disk);
            return;
        }

        QMessageBox::critical(this, "Failed to open",
                                              "Failed to open this file with specified parameters!");
    }
}

// IMD-aware entry point: if the file is an .IMD image, convert it to a
// temporary raw .img first, run the standard opening pipeline, then mark
// the resulting panel so that direct Save back to IMD is blocked.
void MainWindow::LoadImg(QString path) {
    if (path.isEmpty()) {
        return;
    }

    bool fromImd = false;
    if (QFileInfo(path).suffix().toLower() == "imd") {
        QString tempImg = convertImdToTempImg(path, this);
        if (tempImg.isEmpty()) {
            return;
        }
        fromImd = true;
        path = tempImg;
    }

    loadImgStandard(path);

    if (fromImd && panels[active_panel] && panels[active_panel]->disk) {
        panels[active_panel]->is_imd = true;
    }
}

void MainWindow::loadImgStandard(QString path) {
    if (path.isEmpty()) {
        return;
    }

    const int targetPanel = active_panel;

    // Same image already open on this panel -- nothing to do.
    if (panels[targetPanel] && panels[targetPanel]->path == path)
        return;

    // Same image open on the other panel.
    const int otherPanel = !targetPanel;
    if (panels[otherPanel] && panels[otherPanel]->path == path) {
        handleAlreadyOpenedImg(path, targetPanel);
        return;
    }

    // Read the file BEFORE closing the current image, so that a read error or
    // a user-cancelled dialog leaves the existing disk intact.
    uint8_t* data;
    size_t size;
    if (readFileQt(path, &data, &size, this)) {
        return;
    }

    ccos_disk_t* disk = tryFromBootsector(data, size);
    if (!disk)
        disk = tryDetectBySize(data, size);
    if (disk) {
        if (!CloseImg()) {
            free(data);
            free(disk);
            return;
        }
        openValidNonMbrDisk(path, disk);
        return;
    }

    if (isMbrDisk(data, size)) {
        // tryToOpenValidMbrDisk takes ownership of data and calls CloseImg()
        // itself, only after the user confirms a partition.
        tryToOpenValidMbrDisk(path, data, size);
        return;
    }

    // loadCustomImg takes ownership of data and calls CloseImg() itself, only
    // after the user successfully picks parameters.
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
                size_t frsp = 0;
                if (ccos_calc_free_space(panel.disk, &frsp) != CCOS_OK) {
                    QMessageBox::critical(this, "Calculation error",
                                    "Program can't calculate free space in the image!");
                    break;
                }
                if (frsp < 1024) {
                    QMessageBox::critical(this, "Not enough space",
                                    QString("Requires %1 bytes of additional disk space to make dir!").arg(1024-frsp));
                    break;
                }
                ccos_inode_t* root = ccos_get_root_dir(panel.disk);
                if (ccos_create_dir(panel.disk, root, name.toStdString().c_str()) == nullptr){
                    QMessageBox::critical(this, "Failed to create folder",
                                    "Program can't create a folder in the image!");
                    break;
                }
                panel.modified = true;
                refreshPanel(active_panel);
                break;
            }
        }
    }
}

void MainWindow::NewImage(){
    ImageCreationWizard wizard(this);
    if (wizard.exec() != QDialog::Accepted) return;

    const ImageCreationWizard::Result settings = wizard.result();
    if (panels[active_panel] && !CloseImg()) return;

    if (settings.kind == ImageCreationWizard::ImageKind::HardDisk && settings.useMbr) {
        createMbrImage(settings);
    } else {
        createMonolithicImage(settings);
    }
}

void MainWindow::createMbrImage(const ImageCreationWizard::Result& settings) {
    std::vector<std::vector<uint8_t>> partitions;
    partitions.reserve(settings.partitionSizesMiB.size());
    for (int index = 0; index < settings.partitionSizesMiB.size(); ++index) {
        const size_t partitionBytes = size_t(settings.partitionSizesMiB[index]) * 1024 * 1024;
        ccos_disk_t* partition = nullptr;
        if (ccos_new_disk_image(CCOS_DISK_FORMAT_COMPASS, partitionBytes, &partition) != 0 || partition == nullptr) {
            QMessageBox::critical(this, "Creation error", "Program can't create a new MBR partition!");
            return;
        }
        if (!settings.partitionLabels.value(index).isEmpty()) {
            ccos_set_disk_label(partition, settings.partitionLabels[index].toStdString().c_str());
        }
        partitions.emplace_back(ccos_disk_data(partition), ccos_disk_data(partition) + partitionBytes);
        ccos_disk_free(partition);
    }

    std::optional<MbrImage> image = buildMbrImage(partitions);
    if (!image.has_value()) {
        QMessageBox::critical(this, "Creation error", "Program can't create a new MBR image!");
        return;
    }

    auto hddData = std::make_shared<std::vector<uint8_t>>(std::move(image->data));
    ccos_disk_t* disk = tryOpenMbrPartition(hddData->data(), image->firstPartition);
    if (disk == nullptr) {
        QMessageBox::critical(this, "Creation error", "Program can't open the newly created MBR partition!");
        return;
    }

    panels[active_panel].emplace();
    auto& panel = *panels[active_panel];
    panel.disk = disk;
    panel.hdd_mode = true;
    panel.hdd_data = std::move(hddData);
    panel.hdd_partition = 0;
    panel.modified = true;
    panel.current_dir = ccos_get_root_dir(disk);
    fillTable(active_panel, panel.current_dir, false);
    HDDMenu(true);
}

void MainWindow::createMonolithicImage(const ImageCreationWizard::Result& settings) {
    disk_format_t format = CCOS_DISK_FORMAT_COMPASS;
    size_t imageSize = 0;
    if (settings.kind == ImageCreationWizard::ImageKind::Bubble) {
        format = CCOS_DISK_FORMAT_BUBMEM;
        imageSize = 384 * 1024;
    } else if (settings.kind == ImageCreationWizard::ImageKind::Floppy) {
        format = settings.floppyKind == ImageCreationWizard::FloppyKind::GridCase720K
            ? CCOS_DISK_FORMAT_GRIDCASE : CCOS_DISK_FORMAT_COMPASS;
        imageSize = settings.floppyKind == ImageCreationWizard::FloppyKind::GridCase720K
            ? 720 * 1024 : 360 * 1024;
    } else {
        imageSize = size_t(settings.sizeMiB) * 1024 * 1024;
    }

    ccos_disk_t* disk = nullptr;
    if (ccos_new_disk_image(format, imageSize, &disk) != 0 || disk == nullptr) {
        QMessageBox::critical(this, "Creation error", "Program can't create new image!");
        return;
    }
    if (!settings.label.isEmpty()) {
        ccos_set_disk_label(disk, settings.label.toStdString().c_str());
    }

    panels[active_panel].emplace();
    auto& panel = *panels[active_panel];
    panel.disk = disk;
    panel.modified = true;
    panel.current_dir = ccos_get_root_dir(disk);
    fillTable(active_panel, panel.current_dir, false);
}



bool MainWindow::goToParentDir(int panel_idx) {
    if (!panels[panel_idx]) {
        return false;
    }
    auto& panel = *panels[panel_idx];
    if (!panel.in_subdir) {
        return false;  // already at the disk root, nothing above it
    }
    ccos_inode_t* root = ccos_get_root_dir(panel.disk);
    saveCurrentViewState(panel_idx);  // remember the subfolder view before leaving
    panel.current_dir = ccos_get_parent_dir(panel.disk, panel.current_dir);
    if (panel.current_dir == root) {
        panel.in_subdir = false;
    }
    fillTable(panel_idx, panel.current_dir, panel.in_subdir);
    return true;
}

void MainWindow::OpenImg(){
    QString path = QFileDialog::getOpenFileName(this, "Open Image", "",
                                                 "GRiD image files (*.img *.imd);;"
                                                 "All files (*)");
    LoadImg(path);
}

void MainWindow::Rename(){
    if (panels[active_panel]){
        auto& panel = *panels[active_panel];
        QVector<int> selected = panelWidget(active_panel)->selectedFileIndices();
        if (selected.isEmpty())
            return;

        for (int idx : selected){
            ccos_inode_t* reninode = panel.inodes[idx];

            char basename[CCOS_MAX_FILE_NAME];
            char type[CCOS_MAX_FILE_NAME];
            memset(basename, 0, CCOS_MAX_FILE_NAME);
            memset(type, 0, CCOS_MAX_FILE_NAME);
            ccos_parse_file_name(reninode, basename, type, nullptr, nullptr);
            RenameDlg dlg(this, !panel.in_subdir); //All files in the root are directories
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
                        ccos_rename_file(panel.disk, reninode, newname.toStdString().c_str(),
                                         newtype.toStdString().c_str());
                        panel.modified = true;
                        refreshPanel(active_panel);
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

    if (panels[active_panel]->is_imd) {
        QMessageBox::warning(this, "Saving IMD is not supported",
                        "This image was opened from an IMD file.\n"
                        "Saving back to IMD format is not supported.\n\n"
                        "Please use \"Save as\" to export the image as a .img file.");
        return;
    }

    auto& panel = *panels[active_panel];
    if (!panel.modified)
        return;

    int res;
    if (panel.hdd_mode)
        res = saveFileQt(panel.path, panel.hdd_data->data(), panel.hdd_data->size(), this);
    else
        res = saveFileQt(panel.path, ccos_disk_data(panel.disk), ccos_disk_size(panel.disk), this);

    if (res == -1){
        QMessageBox::critical(this, "Unable to save file",
                        QString("Unable to save file \"%1\". Please check the path.").arg(panel.path));
        return;
    }
    panel.modified = false;
    int other = !active_panel;
    bool shared = panel.hdd_data && panel.hdd_data.use_count() > 1 && panels[other];
    if (shared)
        panels[other]->modified = false;
    updatePanelTitle(active_panel);
    if (shared)
        updatePanelTitle(other);
}

void MainWindow::SaveAs(){
    if (!panels[active_panel])
        return;

    auto& panel = *panels[active_panel];
    QString nameQ = QFileDialog::getSaveFileName(this, tr("Save as"), "", "GRiD Image Files (*.img)");
    if (nameQ == "")
        return;

    int res;
    if (panel.hdd_mode)
        res = saveFileQt(nameQ, panel.hdd_data->data(), panel.hdd_data->size(), this);
    else
        res = saveFileQt(nameQ, ccos_disk_data(panel.disk), ccos_disk_size(panel.disk), this);

    if (res == -1){
        QMessageBox::critical(this, "Unable to save file",
                              QString("Unable to save file \"%1\". Please check the path.").arg(nameQ));
        return;
    }
    if (panel.is_imd){
        // The old path pointed to a temporary converted image; drop it and
        // turn this into a normal standalone .img from now on.
        QString oldTemp = panel.path;
        panel.path = nameQ;
        QFile::remove(oldTemp);
        panel.is_imd = false;
    } else {
        panel.path = nameQ;
    }
    if (panel.modified){
        panel.modified = false;
        int other = !active_panel;
        bool shared = panel.hdd_data && panel.hdd_data.use_count() > 1 && panels[other];
        if (shared)
            panels[other]->modified = false;
        updatePanelTitle(active_panel);
        if (shared)
            updatePanelTitle(other);
    }
}

void MainWindow::SavePart(){
    auto& panel = *panels[active_panel];
    panel.hdd_mode = false;
    bool oldch = panel.modified;
    panel.modified = true;
    SaveAs();

    if (!panel.modified){
        // Detach the partition data from the shared HDD buffer by copying it
        // into a standalone buffer owned by a new disk handle.
        size_t dsize = ccos_disk_size(panel.disk);
        uint8_t* imdat = (uint8_t*)calloc(dsize, sizeof(uint8_t));
        memcpy(imdat, ccos_disk_data(panel.disk), dsize);
        ccos_disk_t* new_disk = (ccos_disk_sector_size(panel.disk) == GRID_BUBBLE_SECTOR_SIZE)
            ? ccos_disk_new_bubble(imdat, dsize, ccos_disk_superblock(panel.disk), ccos_disk_bitmap(panel.disk))
            : ccos_disk_new_extdisk(imdat, dsize, ccos_disk_superblock(panel.disk), ccos_disk_bitmap(panel.disk));
        // The old handle wraps data inside hdd_data, so release the handle only.
        free(panel.disk);
        panel.disk = new_disk;
        panel.hdd_data.reset();
    }
    else{
        panel.hdd_mode = true;
        panel.modified = oldch;
    }
    updatePanelTitle(active_panel);
}

void MainWindow::SetActivePart() {
    auto& panel = *panels[active_panel];
    uint8_t* mbrtab = panel.hdd_data->data() + 0x1BE;

    std::vector<MbrPartition> parts = parseMbr(panel.hdd_data->data(), panel.hdd_data->size());

    QStringList labels;
    for (const auto& p : parts)
        labels << mbrPartitionLabel(panel.hdd_data->data(), p);

    PartitionDlg dlg(this);
    dlg.setTitle("Select disk partition");
    dlg.setInfo("Select the GRiD disk partition to make it active:");
    dlg.allowNone("No active partition");
    dlg.setPartitions(parts, labels);

    if (dlg.exec() == QDialog::Accepted){
        int idx = dlg.selectedIndex();

        mbrtab[0] = 0x0;
        mbrtab[16] = 0x0;
        mbrtab[32] = 0x0;
        mbrtab[48] = 0x0;
        if (idx >= 0)
            mbrtab[parts[idx].index * 16] = 0x80;

        panel.modified = true;
        refreshPanel(active_panel);
    }
}

void MainWindow::Version(){
    if (!panels[active_panel]) {
        return;
    }

    auto& panel = *panels[active_panel];

    int idx = panelWidget(active_panel)->currentFileIndex();
    if (idx < 0) return;

    ccos_inode_t* file = panel.inodes[idx];
    ccos_version_t ver = ccos_get_file_version(file);
    VersionDlg dlg(this);
    dlg.init(file->desc.name, ver);
    if (dlg.exec() == 1){
        ver = dlg.retVer();
        ccos_set_file_version(panel.disk, file, ver);
        panel.modified = true;
        refreshPanel(active_panel);
    }
}

MainWindow::~MainWindow() = default;
