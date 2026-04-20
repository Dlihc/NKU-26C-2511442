#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
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
private:
    QMediaPlayer m_bgmPlayer;
    QAudioOutput m_bgmOutput;
    QSoundEffect m_stepEffect;
    QSoundEffect m_alertEffect;
    QSoundEffect m_suspiciousEffect;
    QSoundEffect m_clearEffect;
    QSoundEffect m_levelStartEffect;
};
