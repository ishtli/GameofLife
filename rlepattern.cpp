#include "rlepattern.h"

QVector<QPoint> RlePattern::parse(const QString &rle)
{
    QVector<QPoint> cells;
    QString body;

    // RLE 前面可能有注释和头信息，这里只保留真正的图案内容。
    for (QString line : rle.split('\n')) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('x', Qt::CaseInsensitive)) {
            continue;
        }
        body += line;
    }

    int x = 0;
    int y = 0;
    int run = 0;

    for (const QChar &ch : body) {
        if (ch.isDigit()) {
            run = run * 10 + ch.digitValue();
            continue;
        }
        if (ch.isSpace()) {
            continue;
        }

        const int count = run == 0 ? 1 : run;
        run = 0;

        if (ch == 'o' || ch == 'O') {
            for (int i = 0; i < count; ++i) {
                cells.append(QPoint(x + i, y));
            }
            x += count;
        } else if (ch == 'b' || ch == 'B') {
            x += count;
        } else if (ch == '$') {
            y += count;
            x = 0;
        } else if (ch == '!') {
            break;
        }
    }

    return cells;
}

QString RlePattern::readName(const QString &rle, const QString &fallbackName)
{
    for (QString line : rle.split('\n')) {
        line = line.trimmed();
        if (line.startsWith("#N")) {
            const QString name = line.mid(2).trimmed();
            if (!name.isEmpty()) {
                return name;
            }
        }
    }

    return fallbackName;
}
