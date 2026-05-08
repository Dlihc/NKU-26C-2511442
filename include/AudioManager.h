#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QElapsedTimer>
class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);
    void playBgm();
    void stopBgm();
    void playStep();
    void playAlert();
    void playSuspicious();
    void playClear();
    void playLevelStart();
    void playTranquilizer();
private:
    QMediaPlayer m_bgmPlayer;
    QAudioOutput m_bgmOutput;
    QSoundEffect m_stepEffect;
    QSoundEffect m_alertEffect;
    QSoundEffect m_suspiciousEffect;
    QSoundEffect m_clearEffect;
    QSoundEffect m_levelStartEffect;
    QSoundEffect m_tranquilizerEffect;
    QElapsedTimer m_alertTimer;
};
