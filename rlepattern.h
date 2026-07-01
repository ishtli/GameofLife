#ifndef RLEPATTERN_H
#define RLEPATTERN_H

#include <QPoint>
#include <QString>
#include <QVector>

class RlePattern
{
public:
    static QVector<QPoint> parse(const QString &rle);
    static QString readName(const QString &rle, const QString &fallbackName);
};

#endif // RLEPATTERN_H
