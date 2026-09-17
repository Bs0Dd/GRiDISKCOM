#include "font.h"

#include <QByteArray>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStringList>
#include <QVBoxLayout>

#include <array>
#include <cmath>
#include <cstring>

#include "omf_record.h"

namespace {

constexpr int kHeaderSize  = 8;
constexpr int kGridColumns = 16;  // glyphs per row in the table

constexpr int kFullCharCount = 256;  // numChars == 0 encodes the full set
constexpr int kMaxGlyphDim   = 64;
constexpr int kMaxLineHeight = 80;

constexpr QRgb kFg     = qRgb(0xFF, 0xEB, 0x00);
constexpr QRgb kBg     = qRgb(0x00, 0x00, 0x00);
constexpr QRgb kBorder = qRgb(0x40, 0x40, 0x40);

struct RawFontInfo {
    int charCount = 0;
    int charWidth = 0;
    int charHeight = 0;
    int effectiveLineHeight = 0;
    int baseLine = 0;
    int bytesPerRow = 0;
    size_t expectedSize = 0;
};

struct FoundFont {
    size_t offset = 0;
    long long leftover = 0;
    RawFontInfo info;
};

bool parseHeaderAt(const uint8_t* data, size_t size, size_t offset, RawFontInfo& out) {
    if (offset + size_t(kHeaderSize) > size)
        return false;

    const uint8_t* h = data + offset;
    const int numChars = h[0];
    const int lineHeight = h[3];

    out.charWidth  = h[1];
    if (out.charWidth <= 0 || out.charWidth > kMaxGlyphDim)
        return false;

    out.charHeight = h[2];
    if (out.charHeight <= 0 || out.charHeight > kMaxGlyphDim)
        return false;

    out.baseLine = h[4];

    out.charCount = numChars ? numChars : kFullCharCount;
    if (out.charCount <= 0 || out.charCount > kFullCharCount)
        return false;

    out.bytesPerRow = (out.charWidth + 7) / 8;
    if (out.bytesPerRow <= 0)
        return false;

    out.effectiveLineHeight = lineHeight ? lineHeight : (out.charHeight + 1);
    out.expectedSize = size_t(kHeaderSize) +
                       size_t(out.bytesPerRow) * size_t(out.charHeight) * size_t(out.charCount);

    return out.effectiveLineHeight >= out.charHeight &&
           out.effectiveLineHeight <= kMaxLineHeight &&
           out.baseLine <= out.effectiveLineHeight &&
           out.expectedSize <= size - offset;
}

bool findRawFont(const uint8_t* data, size_t size, FoundFont& out) {
    RawFontInfo info;
    if (!parseHeaderAt(data, size, 0, info)) return false;
    out = { 0, (long long)size - (long long)info.expectedSize, info };
    return true;
}

bool readOmfIndex(const uint8_t* p, const uint8_t* end, int& value, const uint8_t*& next) {
    if (p >= end) return false;
    const uint8_t first = *p;
    if (first & 0x80) {
        if (p + 1 >= end) return false;
        value = (int(first & 0x7F) << 8) | p[1];
        next = p + 2;
    } else {
        value = first;
        next = p + 1;
    }
    return true;
}

void placeChunk(QByteArray& seg, int offset, const uint8_t* chunk, int len) {
    const int need = offset + len;
    if (seg.size() < need) {
        const int oldSize = seg.size();
        seg.resize(need);
        std::memset(seg.data() + oldSize, 0, size_t(need - oldSize));
    }
    std::memcpy(seg.data() + offset, chunk, size_t(len));
}

bool tryExtractOmfFont(const uint8_t* data, size_t size, QByteArray& outPayload, RawFontInfo& outInfo) {
    constexpr uint8_t kLedata = 0xA0;

    QMap<int, QByteArray> segments;
    int ledataCount = 0;
    const uint8_t* p = data;
    const uint8_t* end = data + size;
    while (p < end) {
        const OmfRecord rec = readOmfRecord(p, end);  // shared OMF record reader
        if (rec.payload == nullptr) break;
        p = rec.payload + rec.length;  // advance past this record
        if (rec.type != kLedata || rec.length < 2) continue;

        const uint8_t* contentEnd = rec.payload + rec.length - 1;  // drop the checksum byte
        int segmentIndex = 0;
        const uint8_t* q = rec.payload;
        if (!readOmfIndex(q, contentEnd, segmentIndex, q)) continue;
        if (q + 2 > contentEnd) continue;
        const int dataOffset = int(q[0]) | (int(q[1]) << 8);
        q += 2;

        const int chunkLen = int(contentEnd - q);
        if (chunkLen <= 0) continue;
        QByteArray& seg = segments[segmentIndex];
        placeChunk(seg, dataOffset, q, chunkLen);
        ledataCount++;
    }

    if (ledataCount == 0) return false;

    bool found = false;
    long long bestLeftover = 0;
    for (const QByteArray& seg : segments) {
        FoundFont candidate;
        if (!findRawFont(reinterpret_cast<const uint8_t*>(seg.constData()), seg.size(), candidate)) {
            continue;
        }
        if (!found || std::llabs(candidate.leftover) < std::llabs(bestLeftover)) {
            bestLeftover = candidate.leftover;
            outPayload = seg.mid(int(candidate.offset), int(candidate.info.expectedSize));
            outInfo = candidate.info;
            found = true;
        }
    }
    return found;
}

bool extractFontPayload(const uint8_t* data, size_t size,
                        QByteArray& outPayload, RawFontInfo& outInfo) {
    return tryExtractOmfFont(data, size, outPayload, outInfo);
}

QImage buildGlyphImage(const uint8_t* payload, const RawFontInfo& info, int gi) {
    QImage img(info.charWidth, info.charHeight, QImage::Format_Mono);
    img.setColor(0, kBg);
    img.setColor(1, kFg);
    img.fill(0);

    for (int y = 0; y < info.charHeight; y++) {
        const size_t rowOffset = size_t(kHeaderSize) +
                                 size_t(y) * info.charCount * info.bytesPerRow +
                                 size_t(gi) * info.bytesPerRow;
        if (rowOffset + size_t(info.bytesPerRow) > info.expectedSize) break;
        std::memcpy(img.scanLine(y), payload + rowOffset, info.bytesPerRow);
    }
    return img;
}

QImage renderFontGrid(const uint8_t* payload, const RawFontInfo& info, int scale) {
    const int cellSize = std::max(info.charWidth, info.effectiveLineHeight) * scale;
    const int rows = (info.charCount + kGridColumns - 1) / kGridColumns;
    const int border = 1;
    const int imgW = kGridColumns * (cellSize + border) + border;
    const int imgH = rows * (cellSize + border) + border;

    QImage img(imgW, imgH, QImage::Format_RGB32);
    img.fill(kBorder);

    const int glyphW = info.charWidth * scale;
    const int glyphH = info.charHeight * scale;
    const int offX = (cellSize - glyphW) / 2;
    const int offY = (cellSize - glyphH) / 2;

    QPainter p(&img);
    const int totalCells = rows * kGridColumns;
    for (int i = 0; i < totalCells; i++) {
        const int col = i % kGridColumns;
        const int row = i / kGridColumns;
        const int cellX = border + col * (cellSize + border);
        const int cellY = border + row * (cellSize + border);

        p.fillRect(cellX, cellY, cellSize, cellSize, QColor(kBg));
        if (i >= info.charCount) continue;

        const QImage glyph = buildGlyphImage(payload, info, i)
                                 .scaled(glyphW, glyphH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        p.drawImage(cellX + offX, cellY + offY, glyph);
    }
    return img;
}

int pickTableScale(int charHeight) {
    if (charHeight <= 0) {
        return 2;
    }
    int scale = int(std::lround(24.0 / charHeight));
    return std::max(2, std::min(3, scale));
}

constexpr const char* kDefaultSampleText = "GRiDISKCOM loves GRiD-OS files";

QStringList wrapText(const QString& text, int wrapCols) {
    QStringList lines;
    QString line;
    for (const QString& word : text.split(QLatin1Char(' '))) {
        const int add = word.length() + (line.isEmpty() ? 0 : 1);
        if (!line.isEmpty() && line.length() + add > wrapCols) {
            lines << line;
            line = word;
        } else {
            line = line.isEmpty() ? word : (line + QLatin1Char(' ') + word);
        }
    }
    lines << line;
    return lines;
}

class FontTextPreview : public QWidget {
public:
    explicit FontTextPreview(QWidget* parent = nullptr) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setAutoFillBackground(true);
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::black);
        setPalette(pal);
    }

    void setFontData(const QByteArray& payload, const RawFontInfo& info) {
        m_payload = payload;
        m_info = info;

        constexpr int kTargetGlyphH = 24;
        const double s = double(kTargetGlyphH) / std::max(1, info.charHeight);
        m_dispW = std::max(1, int(std::lround(info.charWidth * s)));
        m_dispH = std::max(1, int(std::lround(info.charHeight * s)));
        m_dispLineH = std::max(m_dispH, int(std::lround(info.effectiveLineHeight * s)));

        buildCodeMap();
        buildGlyphCache();
        rewrap();
        updateGeometry();
        update();
    }

    void setText(const QString& text) {
        m_text = text;
        rewrap();
        updateGeometry();
        update();
    }

    QSize sizeHint() const override {
        return QSize(width() > 0 ? width() : 600, heightForLines(m_lineCount));
    }

    QSize minimumSizeHint() const override {
        return QSize(0, heightForLines(1));
    }

protected:
    void resizeEvent(QResizeEvent*) override {
        rewrap();
        updateGeometry();
        update();
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);

        const int maxLines = height() / m_dispLineH;

        int y = 0;
        for (int li = 0; li < m_lines.size() && li < maxLines; li++) {
            const QString& line = m_lines[li];
            int x = 0;
            for (int ci = 0; ci < line.length(); ci++) {
                int code = line[ci].unicode();
                if (code > 255) code = '?';  // out of the font's byte range
                const int gi = m_codeToGlyph[code];
                if (gi >= 0) {
                    p.drawImage(x, y, m_glyphCache[gi]);
                }
                x += m_dispW;
            }
            y += m_dispLineH;
        }
    }

private:
    int heightForLines(int lines) const {
        const int n = std::max(2, std::min(6, lines));
        return n * m_dispLineH;
    }

    void buildCodeMap() {
        const int firstChar = (m_info.charCount >= 95 && m_info.charCount <= 96) ? 32 : 0;
        const int qGlyph = 63 - firstChar;
        const int missing = (qGlyph >= 0 && qGlyph < m_info.charCount) ? qGlyph : -1;
        for (int c = 0; c < 256; c++) {
            const int gi = c - firstChar;
            m_codeToGlyph[c] = (gi >= 0 && gi < m_info.charCount) ? gi : missing;
        }
    }

    void buildGlyphCache() {
        m_glyphCache.clear();
        m_glyphCache.reserve(m_info.charCount);
        const uint8_t* payload = reinterpret_cast<const uint8_t*>(m_payload.constData());
        for (int gi = 0; gi < m_info.charCount; gi++) {
            QImage base = buildGlyphImage(payload, m_info, gi);
            m_glyphCache.append(base.scaled(m_dispW, m_dispH,
                                            Qt::IgnoreAspectRatio, Qt::FastTransformation));
        }
    }

    void rewrap() {
        const int w = width() > 0 ? width() : 600;
        const int wrapCols = std::max(1, w / m_dispW);
        m_lines = wrapText(m_text, wrapCols);
        m_lineCount = std::max(1, m_lines.size());
    }

    QByteArray m_payload;
    RawFontInfo m_info;
    QString m_text;
    QVector<QImage> m_glyphCache;
    std::array<int, 256> m_codeToGlyph{};  // byte code -> glyph index (-1 = none)
    QStringList m_lines;
    int m_dispW = 8;
    int m_dispH = 8;
    int m_dispLineH = 10;
    int m_lineCount = 1;
};

}  // namespace

bool FontPreview::supports(const QString& fileType, size_t fileSize) const {
    return fileSize >= size_t(kHeaderSize) &&
        fileType.compare("font", Qt::CaseInsensitive) == 0;
}

QWidget* FontPreview::createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) {
    uint8_t* data = nullptr;
    size_t size = 0;

    if (ccos_read_file(disk, file, &data, &size) != CCOS_OK || data == nullptr) {
        QMessageBox::critical(parent, "Preview", "Failed to read file contents!");
        return nullptr;
    }

    QByteArray payload;
    RawFontInfo info;
    if (!extractFontPayload(data, size, payload, info)) {
        QMessageBox::warning(parent, "Preview", "Could not decode this font (no GRiD font payload found).");
        free(data);
        return nullptr;
    }
    free(data);

    QImage grid = renderFontGrid(reinterpret_cast<const uint8_t*>(payload.constData()),
                                 info, pickTableScale(info.charHeight));

    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // --- Typed-text preview ---
    auto* sampleEdit = new QLineEdit(container);
    sampleEdit->setText(QString::fromUtf8(kDefaultSampleText));
    sampleEdit->setClearButtonEnabled(true);
    sampleEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[^\n]*")), sampleEdit));
    QFont mono("Monospace");
    mono.setStyleHint(QFont::TypeWriter);
    sampleEdit->setFont(mono);
    layout->addWidget(sampleEdit);

    auto* textPreview = new FontTextPreview(container);
    textPreview->setFontData(payload, info);
    textPreview->setText(sampleEdit->text());
    layout->addWidget(textPreview);

    QObject::connect(sampleEdit, &QLineEdit::textChanged, textPreview, [textPreview](const QString& text) {
        textPreview->setText(text);
    });

    // --- Full glyph table ---
    auto* scroll = new QScrollArea(container);
    scroll->setWidgetResizable(false);
    scroll->setAlignment(Qt::AlignCenter);
    scroll->setFocusPolicy(Qt::NoFocus);
    // Plain black background, no native frame: KDE/Breeze otherwise paints a
    // blue rounded focus/hover rectangle around the viewport on mouse-over.
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: black; }"
        "QScrollArea > QWidget > QWidget { background: black; border: none; }"));
    auto* imageLabel = new QLabel(scroll);
    imageLabel->setPixmap(QPixmap::fromImage(grid));
    imageLabel->setFixedSize(grid.size());
    // The label is purely decorative: ignore mouse events so the toolkit never
    // enters a hover/focus state (which is what draws that blue rounded box).
    imageLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    imageLabel->setFocusPolicy(Qt::NoFocus);
    scroll->setWidget(imageLabel);
    layout->addWidget(scroll, 1);

    // --- Info label ---
    auto* infoLabel = new QLabel(
        QStringLiteral("Cell: %1 × %2    |    Line height: %3    |    Baseline: %4")
            .arg(info.charWidth)
            .arg(info.charHeight)
            .arg(info.effectiveLineHeight)
            .arg(info.baseLine),
        container);
    infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    infoLabel->setWordWrap(true);
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(infoLabel);

    return container;
}
