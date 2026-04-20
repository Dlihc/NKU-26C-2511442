#include "AudioManager.h"
#include <QUrl>
AudioManager::AudioManager(QObject* parent) : QObject(parent) {
    m_bgmOutput.setVolume(0.20f);
    m_bgmPlayer.setAudioOutput(&m_bgmOutput);
    m_bgmPlayer.setSource(QUrl("qrc:/assets/audio/bgm.wav"));
    m_bgmPlayer.setLoops(QMediaPlayer::Infinite);
    m_stepEffect.setSource(QUrl("qrc:/assets/audio/step.wav"));
    m_stepEffect.setVolume(0.18f);
    m_alertEffect.setSource(QUrl("qrc:/assets/audio/alert.wav"));
    m_alertEffect.setVolume(0.55f);
    m_suspiciousEffect.setSource(QUrl("qrc:/assets/audio/suspicious.wav"));
    m_suspiciousEffect.setVolume(0.38f);
    m_clearEffect.setSource(QUrl("qrc:/assets/audio/clear.wav"));
    m_clearEffect.setVolume(0.38f);
    m_levelStartEffect.setSource(QUrl("qrc:/assets/audio/level_start.wav"));
    m_levelStartEffect.setVolume(0.30f);
}
void AudioManager::playBgm() { m_bgmPlayer.play(); }
void AudioManager::stopBgm() { m_bgmPlayer.stop(); }
void AudioManager::playStep() { m_stepEffect.play(); }
void AudioManager::playAlert() { m_alertEffect.play(); }
void AudioManager::playClear() { m_clearEffect.play(); }
void AudioManager::playLevelStart() { m_levelStartEffect.play(); }

void AudioManager::playSuspicious() { m_suspiciousEffect.play(); }
