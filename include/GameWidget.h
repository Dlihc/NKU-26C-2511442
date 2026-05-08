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
    enum class GuardMode { Patrol, Chase, Search, Return, Sleep };
    enum class DetectionSource { None, GuardVision, CameraVision, SameTileCapture, SwapTileCapture };

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

    struct KeyItem {
        QString id;
        Vec2 pos;
    };

    struct Door {
        QString id;
        Vec2 pos;
        int w = 1;
        int h = 1;
    };

    struct LevelData {
        std::vector<QString> rows;
        std::vector<Guard> guards;
        std::vector<Camera> cameras;
        std::vector<KeyItem> keys;
        std::vector<Door> doors;
        QString name;
        QString briefing;
    };

    struct MissionStats {
        QString levelName;
        int steps = 0;
        int alertCount = 0;
        int cameraAlertCount = 0;
        int caughtCount = 0;
        int hideCount = 0;
        int noiseCount = 0;
        int retryCount = 0;
        int frames = 0;
        bool tranquilizerUsed = false;
        bool tranquilizerHit = false;
        int score = 0;
        QString grade;
        QString title;
    };

    struct TranquilizerShot {
        Vec2 start{-1, -1};
        Vec2 pos{-1, -1};
        Direction dir = Direction::Down;
        bool active = false;
        int stepCooldown = 0;
        int remainingRange = 0;
        int trailFrames = 0;
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
    bool isDoorCell(const Vec2& pos) const;
    bool isDoorOpen(const Door& door) const;
    const Door* doorAt(const Vec2& pos) const;
    const KeyItem* keyAt(const Vec2& pos) const;
    bool blocksMovement(const Vec2& pos) const;
    bool blocksProjectile(const Vec2& pos) const;
    void collectKeyAtPlayer();
    void resetAttemptCounters();
    void rebuildIntroPoints();
    Vec2 introFocusPoint() const;
    void fireTranquilizer();
    void updateTranquilizer();
    void setAimDirection(Direction dir);
    std::vector<Vec2> aimLine(Direction dir, int range) const;
    void updateGuards();
    void updateCameras();
    void updateDetection();
    void triggerAlert(DetectionSource source, int actorIndex);
    void onPlayerCaught(DetectionSource source, int actorIndex);
    void onPlayerReachedGoal();
    std::vector<Vec2> visibleTilesFrom(const Vec2& watcher, Direction facing, int range, int sideDivisor) const;
    std::vector<Vec2> getGuardVisibleTiles(const Guard& guard) const;
    std::vector<Vec2> getCameraVisibleTiles(const Camera& camera) const;
    bool isTileVisibleFromGuard(const Guard& guard, const Vec2& target) const;
    bool isTileVisibleFromCamera(const Camera& camera, const Vec2& target) const;
    bool canSeePlayer(const Guard& guard) const;
    bool canCameraSeePlayer(const Camera& camera) const;
    bool canSeeFrom(const Vec2& watcher, Direction facing, int range, const Vec2& target) const;
    bool canSeePoint(const Vec2& watcher, Direction facing, int range, const Vec2& target) const;
    bool hasLineOfSight(const Vec2& from, const Vec2& to) const;
    Vec2 dirToVec(Direction dir) const;
    QString alertText() const;
    QString detectionSourceText(DetectionSource source) const;
    QString guardModeText(GuardMode mode) const;
    QString detectionDebugText() const;
    std::optional<Vec2> nextStepToward(const Vec2& start, const Vec2& goal) const;
    std::optional<Vec2> noiseTarget() const;
    std::optional<Vec2> nearestWalkableAround(const Vec2& pos, const Vec2& from) const;
    std::vector<Vec2> searchPattern(const Vec2& pivot) const;
    void applyFacingFromStep(Guard& guard, const Vec2& next);
    Direction cameraFacing(const Camera& camera) const;
    Direction clockwiseDirection(Direction dir) const;
    std::optional<Direction> predictedGuardFacing(const Guard& guard) const;
    std::optional<Direction> nextCameraFacing(const Camera& camera) const;
    bool shouldWarnGuardTurn(const Guard& guard) const;
    bool shouldWarnCameraTurn(const Camera& camera) const;
    QString directionGlyph(Direction d) const;
    void makeNoise();
    QString completedLabelForLevel(int index) const;
    void beginLevelIntro();
    QString formatFrames(int frames) const;
    QString gradeFromScore(int score) const;
    QString titleFromGrade(const QString& grade) const;
    QString routeEvaluation(const MissionStats& stats) const;
    QString alertEvaluation(const MissionStats& stats) const;
    QString itemEvaluation(const MissionStats& stats) const;
    MissionStats makeCurrentMissionStats() const;
    void finishCurrentMission();
    int tranquilizerMaxAmmoForLevel(int levelIndex) const;
    void drawDebugOverlay(QPainter& p, const QRect& viewport);
    void drawRadar(QPainter& p, const QRect& viewport, int camX, int camY);
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
    int m_simulationFrames = 0;
    int m_alertCount = 0;
    int m_cameraAlertCount = 0;
    int m_caughtCount = 0;
    int m_hideCount = 0;
    int m_noiseUseCount = 0;
    int m_retryCount = 0;
    int m_suspiciousFrames = 0;
    int m_respawnInvincibleFrames = 0;
    int m_victoryFrames = 0;
    int m_caughtFlashFrames = 0;
    int m_noiseFrames = 0;
    int m_noiseCooldown = 0;
    int m_cameraAlertFrames = 0;
    int m_slowMotionFrame = 0;
    int m_detectionDebugFrames = 0;
    int m_currentLevel = 0;
    Direction m_playerFacing = Direction::Down;
    bool m_aimingNoise = false;
    bool m_aimingTranquilizer = false;
    bool m_playerHidden = false;
    bool m_playerWon = false;
    bool m_playerCaught = false;
    bool m_gameStarted = false;
    bool m_showMissionResult = false;
    bool m_showFinalResult = false;
    bool m_debugOverlay = false;
    DetectionSource m_lastDetectionSource = DetectionSource::None;
    int m_lastDetectionActor = -1;
    bool m_tranquilizerUsed = false;
    bool m_tranquilizerHit = false;
    int m_menuSelectedLevel = 0;
    int m_introPanFrames = 0;
    int m_introTotalFrames = 0;
    std::vector<MissionStats> m_completedStats;
    std::vector<Camera> m_cameras;
    std::vector<KeyItem> m_keys;
    std::vector<Door> m_doors;
    std::vector<Vec2> m_introPoints;
    QSet<QString> m_collectedKeys;
    TranquilizerShot m_tranquilizerShot;
    int m_tranquilizerAmmo = 1;
    int m_tranquilizerMaxAmmo = 1;
    Direction m_noiseAimDir = Direction::Down;
    Direction m_tranquilizerAimDir = Direction::Down;

    QPixmap m_playerTex, m_guardTex, m_floorTex, m_wallTex, m_boxTex, m_goalTex, m_titleTex;
    AudioManager m_audio;
};
