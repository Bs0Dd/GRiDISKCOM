#include "canvaspreview.h"

#include "omf/omf_record.h"

#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMessageBox>
#include <QResizeEvent>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kMaxImageWidth  = 8192;
constexpr int kMaxImageHeight = 4096;

struct CanvasImage {
    QString format;
    int w = 0;
    int h = 0;
    int stride = 0;
    const uint8_t* raster = nullptr;
    size_t rasterLen = 0;
};

// Gridpaint packs pixels into 16-bit words (MSB first). The logical width need
// not be a multiple of 16, but each row is padded to a whole number of words:
// e.g. a 70-pixel-wide image is stored in 80-pixel (10-byte) rows.
inline int wordAlignedStride(int w) {
    return ((w + 15) / 16) * 2;
}

inline size_t rasterBytes(int w, int h) {
    return size_t(wordAlignedStride(w)) * size_t(h);
}

bool plausibleDims(int w, int h) {
    return w > 0 && w <= kMaxImageWidth && h > 0 && h <= kMaxImageHeight;
}

// Decodes GRiDPaint Canvas files. The image size is stored as a TLV record
// terminated by 0xFF: FE 04 00 <w_lo> <w_hi> <h_lo> <h_hi> FF <raster...>.
CanvasImage decodeFromMetadata(const OmfMetadata& md) {
    const uint8_t* end = md.content + md.contentSize;

    const OmfRecord dims = readOmfRecord(md.content, end);
    if (dims.type != kOmfMetadataRecord || dims.length != 4) {
        return CanvasImage{};
    }

    const int w = omfU16LE(dims.payload);
    const int h = omfU16LE(dims.payload + 2);
    if (!plausibleDims(w, h)) {
        return CanvasImage{};
    }

    const uint8_t* raster = dims.payload + 4;
    if (*raster++ != 0xFF) {
        return CanvasImage{};
    }

    const size_t expect = rasterBytes(w, h);
    if (end - raster < expect) {
        return CanvasImage{};
    }

    CanvasImage img;
    img.format = QStringLiteral("gridpaint");
    img.w = w;
    img.h = h;
    img.stride = wordAlignedStride(w);
    img.raster = raster;
    img.rasterLen = expect;

    return img;
}

CanvasImage decodeRawHeaderless(const uint8_t* data, size_t size) {
    constexpr int compassScreenWidth = 320;

    const int stride = wordAlignedStride(compassScreenWidth);
    if (size == 0 || size % stride != 0) {
        return CanvasImage{};
    }

    int h = static_cast<int>(size / stride);
    if (h <= 0 || h > kMaxImageHeight) {
        return CanvasImage{};
    }

    CanvasImage img;
    img.format = QStringLiteral("raw");
    img.w = compassScreenWidth;
    img.h = h;
    img.stride = stride;
    img.raster = data;
    img.rasterLen = size;

    return img;
}

CanvasImage detectCanvas(const uint8_t* data, size_t size, uint32_t propLength) {
    OmfMetadata md = parseOmfMetadata(data, size, propLength);
    if (CanvasImage img = decodeFromMetadata(md); img.raster) {
        return img;
    } else {
        return decodeRawHeaderless(md.content, md.contentSize);
    }
}

QImage renderMono(const uint8_t* data, size_t dataLen, int w, int h, int bytesPerRow) {
    QImage img(w, h, QImage::Format_Mono);
    img.setColor(0, qRgb(0, 0, 0));
    img.setColor(1, qRgb(0xFF, 0xEB, 0x00));
    img.fill(0);

    const int wordsPerRow = bytesPerRow / 2;
    const int rows = std::min(h, static_cast<int>(dataLen / bytesPerRow));
    for (int y = 0; y < rows; y++) {
        const uint8_t* src = data + y * bytesPerRow;
        uint8_t* dst = img.scanLine(y);

        for (int i = 0; i < wordsPerRow; i++) {
            dst[2 * i]     = src[2 * i + 1];
            dst[2 * i + 1] = src[2 * i];
        }
    }
    return img;
}

// QLabel that rescales its pixmap on resize, keeping aspect ratio with crisp
// nearest-neighbour scaling for pixel art.
class ImageLabel : public QLabel {
public:
    using QLabel::QLabel;

    void setSourceImage(const QImage& image) {
        source = image;
        rescale(size());
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        rescale(event->size());
    }

private:
    void rescale(const QSize& area) {
        if (source.isNull()) {
            return;
        }
        setPixmap(QPixmap::fromImage(
            source.scaled(area, Qt::KeepAspectRatio, Qt::FastTransformation)));
    }

    QImage source;
};

}  // namespace

bool CanvasPreview::supports(const QString& fileType, size_t fileSize) const {
    return fileSize >= 4 &&
        (fileType.compare("canvas", Qt::CaseInsensitive) == 0 ||
         fileType.compare("screenimage", Qt::CaseInsensitive) == 0);
}

QWidget* CanvasPreview::createWidget(ccos_disk_t* disk, ccos_inode_t* file, QWidget* parent) {
    uint8_t* data = nullptr;
    size_t size = 0;

    if (ccos_read_file(disk, file, &data, &size) != CCOS_OK || data == nullptr) {
        QMessageBox::critical(parent, "Preview", "Failed to read file contents!");
        return nullptr;
    }

    CanvasImage img = detectCanvas(data, size, file->desc.prop_length);
    if (img.raster == nullptr) {
        QMessageBox::warning(parent, "Preview", "Could not decode this canvas image.");
        free(data);
        return nullptr;
    }

    QImage image = renderMono(img.raster, img.rasterLen, img.w, img.h, img.stride);
    free(data);

    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* imageLabel = new ImageLabel(container);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSourceImage(image);
    layout->addWidget(imageLabel, 1);

    auto* info = new QLabel(
        QStringLiteral("Format: %1    |    Resolution: %2 × %3").arg(img.format).arg(img.w).arg(img.h),
        container);
    info->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    info->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(info);

    return container;
}
