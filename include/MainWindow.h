#pragma once
#include <QMainWindow>
class GameWidget;
class QLabel;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private:
    GameWidget* m_game = nullptr;
    QLabel* m_statusLabel = nullptr;
};
