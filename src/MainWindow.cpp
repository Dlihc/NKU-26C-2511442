#include "MainWindow.h"
#include "GameWidget.h"

#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("NKU-26C-MGS-tribute - Qt 6.11.0");

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_game = new GameWidget(this);
    layout->addWidget(m_game, 1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setMinimumHeight(28);
    m_statusLabel->setStyleSheet("background:#101610; color:#b8ffb8; padding:6px; border:1px solid #335533;");
    layout->addWidget(m_statusLabel);

    setCentralWidget(central);
    resize(1520, 980);
    setStyleSheet("QMainWindow{background:#0c0f0c;} QLabel{font-family:'Microsoft YaHei';}");

    connect(m_game, &GameWidget::statusChanged, m_statusLabel, &QLabel::setText);

    auto* gameMenu = menuBar()->addMenu(QString::fromUtf8(u8"游戏"));
    auto* restartAction = gameMenu->addAction(QString::fromUtf8(u8"重新开始"));
    restartAction->setShortcut(QKeySequence("R"));
    connect(restartAction, &QAction::triggered, m_game, &GameWidget::restartLevel);

    auto* menuAction = gameMenu->addAction(QString::fromUtf8(u8"返回选关"));
    menuAction->setShortcut(QKeySequence("Esc"));
    connect(menuAction, &QAction::triggered, m_game, &GameWidget::returnToMenu);

    auto* levelMenu = gameMenu->addMenu(QString::fromUtf8(u8"选择关卡"));
    for (int i = 0; i < 6; ++i) {
        auto* act = levelMenu->addAction(QString::fromUtf8(u8"第 %1 关").arg(i + 1));
        connect(act, &QAction::triggered, this, [this, i]() { m_game->selectLevel(i); });
    }

    statusBar()->showMessage("NKU-26C-MGS-tribute / Qt 6.11.0 / C++17 / Widgets + Multimedia");
}
