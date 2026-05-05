#include "GameWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFont>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <queue>
#include <array>
#include <algorithm>
#include <tuple>

GameWidget::GameWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    m_playerTex.load(":/assets/images/player.png");
    m_guardTex.load(":/assets/images/guard.png");
    m_floorTex.load(":/assets/images/floor.png");
    m_wallTex.load(":/assets/images/wall.png");
    m_boxTex.load(":/assets/images/box.png");
    m_goalTex.load(":/assets/images/goal.png");
    m_titleTex.load(":/assets/images/title.png");

    buildLevels();
    loadLevel(0);
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.start(16);
    m_audio.playBgm();
    updateStatusText();
}

QSize GameWidget::minimumSizeHint() const { return QSize(1440, 900); }

void GameWidget::restartLevel() {
    loadLevel(m_currentLevel);
    beginLevelIntro();
    update();
    updateStatusText();
}

void GameWidget::selectLevel(int index) {
    m_menuSelectedLevel = std::clamp(index, 0, (int)m_levels.size() - 1);
    loadLevel(m_menuSelectedLevel);
    m_gameStarted = false;
    m_playerCaught = false;
    m_playerWon = false;
    update();
    updateStatusText();
}

void GameWidget::returnToMenu() {
    m_gameStarted = false;
    m_playerCaught = false;
    m_playerWon = false;
    m_alertState = AlertState::Calm;
    m_pressedKeys.clear();
    loadLevel(m_menuSelectedLevel);
    update();
    updateStatusText();
}

void GameWidget::beginLevelIntro() {
    m_introPanFrames = 96;
    m_audio.playLevelStart();
}

void GameWidget::buildLevels() {
    m_levels.clear();

    auto blank = [](int w, int h) {
        return std::vector<QString>(h, QString(w, '#'));
    };
    auto carveRect = [](std::vector<QString>& rows, int x, int y, int w, int h, QChar fill = '.') {
        for (int yy = y; yy < y + h; ++yy)
            for (int xx = x; xx < x + w; ++xx)
                rows[yy][xx] = fill;
    };
    auto carveH = [](std::vector<QString>& rows, int x1, int x2, int y, QChar fill = '.') {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) rows[y][x] = fill;
    };
    auto carveV = [](std::vector<QString>& rows, int y1, int y2, int x, QChar fill = '.') {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) rows[y][x] = fill;
    };
    auto place = [](std::vector<QString>& rows, int x, int y, QChar c) { rows[y][x] = c; };
    auto G = [](int x, int y, std::initializer_list<Vec2> pts, Direction dir, int cd) {
        Guard guard;
        guard.pos = {x, y};
        guard.patrol = pts;
        guard.facing = dir;
        guard.moveCooldown = cd;
        return guard;
    };
    auto parseDir = [](const QString& s) {
        if (s == "Up") return Direction::Up;
        if (s == "Left") return Direction::Left;
        if (s == "Down") return Direction::Down;
        return Direction::Right;
    };
    auto loadSceneLevel = [&](const QString& resPath) {
        QFile file(resPath);
        if (!file.open(QIODevice::ReadOnly)) return;
        const auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) return;
        const auto obj = doc.object();
        LevelData lv;
        lv.name = obj.value("name").toString();
        const int w = obj.value("width").toInt();
        const int h = obj.value("height").toInt();
        lv.rows.assign(h, QString(w, '.'));
        for (int x = 0; x < w; ++x) {
            lv.rows[0][x] = '#';
            lv.rows[h - 1][x] = '#';
        }
        for (int y = 0; y < h; ++y) {
            lv.rows[y][0] = '#';
            lv.rows[y][w - 1] = '#';
        }
        auto applyRects = [&](const QJsonArray& arr, QChar c) {
            for (const auto& v : arr) {
                const auto a = v.toArray();
                if (a.size() < 4) continue;
                carveRect(lv.rows, a[0].toInt(), a[1].toInt(), a[2].toInt(), a[3].toInt(), c);
            }
        };
        applyRects(obj.value("walls").toArray(), '#');
        applyRects(obj.value("cuts").toArray(), '.');
        const auto spawn = obj.value("spawn").toArray();
        const auto goal = obj.value("goal").toArray();
        if (spawn.size() >= 2) place(lv.rows, spawn[0].toInt(), spawn[1].toInt(), 'S');
        if (goal.size() >= 2) place(lv.rows, goal[0].toInt(), goal[1].toInt(), 'G');
        for (const auto& v : obj.value("boxes").toArray()) {
            const auto a = v.toArray();
            if (a.size() >= 2) place(lv.rows, a[0].toInt(), a[1].toInt(), 'B');
        }
        for (const auto& gv : obj.value("guards").toArray()) {
            const auto go = gv.toObject();
            Guard g;
            const auto pos = go.value("pos").toArray();
            if (pos.size() >= 2) g.pos = {pos[0].toInt(), pos[1].toInt()};
            g.facing = parseDir(go.value("facing").toString());
            g.moveCooldown = go.value("cooldown").toInt(24);
            for (const auto& pv : go.value("patrol").toArray()) {
                const auto pa = pv.toArray();
                if (pa.size() >= 2) g.patrol.push_back({pa[0].toInt(), pa[1].toInt()});
            }
            if (!g.patrol.empty()) lv.guards.push_back(g);
        }
        m_levels.push_back(lv);
    };

    // Level 1: scene-authored sample level (perimeter blocked, goal room enclosed)
    loadSceneLevel(":/assets/levels/level1.json");

    // Level 2: scene-authored warehouse with denser chokepoints
    loadSceneLevel(":/assets/levels/level2.json");

    // Level 3: scene-authored barracks with layered patrol zones
    loadSceneLevel(":/assets/levels/level3.json");

    // Level 4: scene-authored stronghold with denser patrol webs
    loadSceneLevel(":/assets/levels/level4.json");

    // Level 5: scene-authored final sector with heavier pressure
    loadSceneLevel(":/assets/levels/level5.json");
}

void GameWidget::loadLevel(int index) {
    m_currentLevel = std::clamp(index, 0, (int)m_levels.size() - 1);
    const auto& data = m_levels[m_currentLevel];
    m_map.assign(data.rows.size(), std::vector<TileType>(data.rows[0].size(), TileType::Floor));
    m_guards = data.guards;

    for (int y = 0; y < (int)data.rows.size(); ++y) {
        for (int x = 0; x < data.rows[y].size(); ++x) {
            const QChar c = data.rows[y][x];
            if (c == '#') m_map[y][x] = TileType::Wall;
            else if (c == 'G') { m_map[y][x] = TileType::Goal; m_goal = {x, y}; }
            else if (c == 'B') m_map[y][x] = TileType::HideBox;
            else if (c == 'S') {
                m_map[y][x] = TileType::Floor;
                m_spawn = {x, y};
                m_player = m_spawn;
            } else m_map[y][x] = TileType::Floor;
        }
    }

    for (auto& g : m_guards) {
        g.mode = GuardMode::Patrol;
        g.searchFrames = 0;
        g.searchIndex = 0;
        g.lastKnownPlayer = m_player;
        g.searchPivot = g.pos;
    }

    m_alertState = AlertState::Calm;
    m_pressedKeys.clear();
    m_playerMoveCooldown = 0;
    m_stepCounter = 0;
    m_levelFrames = 0;
    m_alertCount = 0;
    m_caughtCount = 0;
    m_hideCount = 0;
    m_suspiciousFrames = 0;
    m_respawnInvincibleFrames = 120;
    m_victoryFrames = 0;
    m_caughtFlashFrames = 0;
    m_playerHidden = false;
    m_playerWon = false;
    m_playerCaught = false;
    m_showMissionResult = false;
    m_showFinalResult = false;
    m_playerTickStart = m_player;
    m_introPanFrames = 0;
}

void GameWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(4, 10, 8));

    const QRect viewport(24, 120, width() - 48, height() - 180);
    const int mapWpx = (int)m_map[0].size() * kTileSize;
    const int mapHpx = (int)m_map.size() * kTileSize;
    double focusX = m_player.x;
    double focusY = m_player.y;
    if (m_introPanFrames > 0) {
        const int holdFrames = 24;
        const int moveFrames = 72;
        if (m_introPanFrames > moveFrames) {
            focusX = m_goal.x;
            focusY = m_goal.y;
        } else {
            const double t = 1.0 - (double)m_introPanFrames / (double)moveFrames;
            const double ease = t * t * (3.0 - 2.0 * t);
            focusX = m_goal.x * (1.0 - ease) + m_player.x * ease;
            focusY = m_goal.y * (1.0 - ease) + m_player.y * ease;
        }
    }
    const int camX = std::clamp((int)(focusX * kTileSize - viewport.width() / 2), 0, std::max(0, mapWpx - viewport.width()));
    const int camY = std::clamp((int)(focusY * kTileSize - viewport.height() / 2), 0, std::max(0, mapHpx - viewport.height()));

    p.setFont(QFont("Microsoft YaHei", 10));
    p.setPen(QColor(200, 235, 200));
    p.drawText(QRect(16, 18, width() - 32, 24), Qt::AlignLeft | Qt::AlignVCenter,
               "方向键/WASD 移动 | 空格躲藏 | 1-5 选关 | Enter 开始 | Esc 返回选关 | R 重开当前关");
    p.drawText(QRect(16, 42, width() - 32, 24), Qt::AlignLeft | Qt::AlignVCenter,
               QString("当前关卡: %1 / %2 - %3").arg(m_currentLevel + 1).arg((int)m_levels.size()).arg(m_levels[m_currentLevel].name));

    if (!m_gameStarted) {
        p.fillRect(rect(), QColor(0, 0, 0, 190));
        p.setPen(QColor(220, 255, 220));
        p.setFont(QFont("Microsoft YaHei", 12));
        p.drawText(QRect(0, 240, width(), 28), Qt::AlignCenter,
                   QString("当前选择关卡: %1 / %2 - %3").arg(m_menuSelectedLevel + 1).arg((int)m_levels.size()).arg(m_levels[m_menuSelectedLevel].name));
        p.drawText(QRect(0, 272, width(), 28), Qt::AlignCenter, "按 1-5 选关，按 Enter 开始，按 Esc 回到这里");
        return;
    }

    p.setClipRect(viewport);

    // 大地图版本只绘制当前镜头范围内的格子，避免 100x70 以上地图拖慢帧率。
    const int visibleStartX = std::max(0, camX / kTileSize - 1);
    const int visibleEndX = std::min((int)m_map[0].size() - 1, (camX + viewport.width()) / kTileSize + 1);
    const int visibleStartY = std::max(0, camY / kTileSize - 1);
    const int visibleEndY = std::min((int)m_map.size() - 1, (camY + viewport.height()) / kTileSize + 1);

    for (int y = visibleStartY; y <= visibleEndY; ++y) {
        for (int x = visibleStartX; x <= visibleEndX; ++x) {
            QRect tileRect(viewport.left() + x * kTileSize - camX,
                           viewport.top() + y * kTileSize - camY,
                           kTileSize, kTileSize);
            p.drawPixmap(tileRect, m_floorTex);
            switch (m_map[y][x]) {
                case TileType::Wall: p.drawPixmap(tileRect, m_wallTex); break;
                case TileType::Goal: p.drawPixmap(tileRect, m_goalTex); break;
                case TileType::HideBox: p.drawPixmap(tileRect, m_boxTex); break;
                default: break;
            }
        }
    }

    p.setPen(Qt::NoPen);
    for (const auto& guard : m_guards) {
        QColor coneColor = QColor(245, 235, 120, 38);
        if (guard.mode == GuardMode::Chase) coneColor = QColor(255, 80, 80, 62);
        else if (guard.mode == GuardMode::Search) coneColor = QColor(255, 170, 80, 48);
        p.setBrush(coneColor);
        const Vec2 f = dirToVec(guard.facing);
        for (int i = 1; i <= 7; ++i) {
            const Vec2 center{guard.pos.x + f.x * i, guard.pos.y + f.y * i};
            for (int side = -i / 2; side <= i / 2; ++side) {
                Vec2 cell = center;
                if (f.x != 0) cell.y += side;
                if (f.y != 0) cell.x += side;
                if (!isInsideMap(cell) || tileAt(cell) == TileType::Wall) continue;
                if (!hasLineOfSight(guard.pos, cell)) continue;
                QRect r(viewport.left() + cell.x * kTileSize - camX,
                        viewport.top() + cell.y * kTileSize - camY,
                        kTileSize, kTileSize);
                p.drawRect(r);
            }
        }
    }

    for (const auto& guard : m_guards) {
        QRect r(viewport.left() + guard.pos.x * kTileSize - camX,
                viewport.top() + guard.pos.y * kTileSize - camY,
                kTileSize, kTileSize);
        p.drawPixmap(r, m_guardTex);
        p.setPen(Qt::white);
        p.drawText(r, Qt::AlignCenter, directionGlyph(guard.facing));
        if (guard.mode == GuardMode::Chase) {
            p.setPen(QColor(255, 120, 120));
            p.drawText(QRect(r.left(), r.top() - 18, kTileSize + 8, 18), Qt::AlignCenter, "!");
        } else if (guard.mode == GuardMode::Search) {
            p.setPen(QColor(255, 190, 120));
            p.drawText(QRect(r.left(), r.top() - 18, kTileSize + 8, 18), Qt::AlignCenter, "?");
        }
    }

    QRect pr(viewport.left() + m_player.x * kTileSize - camX,
             viewport.top() + m_player.y * kTileSize - camY,
             kTileSize, kTileSize);
    if (!m_playerHidden || m_alertState == AlertState::Alerted) p.drawPixmap(pr, m_playerTex);
    else {
        p.setPen(QColor(180, 255, 180));
        p.drawText(pr, Qt::AlignCenter, "...");
    }

    if (m_playerCaught) {
        p.fillRect(viewport, QColor(255, 0, 0, 35));
        p.setPen(QColor(255, 210, 210));
        p.setFont(QFont("Consolas", 24, QFont::Bold));
        p.drawText(QRect(viewport.left(), viewport.center().y() - 20, viewport.width(), 40), Qt::AlignCenter, "ALERT !");
    }
    p.setClipping(false);

    if (m_showFinalResult) {
        drawFinalResult(p);
        return;
    }
    if (m_showMissionResult) {
        drawMissionResult(p);
        return;
    }

    p.setPen(QColor(180, 255, 180));
    p.setFont(QFont("Microsoft YaHei", 10));
    p.drawText(QRect(24, height() - 48, width() - 48, 24), Qt::AlignLeft,
               QString("状态: %1    步数: %2    躲藏: %3    守卫数: %4")
               .arg(alertText()).arg(m_stepCounter).arg(m_playerHidden ? "是" : "否").arg((int)m_guards.size()));
}

void GameWidget::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;
    if (!m_gameStarted) {
        if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_5) {
            selectLevel(event->key() - Qt::Key_1);
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            m_completedStats.clear();
            loadLevel(m_menuSelectedLevel);
            m_gameStarted = true;
            m_respawnInvincibleFrames = 120;
            beginLevelIntro();
            update();
        }
        return;
    }
    if (m_showMissionResult || m_showFinalResult) {
        if (event->key() == Qt::Key_Escape) {
            returnToMenu();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (m_showFinalResult) {
                returnToMenu();
            } else if (m_currentLevel + 1 >= (int)m_levels.size()) {
                m_showMissionResult = false;
                m_showFinalResult = true;
                updateStatusText();
                update();
            } else {
                loadLevel(m_currentLevel + 1);
                m_gameStarted = true;
                beginLevelIntro();
                updateStatusText();
                update();
            }
            return;
        }
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        returnToMenu();
        return;
    }
    if (m_introPanFrames > 0) return;
    if (m_pressedKeys.contains(event->key())) return;
    m_pressedKeys.insert(event->key());

    if (event->key() == Qt::Key_Space && !m_playerWon && !m_playerCaught) {
        if (tileAt(m_player) == TileType::HideBox) {
            bool threatened = false;
            for (const auto& guard : m_guards) {
                if (guard.mode == GuardMode::Chase || canSeePlayer(guard)) {
                    threatened = true;
                    break;
                }
            }
            if (m_playerHidden) {
                m_playerHidden = false;
            } else if (!threatened) {
                m_playerHidden = true;
                ++m_hideCount;
                for (auto& guard : m_guards) {
                    if (guard.mode == GuardMode::Chase) {
                        guard.mode = GuardMode::Search;
                        guard.searchPivot = m_player;
                        guard.lastKnownPlayer = m_player;
                        guard.searchFrames = 18;
                        guard.searchIndex = 0;
                    }
                }
            }
            updateStatusText();
            update();
        }
        return;
    }

    if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up) tryMove(Direction::Up, false);
    else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) tryMove(Direction::Right, false);
    else if (event->key() == Qt::Key_S || event->key() == Qt::Key_Down) tryMove(Direction::Down, false);
    else if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left) tryMove(Direction::Left, false);
}

void GameWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;
    m_pressedKeys.remove(event->key());
}

void GameWidget::tick() {
    if (!m_gameStarted) { update(); return; }
    m_playerTickStart = m_player;
    if (m_introPanFrames > 0) { --m_introPanFrames; updateStatusText(); update(); return; }
    if (m_showMissionResult || m_showFinalResult) { updateStatusText(); update(); return; }
    if (m_playerMoveCooldown > 0) --m_playerMoveCooldown;
    if (m_respawnInvincibleFrames > 0) --m_respawnInvincibleFrames;

    if (m_playerCaught) {
        if (m_caughtFlashFrames > 0) --m_caughtFlashFrames;
        if (m_caughtFlashFrames == 0) {
            m_player = m_spawn;
            m_playerHidden = false;
            m_playerCaught = false;
            m_respawnInvincibleFrames = 120;
            for (auto& g : m_guards) {
                g.mode = GuardMode::Patrol;
                g.searchFrames = 0;
            }
        }
        updateStatusText();
        update();
        return;
    }

    if (m_playerWon) { updateStatusText(); update(); return; }

    ++m_levelFrames;
    handleHeldMovement();
    updateGuards();
    updateDetection();
    updateStatusText();
    update();
}

void GameWidget::handleHeldMovement() {
    if (m_playerMoveCooldown > 0 || m_playerCaught || m_playerWon) return;
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up)) tryMove(Direction::Up, true);
    else if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right)) tryMove(Direction::Right, true);
    else if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down)) tryMove(Direction::Down, true);
    else if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left)) tryMove(Direction::Left, true);
}

void GameWidget::tryMove(Direction dir, bool fromHeld) {
    if (m_playerHidden || m_playerMoveCooldown > 0 || m_playerWon || m_playerCaught) return;
    Vec2 next = m_player;
    const Vec2 d = dirToVec(dir);
    next.x += d.x; next.y += d.y;
    if (canWalkTo(next)) {
        m_player = next;
        ++m_stepCounter;
        m_audio.playStep();
        if (tileAt(m_player) == TileType::Goal) onPlayerReachedGoal();
    }
    m_playerMoveCooldown = fromHeld ? 3 : 6;
}

bool GameWidget::canWalkTo(const Vec2& pos) const {
    return isInsideMap(pos) && tileAt(pos) != TileType::Wall;
}

bool GameWidget::isInsideMap(const Vec2& pos) const {
    return pos.y >= 0 && pos.y < (int)m_map.size() && pos.x >= 0 && pos.x < (int)m_map[0].size();
}

GameWidget::TileType GameWidget::tileAt(const Vec2& pos) const {
    return m_map[pos.y][pos.x];
}

void GameWidget::updateGuards() {
    for (auto& guard : m_guards) {
        const int directDist = std::abs(guard.pos.x - m_player.x) + std::abs(guard.pos.y - m_player.y);
        const bool seesPlayer = canSeePlayer(guard);

        // 玩家成功进入纸箱后，守卫不应长期保持 Chase 红色视野。
        // 这里把追击态降级为短搜索态，搜索结束后自然回归巡逻，避免 AI 卡在红色警戒。
        if (m_playerHidden && guard.mode == GuardMode::Chase) {
            guard.mode = GuardMode::Search;
            guard.searchPivot = m_player;
            guard.lastKnownPlayer = m_player;
            guard.searchFrames = std::min(guard.searchFrames, 18);
            guard.searchIndex = 0;
        }

        if (seesPlayer) {
            guard.mode = GuardMode::Chase;
            guard.lastKnownPlayer = m_player;
            guard.searchPivot = m_player;
            guard.searchFrames = 36;
        } else if (guard.mode == GuardMode::Chase) {
            if (guard.searchFrames > 0) --guard.searchFrames;
            if (guard.searchFrames == 0) {
                guard.mode = GuardMode::Search;
                guard.searchPivot = guard.lastKnownPlayer;
                guard.searchFrames = 24;
                guard.searchIndex = 0;
            }
        } else if (guard.mode == GuardMode::Search) {
            if (guard.searchFrames > 0) --guard.searchFrames;
            if (guard.searchFrames == 0) guard.mode = GuardMode::Return;
        }

        if (!m_playerHidden && directDist == 0 && m_respawnInvincibleFrames == 0) {
            guard.pos = m_player;
            onPlayerCaught();
            return;
        }

        const Vec2 guardBeforeMove = guard.pos;
        if (guard.moveCooldown > 0) {
            --guard.moveCooldown;
            continue;
        }

        std::optional<Vec2> next;
        switch (guard.mode) {
            case GuardMode::Patrol: {
                Vec2 target = guard.patrol[guard.patrolIndex];
                if (guard.pos == target) {
                    guard.patrolIndex = (guard.patrolIndex + 1) % guard.patrol.size();
                    target = guard.patrol[guard.patrolIndex];
                }
                next = nextStepToward(guard.pos, target);
                break;
            }
            case GuardMode::Chase:
                next = nextStepToward(guard.pos, guard.lastKnownPlayer);
                break;
            case GuardMode::Search: {
                auto pattern = searchPattern(guard.searchPivot);
                if (!pattern.empty()) {
                    Vec2 target = pattern[guard.searchIndex % pattern.size()];
                    if (guard.pos == target) {
                        ++guard.searchIndex;
                        target = pattern[guard.searchIndex % pattern.size()];
                    }
                    next = nextStepToward(guard.pos, target);
                }
                break;
            }
            case GuardMode::Return: {
                Vec2 target = guard.patrol[guard.patrolIndex];
                next = nextStepToward(guard.pos, target);
                if (guard.pos == target) guard.mode = GuardMode::Patrol;
                break;
            }
        }

        if (next && tileAt(*next) != TileType::Wall && tileAt(*next) != TileType::HideBox) {
            applyFacingFromStep(guard, *next);
            guard.pos = *next;
        } else {
            if (guard.mode == GuardMode::Patrol && !guard.patrol.empty())
                guard.patrolIndex = (guard.patrolIndex + 1) % guard.patrol.size();
            else if (guard.mode == GuardMode::Return)
                guard.mode = GuardMode::Patrol;
            else if (guard.mode == GuardMode::Search)
                ++guard.searchIndex;
        }

        guard.moveCooldown = (guard.mode == GuardMode::Chase ? 6 : 12);

        if (!m_playerHidden && m_respawnInvincibleFrames == 0) {
            const bool crossed = (guardBeforeMove == m_player && guard.pos == m_playerTickStart) ||
                                 (guardBeforeMove == m_playerTickStart && guard.pos == m_player);
            if (guard.pos == m_player || crossed) {
                guard.pos = m_player;
                onPlayerCaught();
                return;
            }
        }
    }
}

void GameWidget::updateDetection() {
    if (m_playerWon || m_playerCaught) return;

    AlertState prev = m_alertState;
    bool anySearch = false;
    bool anyChase = false;
    bool nearSight = false;

    for (const auto& guard : m_guards) {
        if (guard.mode == GuardMode::Chase) anyChase = true;
        if (guard.mode == GuardMode::Search || guard.mode == GuardMode::Return) anySearch = true;
        if (!m_playerHidden && std::abs(guard.pos.x - m_player.x) + std::abs(guard.pos.y - m_player.y) <= 2) nearSight = true;
    }

    if (anyChase) {
        m_alertState = AlertState::Alerted;
        m_suspiciousFrames = 0;
    } else if (anySearch) {
        m_alertState = AlertState::Suspicious;
        m_suspiciousFrames = 12;
    } else if (nearSight) {
        ++m_suspiciousFrames;
        m_alertState = (m_suspiciousFrames >= 12) ? AlertState::Suspicious : AlertState::Calm;
    } else {
        m_suspiciousFrames = 0;
        m_alertState = AlertState::Calm;
    }

    if (m_alertState == AlertState::Alerted && prev != AlertState::Alerted) {
        ++m_alertCount;
    }
    if (m_alertState == AlertState::Suspicious && prev != AlertState::Suspicious) {
        m_audio.playSuspicious();
    }
}

void GameWidget::onPlayerCaught() {
    if (m_playerCaught || m_respawnInvincibleFrames > 0) return;
    ++m_caughtCount;
    m_audio.playAlert();
    m_alertState = AlertState::Alerted;
    m_playerCaught = true;
    m_caughtFlashFrames = 20;
}

void GameWidget::onPlayerReachedGoal() {
    if (m_playerWon) return;
    m_alertState = AlertState::Victory;
    m_playerWon = true;
    m_victoryFrames = 0;
    finishCurrentMission();
    m_audio.playClear();
}

bool GameWidget::canSeePlayer(const Guard& guard) const {
    return canSeeFrom(guard.pos, guard.facing, 7, m_player);
}

bool GameWidget::canSeeFrom(const Vec2& watcher, Direction facing, int range, const Vec2& target) const {
    if (m_playerHidden || m_respawnInvincibleFrames > 0) return false;
    const int dx = target.x - watcher.x;
    const int dy = target.y - watcher.y;
    int forward = 0, side = 0;
    switch (facing) {
        case Direction::Up: forward = -dy; side = std::abs(dx); break;
        case Direction::Right: forward = dx; side = std::abs(dy); break;
        case Direction::Down: forward = dy; side = std::abs(dx); break;
        case Direction::Left: forward = -dx; side = std::abs(dy); break;
    }
    if (forward <= 0 || forward > range) return false;
    if (side > forward / 2 + 1) return false;
    return hasLineOfSight(watcher, target);
}

bool GameWidget::hasLineOfSight(const Vec2& from, const Vec2& to) const {
    int x = from.x, y = from.y;
    const int dx = std::abs(to.x - from.x), sx = from.x < to.x ? 1 : -1;
    const int dy = -std::abs(to.y - from.y), sy = from.y < to.y ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (!(x == from.x && y == from.y) && tileAt({x, y}) == TileType::Wall) return false;
        if (x == to.x && y == to.y) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
    return true;
}

std::optional<GameWidget::Vec2> GameWidget::nextStepToward(const Vec2& start, const Vec2& goal) const {
    if (!isInsideMap(start) || !isInsideMap(goal) || tileAt(goal) == TileType::Wall || tileAt(goal) == TileType::HideBox) return std::nullopt;
    if (start == goal) return start;

    const int h = (int)m_map.size(), w = (int)m_map[0].size();
    std::vector<std::vector<bool>> visited(h, std::vector<bool>(w, false));
    std::vector<std::vector<Vec2>> parent(h, std::vector<Vec2>(w, {-1, -1}));
    std::queue<Vec2> q;
    q.push(start);
    visited[start.y][start.x] = true;

    const std::array<Vec2, 4> dirs = {{{1,0},{-1,0},{0,1},{0,-1}}};
    while (!q.empty()) {
        Vec2 cur = q.front();
        q.pop();
        if (cur == goal) break;
        for (const auto& d : dirs) {
            Vec2 nxt{cur.x + d.x, cur.y + d.y};
            if (!isInsideMap(nxt) || visited[nxt.y][nxt.x] || tileAt(nxt) == TileType::Wall || tileAt(nxt) == TileType::HideBox) continue;
            visited[nxt.y][nxt.x] = true;
            parent[nxt.y][nxt.x] = cur;
            q.push(nxt);
        }
    }
    if (!visited[goal.y][goal.x]) return std::nullopt;

    Vec2 cur = goal;
    while (!(parent[cur.y][cur.x] == start) && !(cur == start)) {
        Vec2 p = parent[cur.y][cur.x];
        if (p.x == -1) break;
        cur = p;
    }
    return cur;
}

std::vector<GameWidget::Vec2> GameWidget::searchPattern(const Vec2& pivot) const {
    std::vector<Vec2> points;
    const std::array<Vec2, 8> offsets = {{{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}}};
    for (const auto& o : offsets) {
        Vec2 p{pivot.x + o.x * 3, pivot.y + o.y * 3};
        if (isInsideMap(p) && tileAt(p) != TileType::Wall && tileAt(p) != TileType::HideBox) points.push_back(p);
    }
    if (points.empty() && isInsideMap(pivot) && tileAt(pivot) != TileType::Wall && tileAt(pivot) != TileType::HideBox) points.push_back(pivot);
    return points;
}

void GameWidget::applyFacingFromStep(Guard& guard, const Vec2& next) {
    const int dx = next.x - guard.pos.x;
    const int dy = next.y - guard.pos.y;
    if (dx > 0) guard.facing = Direction::Right;
    else if (dx < 0) guard.facing = Direction::Left;
    else if (dy > 0) guard.facing = Direction::Down;
    else if (dy < 0) guard.facing = Direction::Up;
}

GameWidget::Vec2 GameWidget::dirToVec(Direction dir) const {
    switch (dir) {
        case Direction::Up: return {0, -1};
        case Direction::Right: return {1, 0};
        case Direction::Down: return {0, 1};
        case Direction::Left: return {-1, 0};
    }
    return {0, 0};
}


QString GameWidget::formatFrames(int frames) const {
    const int totalSeconds = std::max(0, frames / 60);
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

QString GameWidget::gradeFromScore(int score) const {
    if (score >= 95) return "S";
    if (score >= 85) return "A";
    if (score >= 72) return "B";
    if (score >= 60) return "C";
    return "D";
}

QString GameWidget::titleFromGrade(const QString& grade) const {
    if (grade == "S") return "无声潜入者";
    if (grade == "A") return "优秀特工";
    if (grade == "B") return "合格潜入者";
    if (grade == "C") return "仓促渗透者";
    return "警报制造机";
}

GameWidget::MissionStats GameWidget::makeCurrentMissionStats() const {
    MissionStats stats;
    stats.levelName = m_levels[m_currentLevel].name;
    stats.steps = m_stepCounter;
    stats.alertCount = m_alertCount;
    stats.caughtCount = m_caughtCount;
    stats.hideCount = m_hideCount;
    stats.frames = m_levelFrames;

    const int idealSteps = ((int)m_map[0].size() + (int)m_map.size()) * 2;
    const int stepPenalty = std::max(0, stats.steps - idealSteps) / 10;
    const int timePenalty = std::max(0, stats.frames / 60 - 180) / 8;
    int score = 100;
    score -= stats.alertCount * 12;
    score -= stats.caughtCount * 25;
    score -= stats.hideCount * 2;
    score -= stepPenalty;
    score -= timePenalty;
    stats.score = std::clamp(score, 0, 100);
    stats.grade = gradeFromScore(stats.score);
    stats.title = titleFromGrade(stats.grade);
    return stats;
}

void GameWidget::finishCurrentMission() {
    MissionStats stats = makeCurrentMissionStats();
    if ((int)m_completedStats.size() <= m_currentLevel) {
        m_completedStats.resize(m_currentLevel + 1);
    }
    m_completedStats[m_currentLevel] = stats;
    m_showMissionResult = true;
    m_showFinalResult = false;
    m_pressedKeys.clear();
}

void GameWidget::drawMissionResult(QPainter& p) {
    const MissionStats stats = makeCurrentMissionStats();
    p.fillRect(rect(), QColor(0, 0, 0, 205));
    QRect panel(width() / 2 - 300, height() / 2 - 220, 600, 440);
    p.fillRect(panel, QColor(12, 24, 20, 235));
    p.setPen(QColor(120, 255, 180));
    p.drawRect(panel.adjusted(0, 0, -1, -1));

    p.setFont(QFont("Consolas", 24, QFont::Bold));
    p.setPen(QColor(220, 255, 220));
    p.drawText(QRect(panel.left(), panel.top() + 24, panel.width(), 42), Qt::AlignCenter, "MISSION COMPLETE");

    p.setFont(QFont("Microsoft YaHei", 13, QFont::Bold));
    p.drawText(QRect(panel.left() + 40, panel.top() + 86, panel.width() - 80, 28), Qt::AlignLeft, QString("关卡：%1 / %2  %3").arg(m_currentLevel + 1).arg((int)m_levels.size()).arg(stats.levelName));

    p.setFont(QFont("Microsoft YaHei", 12));
    int y = panel.top() + 132;
    const int line = 32;
    p.drawText(QRect(panel.left() + 56, y, 260, line), Qt::AlignLeft, QString("用时：%1").arg(formatFrames(stats.frames))); y += line;
    p.drawText(QRect(panel.left() + 56, y, 260, line), Qt::AlignLeft, QString("步数：%1").arg(stats.steps)); y += line;
    p.drawText(QRect(panel.left() + 56, y, 260, line), Qt::AlignLeft, QString("警戒次数：%1").arg(stats.alertCount)); y += line;
    p.drawText(QRect(panel.left() + 56, y, 260, line), Qt::AlignLeft, QString("被抓次数：%1").arg(stats.caughtCount)); y += line;
    p.drawText(QRect(panel.left() + 56, y, 260, line), Qt::AlignLeft, QString("纸箱躲藏次数：%1").arg(stats.hideCount));

    p.setFont(QFont("Consolas", 64, QFont::Bold));
    p.setPen(stats.grade == "S" ? QColor(255, 240, 120) : QColor(170, 255, 210));
    p.drawText(QRect(panel.right() - 210, panel.top() + 132, 160, 86), Qt::AlignCenter, stats.grade);
    p.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    p.setPen(QColor(220, 255, 220));
    p.drawText(QRect(panel.right() - 260, panel.top() + 220, 250, 36), Qt::AlignCenter, stats.title);
    p.setFont(QFont("Consolas", 13, QFont::Bold));
    p.drawText(QRect(panel.right() - 260, panel.top() + 262, 250, 28), Qt::AlignCenter, QString("SCORE %1").arg(stats.score));

    p.setFont(QFont("Microsoft YaHei", 11));
    p.setPen(QColor(180, 230, 190));
    const QString nextText = (m_currentLevel + 1 < (int)m_levels.size()) ? "按 Enter 进入下一关，按 Esc 返回选关" : "按 Enter 查看最终总结算，按 Esc 返回选关";
    p.drawText(QRect(panel.left(), panel.bottom() - 58, panel.width(), 28), Qt::AlignCenter, nextText);
}

void GameWidget::drawFinalResult(QPainter& p) {
    p.fillRect(rect(), QColor(0, 0, 0, 215));
    QRect panel(width() / 2 - 330, height() / 2 - 250, 660, 500);
    p.fillRect(panel, QColor(10, 18, 22, 240));
    p.setPen(QColor(130, 210, 255));
    p.drawRect(panel.adjusted(0, 0, -1, -1));

    int totalSteps = 0, totalAlerts = 0, totalCaught = 0, totalHide = 0, totalFrames = 0, totalScore = 0, count = 0;
    for (const auto& s : m_completedStats) {
        if (s.levelName.isEmpty()) continue;
        totalSteps += s.steps;
        totalAlerts += s.alertCount;
        totalCaught += s.caughtCount;
        totalHide += s.hideCount;
        totalFrames += s.frames;
        totalScore += s.score;
        ++count;
    }
    const int averageScore = count > 0 ? totalScore / count : 0;
    const QString finalGrade = gradeFromScore(averageScore);

    p.setFont(QFont("Consolas", 24, QFont::Bold));
    p.setPen(QColor(220, 245, 255));
    p.drawText(QRect(panel.left(), panel.top() + 22, panel.width(), 42), Qt::AlignCenter, "OPERATION COMPLETE");

    p.setFont(QFont("Microsoft YaHei", 12));
    int y = panel.top() + 86;
    for (int i = 0; i < (int)m_completedStats.size(); ++i) {
        const auto& s = m_completedStats[i];
        if (s.levelName.isEmpty()) continue;
        p.setPen(QColor(210, 245, 220));
        p.drawText(QRect(panel.left() + 46, y, panel.width() - 92, 28), Qt::AlignLeft,
                   QString("%1. %2    %3  %4分  %5").arg(i + 1).arg(s.levelName).arg(s.grade).arg(s.score).arg(s.title));
        y += 32;
    }

    y += 16;
    p.setPen(QColor(180, 230, 255));
    p.drawText(QRect(panel.left() + 46, y, 300, 28), Qt::AlignLeft, QString("总用时：%1").arg(formatFrames(totalFrames))); y += 30;
    p.drawText(QRect(panel.left() + 46, y, 300, 28), Qt::AlignLeft, QString("总步数：%1").arg(totalSteps)); y += 30;
    p.drawText(QRect(panel.left() + 46, y, 300, 28), Qt::AlignLeft, QString("总警戒：%1    总被抓：%2    总躲藏：%3").arg(totalAlerts).arg(totalCaught).arg(totalHide));

    p.setFont(QFont("Consolas", 58, QFont::Bold));
    p.setPen(finalGrade == "S" ? QColor(255, 240, 120) : QColor(160, 220, 255));
    p.drawText(QRect(panel.right() - 210, panel.top() + 275, 160, 80), Qt::AlignCenter, finalGrade);
    p.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    p.setPen(QColor(225, 245, 255));
    p.drawText(QRect(panel.right() - 280, panel.top() + 358, 290, 36), Qt::AlignCenter, titleFromGrade(finalGrade));
    p.setFont(QFont("Consolas", 13, QFont::Bold));
    p.drawText(QRect(panel.right() - 280, panel.top() + 396, 290, 28), Qt::AlignCenter, QString("AVERAGE SCORE %1").arg(averageScore));

    p.setFont(QFont("Microsoft YaHei", 11));
    p.setPen(QColor(190, 225, 235));
    p.drawText(QRect(panel.left(), panel.bottom() - 52, panel.width(), 28), Qt::AlignCenter, "按 Enter 或 Esc 返回选关");
}

QString GameWidget::directionGlyph(Direction d) const {
    switch (d) {
        case Direction::Up: return "^";
        case Direction::Right: return ">";
        case Direction::Down: return "v";
        case Direction::Left: return "<";
    }
    return "?";
}

QString GameWidget::alertText() const {
    switch (m_alertState) {
        case AlertState::Calm: return "正常";
        case AlertState::Suspicious: return "警觉/搜索中";
        case AlertState::Alerted: return "已暴露";
        case AlertState::Victory: return "过关";
    }
    return "未知";
}

void GameWidget::updateStatusText() {
    if (!m_gameStarted) { emit statusChanged(QString("选关中：%1/%2 - %3 | 按 1-5 选关，Enter 开始") .arg(m_menuSelectedLevel + 1).arg((int)m_levels.size()).arg(m_levels[m_menuSelectedLevel].name)); return; }
    if (m_introPanFrames > 0) { emit statusChanged(QString("开场镜头中：%1").arg(m_levels[m_currentLevel].name)); return; }
    if (m_showFinalResult) { emit statusChanged("全部任务完成：最终总结算 | Enter/Esc 返回选关"); return; }
    if (m_showMissionResult) { emit statusChanged("关卡完成：任务评价已生成 | Enter 继续，Esc 返回选关"); return; }
    QString text = "任务代号: NKU-26C-MGS-tribute | ";
    text += QString("关卡: %1/%2 | ").arg(m_currentLevel + 1).arg((int)m_levels.size());
    text += QString("当前状态: %1 | ").arg(alertText());
    text += QString("玩家坐标: (%1,%2) | ").arg(m_player.x).arg(m_player.y);
    text += QString("步数: %1 | ").arg(m_stepCounter);
    text += m_playerHidden ? "已躲藏于纸箱" : "暴露于场景中";
    if (m_playerCaught) text += " | 已触发抓捕";
    if (m_playerWon) text += " | 关卡完成";
    emit statusChanged(text);
}
