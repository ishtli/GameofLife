#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gamewidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
        setupInterface();
        connectControls();
        updateStats(ui->gameWidget->generation(), ui->gameWidget->aliveCells());

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupInterface()
{
    setWindowTitle("Conway Game of Life");
    resize(1100, 700);


    ui->speedCombo->setItemData(0, 350);
    ui->speedCombo->setItemData(1, 120);
    ui->speedCombo->setItemData(2, 45);
    ui->speedCombo->setItemData(3, 10);
    ui->speedCombo->setCurrentIndex(1);

    ui->rowsSpin->setValue(ui->gameWidget->rowCount());
    ui->colsSpin->setValue(ui->gameWidget->columnCount());

    statusBar()->showMessage("选择预设后，左键点击画布即可放置图案");
}

void MainWindow::connectControls()
{

    connect(ui->runButton, &QPushButton::clicked, ui->gameWidget, &GameWidget::SimButtonToggle);
    connect(ui->stepButton, &QPushButton::clicked, ui->gameWidget, &GameWidget::stepSimulation);
    connect(ui->clearButton, &QPushButton::clicked, ui->gameWidget, &GameWidget::clearGrid);
    connect(ui->wrapCheck, &QCheckBox::toggled, ui->gameWidget, &GameWidget::setWrapEdges);
    connect(ui->zoomInButton, &QPushButton::clicked, ui->gameWidget, &GameWidget::zoomIn);
    connect(ui->zoomOutButton, &QPushButton::clicked, ui->gameWidget, &GameWidget::zoomOut);
    connect(ui->resetViewButton, &QPushButton::clicked, ui->gameWidget, &GameWidget::resetView);

    connect(ui->speedCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        ui->gameWidget->setInterval(ui->speedCombo->currentData().toInt());
    });

    connect(ui->rowsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this]() {
        ui->gameWidget->setGridSize(ui->rowsSpin->value(), ui->colsSpin->value());
    });
    connect(ui->colsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this]() {
        ui->gameWidget->setGridSize(ui->rowsSpin->value(), ui->colsSpin->value());
    });

    connect(ui->patternCombo, &QComboBox::currentTextChanged, ui->gameWidget, &GameWidget::setBrushPattern);

    connect(ui->gameWidget, &GameWidget::statsChanged, this, &MainWindow::updateStats);

    connect(ui->gameWidget, &GameWidget::zoomChanged, this, [this](double zoomLevel) {
        ui->zoomLabel->setText(QString("缩放：每格 %1 px").arg(zoomLevel, 0, 'f', 1));
    });
    connect(ui->gameWidget, &GameWidget::runningChanged, this, [this](bool running) {
        ui->runButton->setText(running ? "暂停" : "开始");
        ui->stepButton->setEnabled(!running);
    });

    ui->gameWidget->setBrushPattern(ui->patternCombo->currentText());
    ui->gameWidget->setInterval(ui->speedCombo->currentData().toInt());
}

void MainWindow::updateStats(int generation, int aliveCells)
{
    ui->generationLabel->setText(QString("代数：%1").arg(generation));
    ui->aliveLabel->setText(QString("存活细胞：%1").arg(aliveCells));
    ui->zoomLabel->setText(QString("缩放：每格 %1 px").arg(ui->gameWidget->zoomLevel(), 0, 'f', 1));
}
