#ifndef THEMEDICON_H
#define THEMEDICON_H

#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>

inline QIcon themedSvgIcon(const QString& path, const QPalette& palette) {
    QPixmap pixmap(path);
    if (pixmap.isNull()) return {};

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), palette.color(QPalette::ButtonText));
    return QIcon(pixmap);
}

#endif  // THEMEDICON_H
