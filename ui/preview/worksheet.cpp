#include "worksheet.h"

#include "omf_record.h"

#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr uint8_t kTitleSubtype       = 0x68;
constexpr uint8_t kColumnLabelSubtype = 0x00;

constexpr int kMaxRows = 50000;
constexpr int kMaxCols = 256;

struct Worksheet {
    QString title;
    QMap<int, QString> columnLabels;  // 1-based column index -> label text
    QList<QStringList> rows;
    bool valid = false;
};

// Strip PCL escape sequences from raw bytes.
QString stripPclEsc(const uint8_t* data, int size) {
    QString out;
    out.reserve(size);

    int i = 0;
    while (i < size) {
        if (data[i] == 0x1B) {
            i++;
            while (i < size && !(data[i] >= 'A' && data[i] <= 'Z')) i++;
            if (i < size) i++;  // consume the terminator
        } else {
            out += QLatin1Char(static_cast<char>(data[i]));
            i++;
        }
    }

    return out.trimmed();
}

// Parse worksheet content into rows of tab-separated cells in a single pass,
// skipping over interleaved FE/FD OMF records (e.g. per-row formula byte-code).
// CSV text is plain ASCII, so 0xFE/0xFD never appear inside a cell value.
QList<QStringList> parseCsvRows(const uint8_t* data, size_t size) {
    QList<QStringList> rows;
    QStringList row;
    QByteArray cell;

    const uint8_t* p = data;
    const uint8_t* end = data + size;

    while (p < end) {
        if (*p == kOmfMetadataRecord || *p == kOmfLabelRecord) {
            const OmfRecord rec = readOmfRecord(p, end);
            if (rec.payload != nullptr) {
                p = rec.payload + rec.length;
                continue;
            }
        }

        const char c = static_cast<char>(*p++);
        if (c == '\t') {
            row.append(QString::fromLatin1(cell));
            cell.clear();
        } else if (c == '\n') {
            row.append(QString::fromLatin1(cell));
            rows.append(row);
            row.clear();
            cell.clear();
        } else if (c != '\r') {
            cell.append(c);
        }
    }

    if (!cell.isEmpty() || !row.isEmpty()) {
        row.append(QString::fromLatin1(cell));
        rows.append(row);
    }

    return rows;
}

Worksheet parseWorksheet(const uint8_t* data, size_t size, uint32_t propLength) {
    Worksheet ws;

    OmfMetadata md = parseOmfMetadata(data, size, propLength);
    if (md.records.empty()) {
        return ws;  // not a worksheet: no metadata stream
    }

    if (const OmfRecord* title = md.find(kOmfMetadataRecord, kTitleSubtype)) {
        // payload[0] = subtype 'h', payload[1..] = title text.
        ws.title = stripPclEsc(title->payload + 1, int(title->length - 1));
    }

    // payload[0] = subtype 0x00, payload[1] = 1-based column index, rest = label.
    // length >= 2 is required: payload[1] must exist. (subtype 0x00 also matches
    // zero-length records via subtype(), so the guard is not redundant.)
    for (const OmfRecord* r : md.findAll(kOmfLabelRecord, kColumnLabelSubtype)) {
        if (r->length >= 2) {
            int col = r->payload[1];
            ws.columnLabels[col] = QString::fromLatin1(
                reinterpret_cast<const char*>(r->payload + 2), int(r->length - 2));
        }
    }

    ws.rows = parseCsvRows(md.content, md.contentSize);
    ws.valid = true;

    return ws;
}

}  // namespace

bool WorksheetPreview::supports(const QString& fileType, size_t fileSize) const {
    return fileSize >= 4 &&
        fileType.compare("worksheet", Qt::CaseInsensitive) == 0;
}

QWidget* WorksheetPreview::createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) {
    uint8_t* data = nullptr;
    size_t size = 0;
    if (ccos_read_file(disk, file, &data, &size) != CCOS_OK || data == nullptr) {
        QMessageBox::critical(parent, "Preview", "Failed to read file contents!");
        return nullptr;
    }

    Worksheet ws = parseWorksheet(data, size, file->desc.prop_length);
    free(data);

    if (!ws.valid) {
        QMessageBox::warning(parent, "Preview", "Could not decode this worksheet (unknown format).");
        return nullptr;
    }

    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    if (!ws.title.isEmpty()) {
        auto* titleLabel = new QLabel(ws.title, container);
        QFont titleFont = titleLabel->font();
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setWordWrap(true);
        layout->addWidget(titleLabel);
    }

    int rowCount = qMin(ws.rows.size(), kMaxRows);
    int colCount = 0;
    for (const QStringList& row : ws.rows) {
        colCount = qMax(colCount, row.size());
    }
    for (auto it = ws.columnLabels.cbegin(); it != ws.columnLabels.cend(); ++it) {
        if (it.key() <= kMaxCols) {
            colCount = qMax(colCount, it.key());
        }
    }
    colCount = qMin(colCount, kMaxCols);

    auto* table = new QTableWidget(rowCount, colCount, container);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ContiguousSelection);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    table->setFont(font);

    if (ws.columnLabels.isEmpty()) {
        for (int c = 0; c < colCount; ++c) {
            table->setHorizontalHeaderItem(c, new QTableWidgetItem(QString::number(c + 1)));
        }
    } else {
        QStringList headerLabels;
        headerLabels.reserve(colCount);
        for (int c = 0; c < colCount; ++c) {
            headerLabels << ws.columnLabels.value(c + 1);
        }
        table->setHorizontalHeaderLabels(headerLabels);
    }

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    for (int r = 0; r < rowCount; ++r) {
        const QStringList& row = ws.rows[r];
        for (int c = 0; c < row.size(); ++c) {
            if (!row[c].isEmpty()) {
                table->setItem(r, c, new QTableWidgetItem(row[c]));
            }
        }
    }

    layout->addWidget(table, 1);

    return container;
}
