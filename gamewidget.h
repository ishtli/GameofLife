#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QHash>
#include <QMap>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);

    int rowCount() const;
    int columnCount() const;
    int generation() const;
    int aliveCells() const;
    bool isSimRunning() const;
    double zoomLevel() const;
    void setExternalPattern(const QMap<QString, QVector<QPoint>> &patterns);

public slots:

    void SimStart();
    void SimStop();
    void SimButtonToggle();

    void stepSimulation();

    void clearGrid();
    void setGridSize(int newRows, int newCols);
    void setInterval(int milliseconds);
    void setWrapEdges(bool enabled);

    void setBrushPattern(const QString &patternName);
    void zoomIn();
    void zoomOut();
    void resetView();

signals:
    void statsChanged(int generation, int aliveCells);
    void runningChanged(bool running);
    void zoomChanged(double zoomLevel);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    int rows = 10000;              // 逻辑画布行数
    int cols = 10000;              // 逻辑画布列数
    int timerId = -1;              // 定时器 ID，-1 表示当前未运行
    int interval = 80;             // 每次演化之间的时间间隔，单位毫秒
    int generationCount = 0;       // 当前已经演化的代数
    bool wrapEdges = false;        // 是否开启边界环绕
    bool isCamMoving = false;        // 当前是否正在拖动画布
    QPoint lastCamPosition;        // 上一次拖动画布时的鼠标位置
    double cellSize = 12.0;        // 单个细胞在屏幕上的显示尺寸，单位是像素
    double offsetX = 0.0;          // 视窗左上角在逻辑画布上的坐标（列坐标）
    double offsetY = 0.0;          // 视窗左上角在逻辑画布上的坐标（行坐标）
    QString brushPatternName = "Gosper Glider Gun";

    QSet<QPoint> liveCells;
    QMap<QString, QVector<QPoint>> externalPatterns;

    QPoint normalizeCell(const QPoint &cell) const;
    bool isInside(const QPoint &cell) const;
    QPoint pos2Cell(const QPointF &position) const;
    QPointF screenTopLeftForCell(const QPoint &cell) const;
    QRect visibleCellRange() const;
    void setCellFromMouse(const QPointF &position, Qt::MouseButtons buttons);
    void setCell(const QPoint &cell, bool alive);
    QVector<QPoint> patternCells(const QString &patternName) const;
    void placePatternAt(const QPoint &anchorCell);
    void addPatternCells(const QVector<QPoint> &cells, const QPoint &anchorCell);
    void fitPatternInView();
    void setCellSize(double newCellSize, const QPointF &anchor);
    void emitStats();
};

#endif // GAMEWIDGET_H
