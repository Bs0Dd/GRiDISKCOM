#include "text.h"

#include "omf_record.h"

#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextLayout>
#include <QTextBlock>
#include <QPainter>
#include <QPaintEvent>
#include <QMessageBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>

// Decode file bytes as Latin-1, rendering control characters as Unicode
// control-picture glyphs (NUL -> ␀, BEL -> ␇, ESC -> ␛, DEL -> ␡, etc.) so they
// are visible instead of blank. Tab and newline are kept as-is to preserve layout.
static QString decodeText(const uint8_t* data, size_t size) {
    QString out;
    out.reserve(static_cast<int>(size));
    for (size_t i = 0; i < size; i++) {
        unsigned char c = data[i];
        if (c == '\t') {
            out += QLatin1Char('\t');
        } else if (c == '\n') {
            out += QChar(static_cast<ushort>(0x2400 + c));
            out += QLatin1Char('\n');
        } else if (c < 0x20) {
            out += QChar(static_cast<ushort>(0x2400 + c));
        } else if (c == 0x7F) {
            out += QChar(static_cast<ushort>(0x2421));  // SYMBOL FOR DELETE
        } else {
            out += QLatin1Char(static_cast<char>(c));
        }
    }
    return out;
}

// QPlainTextEdit subclass that paints a soft wrap marker ("↵") at the end of
// each visual line that was wrapped by word-wrap, so wrap points are visible.
// Only applies when lineWrapMode() is WidgetWidth; NoWrap draws nothing.
class WrapPreviewEdit : public QPlainTextEdit {
public:
    using QPlainTextEdit::QPlainTextEdit;

    // Toggle wrap mode while keeping the same block visible at the top,
    // so flipping the checkbox doesn't snap the view back to the start.
    void setLineWrapModePreservingScroll(LineWrapMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;
};

void WrapPreviewEdit::setLineWrapModePreservingScroll(LineWrapMode mode) {
    if (lineWrapMode() == mode) {
        return;
    }

    // Remember the first visible block and how far into it we've scrolled,
    // so we can restore roughly the same view after the document reflows.
    const QTextBlock first = firstVisibleBlock();
    const int oldBlockTop = first.isValid() ? static_cast<int>(blockBoundingGeometry(first).top()) : 0;
    const int offsetWithinBlock = verticalScrollBar()->value() - oldBlockTop;

    setLineWrapMode(mode);

    if (first.isValid()) {
        const int newBlockTop = static_cast<int>(blockBoundingGeometry(first).top());
        verticalScrollBar()->setValue(newBlockTop + offsetWithinBlock);
    }
}

void WrapPreviewEdit::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);

    // Nothing to mark when wrapping is off.
    if (lineWrapMode() == QPlainTextEdit::NoWrap) {
        return;
    }

    QPainter p(viewport());
    p.setPen(palette().color(QPalette::Disabled, QPalette::Text));
    p.setFont(font());

    static const QString wrapMark = QStringLiteral("\u21B5");

    const QRectF dirty = event->rect();
    const QPointF offset = contentOffset();
    QTextBlock block = firstVisibleBlock();

    while (block.isValid()) {
        const QRectF blockRect = blockBoundingGeometry(block).translated(offset);

        if (blockRect.top() > dirty.bottom()) {
            break;  // Rest of the document is below the dirty region.
        }
        if (blockRect.bottom() < dirty.top()) {
            block = block.next();
            continue;  // Block is above the dirty region.
        }

        // For a wrapped block, every visual line except the last continues
        // onto the next line, so mark those.
        QTextLayout* tl = block.layout();
        if (tl != nullptr) {
            const int lineCount = tl->lineCount();
            for (int i = 0; i < lineCount - 1; i++) {
                const QTextLine line = tl->lineAt(i);
                if (!line.isValid()) {
                    continue;
                }
                const qreal x = blockRect.left() + line.position().x() + line.naturalTextWidth() + 2;
                const qreal y = blockRect.top() + line.position().y() + line.ascent();
                p.drawText(QPointF(x, y), wrapMark);
            }
        }

        block = block.next();
    }
}

bool TextPreview::supports(const QString& fileType, size_t fileSize) const {
    (void)fileSize;
    for (const auto ext : {"text", "develop", "lst", "plm", "c", "h", "basic", "task", "com", "asm"}) {
        if (fileType.compare(ext, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QWidget* TextPreview::createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) {
    uint8_t* data = nullptr;
    size_t size = 0;
    if (ccos_read_file(disk, file, &data, &size) != CCOS_OK || data == nullptr) {
        QMessageBox::critical(parent, "Preview", "Failed to read file contents!");
        return nullptr;
    }

    auto* edit = new WrapPreviewEdit(parent);
    edit->setReadOnly(true);
    edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);  // word wrap on by default
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    edit->setFont(font);

    // The first `prop_length` bytes are the OMF metadata stream.
    // The rest is the text body.
    OmfMetadata md = parseOmfMetadata(data, size, file->desc.prop_length);
    edit->setPlainText(decodeText(md.content, md.contentSize));
    free(data);

    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* wrapCheck = new QCheckBox(QStringLiteral("Word wrap"), container);
    wrapCheck->setChecked(true);
    layout->addWidget(wrapCheck);

    layout->addWidget(edit, 1);

    QObject::connect(wrapCheck, &QCheckBox::toggled, edit, [edit](bool checked) {
        edit->setLineWrapModePreservingScroll(
            checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });

    return container;
}
