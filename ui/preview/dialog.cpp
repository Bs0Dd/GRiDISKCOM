#include "dialog.h"

#include "file.h"
#include "text.h"
#include "canvas.h"
#include "worksheet.h"
#include "font.h"
#include "database.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QGuiApplication>
#include <QScreen>

static QList<FilePreview*> previewRegistry() {
    static QList<FilePreview*> registry = {
        new TextPreview(),
        new CanvasPreview(),
        new WorksheetPreview(),
        new FontPreview(),
        new DatabasePreview(),
    };
    return registry;
}

PreviewDlg::PreviewDlg(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) : QDialog(parent) {
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(false);

    auto* layout = new QVBoxLayout(this);

    char basename[CCOS_MAX_FILE_NAME] = {0};
    char type[CCOS_MAX_FILE_NAME] = {0};
    ccos_parse_file_name(file, basename, type, nullptr, nullptr);
    setWindowTitle(QString("Preview %1~%2~ file")
                       .arg(QString::fromLatin1(basename), QString::fromLatin1(type)));
    size_t fileSize = file->desc.file_size;

    FilePreview* matched = nullptr;
    for (FilePreview* p : previewRegistry()) {
        if (p->supports(QString::fromLatin1(type), fileSize)) {
            matched = p;
            break;
        }
    }

    QWidget* content = nullptr;
    if (matched) {
        content = matched->createWidget(disk, file, this);
    }
    if (content == nullptr) {
        content = new QLabel(QString("No preview available for \"~%1~\".")
                                 .arg(QString::fromLatin1(type)), this);
        static_cast<QLabel*>(content)->setAlignment(Qt::AlignCenter);
    }
    layout->addWidget(content, 1);

    // A preview may ask for a roomier window than the default (e.g. the font
    // preview's glyph table). Honour it, but never go below the default or the
    // minimum, and never larger than ~85% of the screen so it stays usable.
    QSize size(660, 540);
    if (matched) {
        const QSize want = matched->preferredDialogSize();
        if (want.isValid() && !want.isEmpty()) {
            size = size.expandedTo(want);
        }
    }
    const QSize screen = QGuiApplication::primaryScreen()
                              ? QGuiApplication::primaryScreen()->availableSize()
                              : QSize();
    if (screen.isValid()) {
        size = size.boundedTo(QSize(screen.width() * 85 / 100, screen.height() * 85 / 100));
    }
    resize(size);
    setMinimumSize(360, 280);
    setSizeGripEnabled(true);
}

void PreviewDlg::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (auto* p = parentWidget()) {
        move(p->geometry().center() - QPoint(width() / 2, height() / 2));
    }
}
