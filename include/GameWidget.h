#pragma once
#include "AudioManager.h"
#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <QSet>
#include <vector>
#include <optional>

class QPainter;

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget* parent = nullptr);
    QSize minimumSizeHint() const override;

public slots:
    void restartLevel();
    void selectLevel(int index);
    void returnToMenu();

signals:
    void statusChanged(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void tick();

private:
    enum class TileType { Floor, Wall, Goal, HideBox };
    enum class AlertState { Calm, Suspicious, Alerted, Victory };
    enum class Direction { Up, Right, Down, Left };
    enum class GuardMode { Patrol, Chase, Search, Return };

    struct Vec2 {
        int x = 0;
        int y = 0;
        bool operator==(const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }
    };

    struct Guard {
        Vec2 pos;
        std::vector<Vec2> patrol;
        int patrolIndex = 0;
        Direction facing = Direction::Down;
        int moveCooldown = 0;
        GuardMode mode = GuardMode::Patrol;
        Vec2 lastKnownPlayer{0,0};
        Vec2 searchPivot{0,0};
        int searchFrames = 0;
        int searchIndex = 0;
        Vec2 pendingNoiseTarget{-1,-1};
        int investigateDelayFrames = 0;
    };

    struct Camera {
        Vec2 pos;
        Direction facing = Direction::Down;
        int range = 7;
        bool rotating = false;
        int cycle = 90;
        int phase = 0;
    };

    struct LevelData {
        std::vector<QString> rows;
        std::vector<Guard> guards;
        std::vector<Camera> cameras;
        QString name;
    };

    struct MissionStats {
        QString levelName;
        int steps = 0;
        int alertCount = 0;
        int caughtCount = 0;
        int hideCount = 0;
        int frames = 0;
        int score = 0;
        QString grade;
        QString title;
    };

    static constexpr int kTileSize = 32;

    void buildLevels();
    void loadLevel(int index);
    void updateStatusText();
    void tryMove(Direction dir, bool fromHeld = false);
    void handleHeldMovement();
    bool canWalkTo(const Vec2& pos) const;
    bool isInsideMap(const Vec2& pos) const;
    TileType tileAt(const Vec2& pos) const;
    void updateGuards();
    void updateCameras();
    void updateDetection();
    void onPlayerCaught();
    void onPlayerReachedGoal();
    bool canSeePlayer(const Guard& guard) const;
    bool canCameraSeePlayer(const Camera& camera) const;
    bool canSeeFrom(const Vec2& watcher, Direction facing, int range, const Vec2& target) const;
    bool hasLineOfSight(const Vec2& from, const Vec2& to) const;
    Vec2 dirToVec(Direction dir) const;
    QString alertText() const;
    std::optional<Vec2> nextStepToward(const Vec2& start, const Vec2& goal) const;
    std::optional<Vec2> noiseTarget() const;
    std::optional<Vec2> nearestWalkableAround(const Vec2& pos, const Vec2& from) const;
    std::vector<Vec2> searchPattern(const Vec2& pivot) const;
    void applyFacingFromStep(Guard& guard, const Vec2& next);
    Direction cameraFacing(const Camera& camera) const;
    QString directionGlyph(Direction d) const;
    void makeNoise();
    QString completedLabelForLevel(int index) const;
    void beginLevelIntro();
    QString formatFrames(int frames) const;
    QString gradeFromScore(int score) const;
    QString titleFromGrade(const QString& grade) const;
    MissionStats makeCurrentMissionStats() const;
    void finishCurrentMission();
    void drawMissionResult(QPainter& p);
    void drawFinalResult(QPainter& p);

    std::vector<LevelData> m_levels;
    std::vector<std::vector<TileType>> m_map;
    Vec2 m_player;
    Vec2 m_spawn;
    Vec2 m_goal;
    Vec2 m_playerTickStart;
    Vec2 m_noisePos{-1, -1};
    std::vector<Guard> m_guards;
    AlertState m_alertState = AlertState::Calm;
    QSet<int> m_pressedKeys;
    QTimer m_timer;
    int m_playerMoveCooldown = 0;
    int m_stepCounter = 0;
    int m_levelFrames = 0;
    int m_alertCount = 0;
    int m_caughtCount = 0;
    int m_hideCount = 0;
    int m_suspiciousFrames = 0;
    int m_respawnInvincibleFrames = 0;
    int m_victoryFrames = 0;
    int m_caughtFlashFrames = 0;
    int m_noiseFrames = 0;
    int m_noiseCooldown = 0;
    int m_cameraAlertFrames = 0;
    int m_currentLevel = 0;
    Direction m_playerFacing = Direction::Down;
    bool m_aimingNoise = false;
    bool m_playerHidden = false;
    bool m_playerWon = false;
    bool m_playerCaught = false;
    bool m_gameStarted = false;
    bool m_showMissionResult = false;
    bool m_showFinalResult = false;
    int m_menuSelectedLevel = 0;
    int m_introPanFrames = 0;
    std::vector<MissionStats> m_completedStats;
    std::vector<Camera> m_cameras;

    QPixmap m_playerTex, m_guardTex, m_floorTex, m_wallTex, m_boxTex, m_goalTex, m_titleTex;
    AudioManager m_audio;
};
