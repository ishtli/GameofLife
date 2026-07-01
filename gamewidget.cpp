#include "gamewidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <QColor>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(720, 480);
    setMouseTracking(true);
    resetView();
}

int GameWidget::rowCount() const { return rows; }
int GameWidget::columnCount() const { return cols; }
int GameWidget::generation() const { return generationCount; }
int GameWidget::aliveCells() const { return liveCells.size(); }
bool GameWidget::isSimRunning() const { return timerId != -1; }
double GameWidget::zoomLevel() const { return cellSize; }

void GameWidget::SimStart()
{
    if (timerId == -1) {
        timerId = startTimer(interval);
        emit runningChanged(true);
    }
}

void GameWidget::SimStop()
{
    if (timerId != -1) {
        killTimer(timerId);
        timerId = -1;
        emit runningChanged(false);
    }
}

void GameWidget::SimButtonToggle()
{
    isSimRunning() ? SimStop() : SimStart();
}

void GameWidget::stepSimulation()
{
    // 活细胞周围有 2 或 3 个活邻居时继续存活
    // 死细胞周围刚好有 3 个活邻居时复活

    // 记录某个格子周围有多少个活细胞，不包括自身
    QHash<QPoint, int> neighborCounts;

    // 提前预留空间可以减少哈希表扩容次数（最多需要计算的数据数量：活细胞数*8）
    neighborCounts.reserve(liveCells.size() * 8);

    // 遍历当前所有活细胞，每个活细胞对周围8个邻居格子+1
    for (const QPoint &cell : liveCells) {
        // dr为行偏移 -1，0，1，dc为列偏移
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                // dr == 0 且 dc == 0 时表示活细胞自身，跳过
                if (dr == 0 && dc == 0) {
                    continue;
                }

                // 根据当前活细胞坐标和偏移量，得到一个邻居格子的坐标
                // 边界环绕处理
                QPoint neighbor = normalizeCell(QPoint(cell.x() + dc, cell.y() + dr));

                // 如果邻居坐标在画布范围内，就给这个格子的邻居计数加 1
                if (isInside(neighbor)) {
                    neighborCounts[neighbor]++;
                }
            }
        }
    }

    //保存下一代活细胞
    QSet<QPoint> nextGenAliveCells;

    // 提前预留空间
    nextGenAliveCells.reserve(neighborCounts.size());

    // 遍历所有可能有活细胞的格子
    for (auto it = neighborCounts.constBegin(); it != neighborCounts.constEnd(); it++) {
        const bool isAlive = liveCells.contains(it.key());
        const int neighbors = it.value();

        if ((isAlive && (neighbors == 2 || neighbors == 3)) || (!isAlive && neighbors == 3)) {
            nextGenAliveCells.insert(it.key());
        }
    }


    liveCells.swap(nextGenAliveCells);// 替换，更新活细胞集合
    generationCount++;  //代数+1
    update();           //QWidget重新绘制控件
    emitStats();        //更新统计数据
}

void GameWidget::clearGrid()
{
    SimStop();
    liveCells.clear();
    generationCount = 0;
    update();
    emitStats();
}

void GameWidget::setGridSize(int newRows, int newCols)
{
    SimStop();
    rows = std::clamp(newRows, 100, 1000000);
    cols = std::clamp(newCols, 100, 1000000);

    for (auto it = liveCells.begin(); it != liveCells.end();) {
        if (!isInside(*it)) {
            it = liveCells.erase(it);
        } else {
            ++it;
        }
    }

    generationCount = 0;
    update();
    emitStats();
}

void GameWidget::setInterval(int milliseconds)
{
    interval = std::clamp(milliseconds, 10, 1000);
    if (isSimRunning()) {
        SimStop();
        SimStart();
    }
}

void GameWidget::setWrapEdges(bool enabled)
{
    wrapEdges = enabled;
}


void GameWidget::setBrushPattern(const QString &patternName)
{
    brushPatternName = patternName;
}

//计算可显示的逻辑画布范围
QRect GameWidget::visibleCellRange() const
{
    const int left = static_cast<int>(std::floor(offsetX)) - 1;
    const int top = static_cast<int>(std::floor(offsetY)) - 1;
    const int right = static_cast<int>(std::ceil(offsetX + width() / cellSize)) + 1;
    const int bottom = static_cast<int>(std::ceil(offsetY + height() / cellSize)) + 1;
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

//绘制画布
void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    //整体绘制成深灰色
    painter.fillRect(rect(), QColor(36, 36, 36));

    const QRectF canvasRect(
        -offsetX * cellSize,
        -offsetY * cellSize,
        cols * cellSize,
        rows * cellSize
    );
    //逻辑画布区域黑色
    painter.fillRect(canvasRect, Qt::black);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);

    const QRect visible = visibleCellRange(); //计算逻辑画布上的可视范围
    const double drawSize = std::max(0.3, std::ceil(cellSize));

    // 只绘制可视范围内的活细胞
    for (const QPoint &cell : liveCells) {
        if (!visible.contains(cell)) {
            continue;
        }

        const QPointF topLeft = screenTopLeftForCell(cell);
        painter.drawRect(QRectF(topLeft.x(), topLeft.y(), drawSize, drawSize));
    }
}

//设置cell显示大小，传入anchor，确保缩放可以固定中心或是围绕鼠标位置
void GameWidget::setCellSize(double newCellSize, const QPointF &anchor)
{
    newCellSize = std::clamp(newCellSize, 1.0, 80.0);

    //记录旧anchor对应的逻辑坐标位置
    const QPointF before(anchor.x() / cellSize + offsetX, anchor.y() / cellSize + offsetY);
    cellSize = newCellSize;

    // 缩放后重新计算逻辑偏移，让锚点指向的逻辑位置保持不变

    offsetX = before.x() - anchor.x() / cellSize;
    offsetY = before.y() - anchor.y() / cellSize;
    update();
    emit zoomChanged(cellSize);
}

void GameWidget::zoomIn()
{
    setCellSize(cellSize * 1.25, QPointF(width() / 2.0, height() / 2.0));
}

void GameWidget::zoomOut()
{
    setCellSize(cellSize / 1.25, QPointF(width() / 2.0, height() / 2.0));
}

//重置显示：cellsize设为12，显示中心为逻辑画布中心
void GameWidget::resetView()
{
    cellSize = 12.0;
    offsetX = cols / 2.0 - width() / (2.0 * cellSize);
    offsetY = rows / 2.0 - height() / (2.0 * cellSize);
    update();
    emit zoomChanged(cellSize);
}



void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        isCamMoving = true;
        lastCamPosition = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        placePatternAt(pos2Cell(event->position()));
    } else {
        setCellFromMouse(event->position(), event->buttons());
    }
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isCamMoving) {
        const QPoint delta = event->pos() - lastCamPosition;
        // offset是逻辑画布坐标，需要根据cellsize缩放。
        offsetX -= delta.x() / cellSize;
        offsetY -= delta.y() / cellSize;
        lastCamPosition = event->pos();
        update();
        return;
    }

    if (event->buttons() & Qt::RightButton) {
        setCellFromMouse(event->position(), event->buttons());
    }
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        isCamMoving = false;
        unsetCursor();
    }
}

void GameWidget::wheelEvent(QWheelEvent *event)
{
    // 以鼠标所在位置为锚点缩放
    const double factor = event->angleDelta().y() > 0 ? 1.18 : 1.0 / 1.18;
    setCellSize(cellSize * factor, event->position());
}

void GameWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timerId) {
        stepSimulation();
    }
}

QPoint GameWidget::normalizeCell(const QPoint &cell) const
{
    if (!wrapEdges) {
        return cell;
    }

    const int x = (cell.x() % cols + cols) % cols;
    const int y = (cell.y() % rows + rows) % rows;
    // 开启边界环绕时，坐标越界会从另一侧回到画布中。
    return QPoint(x, y);
}

bool GameWidget::isInside(const QPoint &cell) const
{
    return cell.x() >= 0 && cell.x() < cols && cell.y() >= 0 && cell.y() < rows;
}


QPoint GameWidget::pos2Cell(const QPointF &position) const
{
    // 屏幕坐标 -> 逻辑坐标
    const int x = static_cast<int>(std::floor(position.x() / cellSize + offsetX));
    const int y = static_cast<int>(std::floor(position.y() / cellSize + offsetY));
    return QPoint(x, y);
}

//输入cell，逻辑坐标 -> 屏幕坐标
QPointF GameWidget::screenTopLeftForCell(const QPoint &cell) const
{
    // 逻辑坐标 -> 屏幕坐标
    return QPointF((cell.x() - offsetX) * cellSize, (cell.y() - offsetY) * cellSize);
}



void GameWidget::setCellFromMouse(const QPointF &position, Qt::MouseButtons buttons)
{
    const QPoint cell = normalizeCell(pos2Cell(position));
    if (!isInside(cell)) {
        return;
    }

    if (buttons & Qt::LeftButton) {
        setCell(cell, true);
    } else if (buttons & Qt::RightButton) {
        setCell(cell, false);
    }
}

void GameWidget::setCell(const QPoint &cell, bool alive)
{
    if (alive) {
        liveCells.insert(cell);
    } else {
        liveCells.remove(cell);
    }

    update();
    emitStats();
}

QVector<QPoint> GameWidget::patternCells(const QString &patternName) const
{
    if (patternName=="Pencil") {
        return {{0,0}};
    }
    if (patternName == "Glider") {
        return {{1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2}};
    }
    if (patternName == "Blinker") {
        return {{-1, 0}, {0, 0}, {1, 0}};
    }
    if (patternName == "Block") {
        return {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
    }
    if (patternName == "Beehive") {
        return {{1, 0}, {2, 0}, {0, 1}, {3, 1}, {1, 2}, {2, 2}};
    }
    if (patternName == "Boat") {
        return {{0, 0}, {1, 0}, {0, 1}, {2, 1}, {1, 2}};
    }
    if (patternName == "Tub") {
        return {{1, 0}, {0, 1}, {2, 1}, {1, 2}};
    }
    if (patternName == "Beacon") {
        return {{0, 0}, {1, 0}, {0, 1}, {3, 2}, {2, 3}, {3, 3}};
    }
    if (patternName == "Toad") {
        return {{1, 0}, {2, 0}, {3, 0}, {0, 1}, {1, 1}, {2, 1}};
    }
    if (patternName == "Clock") {
        return {{1, 0}, {3, 1}, {0, 2}, {3, 2}, {0, 3}, {2, 4}};
    }
    if (patternName == "Pulsar") {
        return {
            {-4, -6}, {-3, -6}, {-2, -6}, {2, -6}, {3, -6}, {4, -6},
            {-6, -4}, {-1, -4}, {1, -4}, {6, -4},
            {-6, -3}, {-1, -3}, {1, -3}, {6, -3},
            {-6, -2}, {-1, -2}, {1, -2}, {6, -2},
            {-4, -1}, {-3, -1}, {-2, -1}, {2, -1}, {3, -1}, {4, -1},
            {-4, 1}, {-3, 1}, {-2, 1}, {2, 1}, {3, 1}, {4, 1},
            {-6, 2}, {-1, 2}, {1, 2}, {6, 2},
            {-6, 3}, {-1, 3}, {1, 3}, {6, 3},
            {-6, 4}, {-1, 4}, {1, 4}, {6, 4},
            {-4, 6}, {-3, 6}, {-2, 6}, {2, 6}, {3, 6}, {4, 6}
        };
    }
    if (patternName == "Pentadecathlon") {
        return {{-1, -4}, {0, -4}, {-2, -3}, {1, -3}, {-1, -2}, {0, -2},
                {-1, -1}, {0, -1}, {-1, 0}, {0, 0}, {-1, 1}, {0, 1},
                {-2, 2}, {1, 2}, {-1, 3}, {0, 3}};
    }
    if (patternName == "Lightweight Spaceship") {
        return {{1, 0}, {4, 0}, {0, 1}, {0, 2}, {4, 2},
                {0, 3}, {1, 3}, {2, 3}, {3, 3}};
    }
    if (patternName == "Middleweight Spaceship") {
        return {{2, 0}, {0, 1}, {4, 1}, {5, 2}, {0, 3}, {5, 3},
                {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}};
    }
    if (patternName == "Heavyweight Spaceship") {
        return {{2, 0}, {3, 0}, {0, 1}, {5, 1}, {6, 2}, {0, 3}, {6, 3},
                {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}};
    }
    if (patternName == "R-pentomino") {
        return {{1, 0}, {2, 0}, {0, 1}, {1, 1}, {1, 2}};
    }
    if (patternName == "Diehard") {
        return {{6, 0}, {0, 1}, {1, 1}, {1, 2}, {5, 2}, {6, 2}, {7, 2}};
    }
    if (patternName == "Acorn") {
        return {{1, 0}, {3, 1}, {0, 2}, {1, 2}, {4, 2}, {5, 2}, {6, 2}};
    }
    if (patternName == "Simkin Glider Gun") {
        return {
            {0, 0}, {1, 0}, {0, 1}, {1, 1},
            {7, 0}, {8, 0}, {7, 1}, {8, 1},
            {4, 3}, {5, 3}, {4, 4}, {5, 4},
            {22, 9}, {26, 9},
            {21, 10}, {26, 10}, {27, 10},
            {20, 11}, {21, 11}, {27, 11}, {28, 11}, {31, 11}, {32, 11},
            {21, 12}, {22, 12}, {27, 12}, {31, 12}, {32, 12},
            {22, 13}
        };
    }
    if (patternName == "Gosper Glider Gun") {
        return {
            {0, 4}, {1, 4}, {0, 5}, {1, 5},
            {10, 4}, {10, 5}, {10, 6}, {11, 3}, {11, 7}, {12, 2}, {12, 8},
            {13, 2}, {13, 8}, {14, 5}, {15, 3}, {15, 7}, {16, 4}, {16, 5}, {16, 6}, {17, 5},
            {20, 2}, {20, 3}, {20, 4}, {21, 2}, {21, 3}, {21, 4}, {22, 1}, {22, 5},
            {24, 0}, {24, 1}, {24, 5}, {24, 6},
            {34, 2}, {35, 2}, {34, 3}, {35, 3}
        };
    }

    return {{0, 0}};
}

void GameWidget::placePatternAt(const QPoint &anchorCell)
{
    const QPoint anchor = normalizeCell(anchorCell);
    if (!isInside(anchor)) {
        return;
    }

    addPatternCells(patternCells(brushPatternName), anchor);
    generationCount = 0;
    update();
    emitStats();
}

void GameWidget::addPatternCells(const QVector<QPoint> &cells, const QPoint &anchorCell)
{
    for (const QPoint &cell : cells) {
        const QPoint placed = normalizeCell(QPoint(anchorCell.x() + cell.x(), anchorCell.y() + cell.y()));
        if (isInside(placed)) {
            liveCells.insert(placed);
        }
    }
}

void GameWidget::emitStats()
{
    emit statsChanged(generationCount, liveCells.size());
}
