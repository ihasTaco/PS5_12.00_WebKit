/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaPlayerPrivatePlayStation.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION)

#if ENABLE(ENCRYPTED_MEDIA)
#include "CDMInstancePlayStation.h"
#endif
#include "CachedResourceLoader.h"
#include "Document.h"
#include "FocusController.h"
#include "GraphicsContext.h"
#include "HTMLMediaElement.h"
#include "LocalFrameView.h"
#include "Logging.h"
#include "MediaSourcePrivateClient.h"
#include "Page.h"
#include "RenderMedia.h"
#include "Settings.h"
#include "TextureMapperPlatformLayerBuffer.h"
#include "TextureMapperPlatformLayerProxy.h"
#include "TextureMapperPlatformLayerProxyGL.h"
#include "VisibilityState.h"
#include <JavaScriptCore/ArrayBuffer.h>

#if ENABLE(MEDIA_SOURCE)
#include "mse/MediaSourcePrivatePlayStation.h"
#endif
#include <canvas/canvas.h>

#include <list>

namespace WebCore {

class MediaPlayerFactoryPlayStation final : public MediaPlayerFactory {
private:
    MediaPlayerEnums::MediaEngineIdentifier identifier() const final { return MediaPlayerEnums::MediaEngineIdentifier::PlayStation; };

    std::unique_ptr<MediaPlayerPrivateInterface> createMediaEnginePlayer(MediaPlayer* player) const final
    {
        return makeUnique<MediaPlayerPrivatePlayStation>(player);
    }

    void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types) const final
    {
        return MediaPlayerPrivatePlayStation::getSupportedTypes(types);
    }

    MediaPlayer::SupportsType supportsTypeAndCodecs(const MediaEngineSupportParameters& parameters) const final
    {
        return MediaPlayerPrivatePlayStation::supportsType(parameters);
    }

    bool supportsKeySystem(const String& keySystem, const String& mimeType) const final
    {
        return MediaPlayerPrivatePlayStation::supportsKeySystem(keySystem, mimeType);
    }
};

void MediaPlayerPrivatePlayStation::registerMediaEngine(MediaEngineRegistrar registrar)
{
    registrar(makeUnique<MediaPlayerFactoryPlayStation>());
}

MediaPlayerPrivatePlayStation::MediaPlayerPrivatePlayStation(MediaPlayer* player)
    : m_weakThis(WeakPtr(this))
    , m_player(player)
    , m_updateStatusTimer(*this, &MediaPlayerPrivatePlayStation::updateStatusTimerFired)
    , m_url()
    , m_isDelayingLoad(false)
    , m_preload(MediaPlayer::Preload::Auto)
    , m_isPaused(true)
    , m_playbackRate(1.0)
    , m_volume(1.0)
    , m_isMuted(false)
    , m_isSeeking(false)
    , m_seekTime(MediaTime::zeroTime())
    , m_canvasHandle(canvas::Canvas::kInvalidHandle)
{
    m_playstation = mediaplayer::PSMediaPlayerBackend::create(this);

    m_isMediaPlayable = isMediaPlayable();
    m_playstation->setMediaPlayable(m_isMediaPlayable);

    static const double timerInterval = (1.0 / 60.0);
    m_updateStatusTimer.startRepeating(WTF::Seconds(timerInterval));
}

MediaPlayerPrivatePlayStation::~MediaPlayerPrivatePlayStation()
{
    m_playstation->shutdown();

    m_updateStatusTimer.stop();
    destroyLayerForAcceleratedCompositing();
}

void MediaPlayerPrivatePlayStation::load(const String& url)
{
#if ENABLE(MEDIA_SOURCE)
    m_mediaSourcePrivateClient = nullptr;
#endif

    m_url = url;
    loadInternal();
}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivatePlayStation::load(const URL& url, const ContentType&, MediaSourcePrivateClient& client)
{
    m_mediaSourcePrivateClient = client;

    m_url = url.string();
    loadInternal();
}
#endif

#if ENABLE(MEDIA_STREAM)
void MediaPlayerPrivatePlayStation::load(MediaStreamPrivate& priv)
{
    // NotSupported Error
    m_player->client().mediaPlayerResourceNotSupported(m_player);
    // m_playstation->load(priv);
}
#endif

void MediaPlayerPrivatePlayStation::cancelLoad()
{
    m_playstation->cancelLoad();
}

void MediaPlayerPrivatePlayStation::prepareToPlay()
{
    if (m_isDelayingLoad) {
        m_isDelayingLoad = false;
        commitLoad();
    }
}

PlatformLayer* MediaPlayerPrivatePlayStation::platformLayer() const
{
    ASSERT(m_nicosiaLayer);
    return m_nicosiaLayer.get();
}

long MediaPlayerPrivatePlayStation::platformErrorCode() const
{
    return m_playstation->platformErrorCode();
}

void MediaPlayerPrivatePlayStation::play()
{
    m_isPaused = false;
    updatePlayStatus();
}

void MediaPlayerPrivatePlayStation::pause()
{
    m_isPaused = true;
    updatePlayStatus();
}

void MediaPlayerPrivatePlayStation::setBufferingPolicy(MediaPlayer::BufferingPolicy)
{
    notImplemented();
}

bool MediaPlayerPrivatePlayStation::supportsPictureInPicture() const
{
    return m_playstation->supportsPictureInPicture();
}

bool MediaPlayerPrivatePlayStation::supportsFullscreen() const
{
    return m_playstation->supportsFullscreen();
}

bool MediaPlayerPrivatePlayStation::supportsScanning() const
{
    return m_playstation->supportsScanning();
}

bool MediaPlayerPrivatePlayStation::requiresImmediateCompositing() const
{
    return m_playstation->requiresImmediateCompositing();
}

bool MediaPlayerPrivatePlayStation::canSaveMediaData() const
{
    return m_playstation->canSaveMediaData();
}

FloatSize MediaPlayerPrivatePlayStation::naturalSize() const
{
    mediaplayer::FloatSize size(0.0f, 0.0f);
    if (readyState() > MediaPlayer::ReadyState::HaveNothing)
        size = m_playstation->naturalSize();
    return FloatSize(size.m_width, size.m_height);
}

bool MediaPlayerPrivatePlayStation::hasVideo() const
{
    return m_playstation->hasVideo();
}

bool MediaPlayerPrivatePlayStation::hasAudio() const
{
    return m_playstation->hasAudio();
}

void MediaPlayerPrivatePlayStation::setPageIsVisible(bool visible)
{
    m_playstation->setVisible(visible);
}

float MediaPlayerPrivatePlayStation::duration() const
{
    return static_cast<float>(durationDouble());
}

double MediaPlayerPrivatePlayStation::durationDouble() const
{
    if (readyState() == MediaPlayer::ReadyState::HaveNothing)
        return 0.0;

    if (m_playstation->isLiveStreaming())
        return std::numeric_limits<double>::infinity();

    return m_playstation->duration();
}

MediaTime MediaPlayerPrivatePlayStation::durationMediaTime() const
{
    return MediaTime::createWithDouble(durationDouble());
}

float MediaPlayerPrivatePlayStation::currentTime() const
{
    return static_cast<float>(currentTimeDouble());
}

double MediaPlayerPrivatePlayStation::currentTimeDouble() const
{
#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSourcePrivateClient
        && ended()
        && (readyState() >= MediaPlayer::ReadyState::HaveMetadata))
        return std::max(durationDouble(), m_mediaSourcePrivateClient->duration().toDouble());
#endif
    if (m_isSeeking)
        return m_seekTime.toDouble();
    return m_playstation->currentTime();
}

MediaTime MediaPlayerPrivatePlayStation::currentMediaTime() const
{
    return MediaTime::createWithDouble(currentTimeDouble());
}

MediaTime MediaPlayerPrivatePlayStation::getStartDate() const
{
    return MediaTime::createWithDouble(m_playstation->getStartDate());
}

void MediaPlayerPrivatePlayStation::seek(float time)
{
    seek(MediaTime::createWithFloat(time));
}

void MediaPlayerPrivatePlayStation::seekDouble(double time)
{
    seek(MediaTime::createWithDouble(time));
}

void MediaPlayerPrivatePlayStation::seek(const MediaTime& time)
{
    seekWithTolerance(time, MediaTime::zeroTime(), MediaTime::zeroTime());
}

void MediaPlayerPrivatePlayStation::seekWithTolerance(const MediaTime& time, const MediaTime& from, const MediaTime& to)
{
    if (time == currentMediaTime()) {
        seeked();
        return;
    }

    m_isSeeking = true;
    m_seekTime = time;

#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSourcePrivateClient)
        m_mediaSourcePrivateClient->seekToTime(time);
#endif
    m_playstation->seekWithTolerance(time.toDouble(), from.toDouble(), to.toDouble());
}

bool MediaPlayerPrivatePlayStation::seeking() const
{
    return m_isSeeking;
}

MediaTime MediaPlayerPrivatePlayStation::startTime() const
{
    return MediaTime::createWithDouble(m_playstation->startTime());
}

MediaTime MediaPlayerPrivatePlayStation::initialTime() const
{
    return MediaTime::createWithDouble(m_playstation->initialTime());
}

void MediaPlayerPrivatePlayStation::setRate(float rate)
{
    setRateDouble(static_cast<double>(rate));
}

void MediaPlayerPrivatePlayStation::setRateDouble(double rate)
{
    if (m_playbackRate == rate)
        return;

    if (m_playstation->isLiveStreaming())
        return;

    m_playbackRate = rate;

    if (paused())
        return;

    m_playstation->setRate(rate);
}

double MediaPlayerPrivatePlayStation::rate() const
{
    return m_playbackRate;
}

void MediaPlayerPrivatePlayStation::setPreservesPitch(bool preserve)
{
    m_playstation->setPreservesPitch(preserve);
}

bool MediaPlayerPrivatePlayStation::paused() const
{
    return m_isPaused;
}

void MediaPlayerPrivatePlayStation::setVolume(float volume)
{
    setVolumeDouble(static_cast<double>(volume));
}

void MediaPlayerPrivatePlayStation::setVolumeDouble(double volume)
{
    if (m_volume == volume)
        return;

    m_volume = volume;

    m_playstation->setVolume(volume);
}

void MediaPlayerPrivatePlayStation::setMuted(bool muted)
{
    if (m_isMuted == muted)
        return;

    m_isMuted = muted;

    m_playstation->setMuted(muted);
}

bool MediaPlayerPrivatePlayStation::hasClosedCaptions() const
{
    return m_playstation->hasClosedCaptions();
}

void MediaPlayerPrivatePlayStation::setClosedCaptionsVisible(bool visible)
{
    m_playstation->setClosedCaptionsVisible(visible);
}

double MediaPlayerPrivatePlayStation::maxFastForwardRate() const
{
    return m_playstation->maxFastForwardRate();
}

double MediaPlayerPrivatePlayStation::minFastReverseRate() const
{
    return m_playstation->minFastReverseRate();
}

MediaPlayer::NetworkState MediaPlayerPrivatePlayStation::networkState() const
{
    mediaplayer::NetworkState state = m_playstation->networkState();
    switch (state) {
    case mediaplayer::NetworkState::Empty:              return MediaPlayer::NetworkState::Empty;
    case mediaplayer::NetworkState::Idle:               return MediaPlayer::NetworkState::Idle;
    case mediaplayer::NetworkState::Loading:            return MediaPlayer::NetworkState::Loading;
    case mediaplayer::NetworkState::Loaded:             return MediaPlayer::NetworkState::Loaded;
    case mediaplayer::NetworkState::FormatError:        return MediaPlayer::NetworkState::FormatError;
    case mediaplayer::NetworkState::NetworkError:       return MediaPlayer::NetworkState::NetworkError;
    case mediaplayer::NetworkState::DecodeError:        return MediaPlayer::NetworkState::DecodeError;
    case mediaplayer::NetworkState::NoResourceError:    return MediaPlayer::NetworkState::NoResourceError;
    default: return MediaPlayer::NetworkState::Empty;
    }
}

MediaPlayer::ReadyState MediaPlayerPrivatePlayStation::readyState() const
{
    mediaplayer::ReadyState state = m_playstation->readyState();
    switch (state) {
    case mediaplayer::ReadyState::HaveNothing:     return MediaPlayer::ReadyState::HaveNothing;
    case mediaplayer::ReadyState::HaveMetaData:    return MediaPlayer::ReadyState::HaveMetadata;
    case mediaplayer::ReadyState::HaveCurrentData: return MediaPlayer::ReadyState::HaveCurrentData;
    case mediaplayer::ReadyState::HaveFutureData:  return MediaPlayer::ReadyState::HaveFutureData;
    case mediaplayer::ReadyState::HaveEnoughData:  return MediaPlayer::ReadyState::HaveEnoughData;
    default: return MediaPlayer::ReadyState::HaveNothing;
    }
}

const PlatformTimeRanges& MediaPlayerPrivatePlayStation::seekable() const
{
    auto ranges = m_playstation->seekable();

    m_seekable.clear();
    size_t len = ranges.get()->length();
    for (size_t i = 0; i < len; i++) {
        double start, end;
        if (ranges.get()->getRange(i, start, end))
            m_seekable.add(MediaTime::createWithDouble(start), MediaTime::createWithDouble(end));
        else
            break;
    }

    return m_seekable;
}

float MediaPlayerPrivatePlayStation::maxTimeSeekable() const
{
    return static_cast<float>(m_playstation->maxTimeSeekable());
}

MediaTime MediaPlayerPrivatePlayStation::maxMediaTimeSeekable() const
{
    return MediaTime::createWithDouble(m_playstation->maxTimeSeekable());
}

double MediaPlayerPrivatePlayStation::minTimeSeekable() const
{
    return m_playstation->minTimeSeekable();
}

MediaTime MediaPlayerPrivatePlayStation::minMediaTimeSeekable() const
{
    return MediaTime::createWithDouble(m_playstation->minTimeSeekable());
}

const PlatformTimeRanges& MediaPlayerPrivatePlayStation::buffered() const
{
    auto ranges = m_playstation->buffered();

    m_buffered.clear();
    size_t len = ranges.get()->length();
    for (size_t i = 0; i < len; i++) {
        double start, end;
        if (ranges.get()->getRange(i, start, end))
            m_buffered.add(MediaTime::createWithDouble(start), MediaTime::createWithDouble(end));
        else
            break;
    }

    return m_buffered;
}

double MediaPlayerPrivatePlayStation::seekableTimeRangesLastModifiedTime() const
{
    return m_playstation->seekableTimeRangesLastModifiedTime();
}

double MediaPlayerPrivatePlayStation::liveUpdateInterval() const
{
    return m_playstation->liveUpdateInterval();
}

unsigned long long MediaPlayerPrivatePlayStation::totalBytes() const
{
    return m_playstation->totalBytes();
}

bool MediaPlayerPrivatePlayStation::didLoadingProgress() const
{
    return m_playstation->didLoadingProgress();
}

void MediaPlayerPrivatePlayStation::setPresentationSize(const IntSize& size)
{
    float deviceScaleFactor = 1.0f;
    Document* document = getDocumentPointer();
    if (document)
        deviceScaleFactor = document->deviceScaleFactor();

    int width = size.width() * deviceScaleFactor;
    int height = size.height() * deviceScaleFactor;

    m_playstation->setSize(mediaplayer::IntSize(width, height));
}

//
// GraphicsContext contains not only the video element but also other elements that should be repainted.
// However, the another rect parameter always points the position of video element in View.
// We should move paintRect with some other offset value to get the actual rect of video element in mediaplayer::PSMediaPlayerBackendInterface.
//
// paintRect.moveBy(...);
//
void MediaPlayerPrivatePlayStation::paint(GraphicsContext& context, const FloatRect& rect)
{
    if (context.paintingDisabled())
        return;

    Document* document = getDocumentPointer();

    if (!document)
        return;

    // If the canvas texture is not ready, the application may not receive punchHole events yet.
    // In such case, we render black rectangle instead of transparent hole.
    if (m_canvasHandle == canvas::Canvas::kInvalidHandle)
        context.fillRect(rect, Color::black);
    else
        context.fillRect(rect, Color::transparentBlack, CompositeOperator::Clear);
}

DestinationColorSpace MediaPlayerPrivatePlayStation::colorSpace()
{
    return DestinationColorSpace::SRGB();
}

void MediaPlayerPrivatePlayStation::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;

    if (m_isDelayingLoad && m_preload != MediaPlayer::Preload::None) {
        m_isDelayingLoad = false;
        commitLoad();
    }
}

bool MediaPlayerPrivatePlayStation::hasAvailableVideoFrame() const
{
    return m_playstation->hasAvailableVideoFrame();
}

#if USE(NATIVE_FULLSCREEN_VIDEO)
void MediaPlayerPrivatePlayStation::enterFullscreen()
{
    m_playstation->enterFullscreen();
}

void MediaPlayerPrivatePlayStation::exitFullscreen()
{
    m_playstation->exitFullscreen();
}
#endif

#if USE(NATIVE_FULLSCREEN_VIDEO)
bool MediaPlayerPrivatePlayStation::canEnterFullscreen() const
{
    return m_playstation->canEnterFullscreen();
}
#endif

bool MediaPlayerPrivatePlayStation::supportsAcceleratedRendering() const
{
    Document* document = getDocumentPointer();
    if (document)
        return document->settings().acceleratedCompositingEnabled();
    return false;
}

void MediaPlayerPrivatePlayStation::acceleratedRenderingStateChanged()
{
    m_player->renderingModeChanged();

    if (m_player->renderingCanBeAccelerated())
        ensureLayerForAcceleratedCompositing();
    else
        destroyLayerForAcceleratedCompositing();
}

void MediaPlayerPrivatePlayStation::setShouldMaintainAspectRatio(bool flag)
{
    m_playstation->setShouldMaintainAspectRatio(flag);
}

bool MediaPlayerPrivatePlayStation::didPassCORSAccessCheck() const
{
    return m_playstation->didPassCORSAccessCheck();
}

MediaPlayer::MovieLoadType MediaPlayerPrivatePlayStation::movieLoadType() const
{
    if (readyState() == MediaPlayer::ReadyState::HaveNothing)
        return MediaPlayer::MovieLoadType::Unknown;

    if (m_playstation->isLiveStreaming())
        return MediaPlayer::MovieLoadType::LiveStream;

    return MediaPlayer::MovieLoadType::Download;
}

void MediaPlayerPrivatePlayStation::prepareForRendering()
{
    m_playstation->prepareForRendering();
}

MediaTime MediaPlayerPrivatePlayStation::mediaTimeForTimeValue(const MediaTime& timeValue) const
{
    return MediaTime::createWithDouble(m_playstation->mediaTimeForTimeValue(timeValue.toDouble()));
}

double MediaPlayerPrivatePlayStation::maximumDurationToCacheMediaTime() const
{
    return m_playstation->maximumDurationToCacheMediaTime();
}

unsigned MediaPlayerPrivatePlayStation::decodedFrameCount() const
{
    return m_playstation->decodedFrameCount();
}

unsigned MediaPlayerPrivatePlayStation::droppedFrameCount() const
{
    return m_playstation->droppedFrameCount();
}

unsigned MediaPlayerPrivatePlayStation::audioDecodedByteCount() const
{
    return m_playstation->audioDecodedByteCount();
}

unsigned MediaPlayerPrivatePlayStation::videoDecodedByteCount() const
{
    return m_playstation->videoDecodedByteCount();
}

void MediaPlayerPrivatePlayStation::setPrivateBrowsingMode(bool flag)
{
    m_playstation->setPrivateBrowsingMode(flag);
}

String MediaPlayerPrivatePlayStation::engineDescription() const
{
    std::string description = m_playstation->engineDescription();
    return String::fromUTF8(description.c_str());
}

#if ENABLE(WEB_AUDIO)
AudioSourceProvider* MediaPlayerPrivatePlayStation::audioSourceProvider()
{
    return 0; // m_playstation->audioSourceProvider();
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayerPrivatePlayStation::cdmInstanceAttached(CDMInstance& instance)
{
    if (m_cdmInstance != &instance) {
        m_cdmInstance = downcast<CDMInstancePlayStation>(&instance);
        m_playstation->cdmInstanceAttached(*(m_cdmInstance->backend().get()));
    }
}

void MediaPlayerPrivatePlayStation::cdmInstanceDetached(CDMInstance& instance)
{
    if (m_cdmInstance == &instance) {
        m_playstation->cdmInstanceDetached(*(m_cdmInstance->backend().get()));
        m_cdmInstance = nullptr;
    }
}

void MediaPlayerPrivatePlayStation::attemptToDecryptWithInstance(CDMInstance& instance)
{
    ASSERT_UNUSED(instance, m_cdmInstance.get() == &instance);
    if (m_cdmInstance)
        m_playstation->attemptToDecryptWithInstance(*(m_cdmInstance->backend().get()));
}

bool MediaPlayerPrivatePlayStation::waitingForKey() const
{
    return m_playstation->waitingForKey();
}
#endif

bool MediaPlayerPrivatePlayStation::requiresTextTrackRepresentation() const
{
    return m_playstation->requiresTextTrackRepresentation();
}

void MediaPlayerPrivatePlayStation::setTextTrackRepresentation(TextTrackRepresentation*)
{
    m_playstation->setTextTrackRepresentation(nullptr);
}

void MediaPlayerPrivatePlayStation::syncTextTrackBounds()
{
    m_playstation->syncTextTrackBounds();
}

void MediaPlayerPrivatePlayStation::tracksChanged()
{
    m_playstation->tracksChanged();
}

String MediaPlayerPrivatePlayStation::languageOfPrimaryAudioTrack() const
{
    std::string audioTrack = m_playstation->languageOfPrimaryAudioTrack();
    return String::fromLatin1(audioTrack.c_str());
}

size_t MediaPlayerPrivatePlayStation::extraMemoryCost() const
{
    MediaTime duration = this->durationMediaTime();
    if (!duration)
        return 0;

    unsigned long long extra = totalBytes() * buffered().totalDuration().toDouble() / duration.toDouble();
    return static_cast<unsigned>(extra);
}

unsigned long long MediaPlayerPrivatePlayStation::fileSize() const
{
    return m_playstation->fileSize();
}

bool MediaPlayerPrivatePlayStation::ended() const
{
    double currentTime = m_isSeeking ? m_seekTime.toDouble() : m_playstation->currentTime();
    return ((m_playbackRate > 0) && (currentTime >= durationDouble()));
}

#if ENABLE(MEDIA_SOURCE)
std::optional<VideoPlaybackQualityMetrics> MediaPlayerPrivatePlayStation::videoPlaybackQualityMetrics()
{
    return std::nullopt;
}
#endif

void MediaPlayerPrivatePlayStation::notifyActiveSourceBuffersChanged()
{
    m_playstation->notifyActiveSourceBuffersChanged();
}

void MediaPlayerPrivatePlayStation::setShouldDisableSleep(bool disable)
{
    m_playstation->setShouldDisableSleep(disable);
}

void MediaPlayerPrivatePlayStation::applicationWillResignActive()
{
    m_playstation->applicationWillResignActive();
}

void MediaPlayerPrivatePlayStation::applicationDidBecomeActive()
{
    m_playstation->applicationDidBecomeActive();
}

bool MediaPlayerPrivatePlayStation::performTaskAtMediaTime(Function<void()>&& callback, const MediaTime& time)
{
    performTaskAtMediaTimeCallbackQueue().enqueue(WTFMove(callback));
    return m_playstation->performTaskAtMediaTime([this]() {
        callOnMainThreadIfNeeded([this]() {
            Function<void()>&& callback = performTaskAtMediaTimeCallbackQueue().dequeue();
            callback();
        });
    }, time.toDouble());
}

bool MediaPlayerPrivatePlayStation::shouldIgnoreIntrinsicSize()
{
    return m_playstation->shouldIgnoreIntrinsicSize();
}

void MediaPlayerPrivatePlayStation::setPreferredDynamicRangeMode(DynamicRangeMode mode)
{
    mediaplayer::DynamicRangeMode dynamicRangeMode = mediaplayer::DynamicRangeMode::None;
    switch (mode) {
    case DynamicRangeMode::None:            dynamicRangeMode = mediaplayer::DynamicRangeMode::None;             break;
    case DynamicRangeMode::Standard:        dynamicRangeMode = mediaplayer::DynamicRangeMode::Standard;         break;
    case DynamicRangeMode::HLG:             dynamicRangeMode = mediaplayer::DynamicRangeMode::HLG;              break;
    case DynamicRangeMode::HDR10:           dynamicRangeMode = mediaplayer::DynamicRangeMode::HDR10;            break;
    case DynamicRangeMode::DolbyVisionPQ:   dynamicRangeMode = mediaplayer::DynamicRangeMode::DolbyVisionPQ;    break;
    default:                                dynamicRangeMode = mediaplayer::DynamicRangeMode::None;             break;
    }
    m_playstation->setPreferredDynamicRangeMode(dynamicRangeMode);
}

uint32_t MediaPlayerPrivatePlayStation::canvasHandle() const
{
    return m_canvasHandle;
}

// interval timer for PS4
void MediaPlayerPrivatePlayStation::updateStatusTimerFired()
{
    bool mediaPlayable = isMediaPlayable();
    if (m_isMediaPlayable != mediaPlayable) {
        m_isMediaPlayable = mediaPlayable;
        m_playstation->setMediaPlayable(m_isMediaPlayable);
    }

    m_playstation->updateStatus();
}

// mediaplayer::PSMediaPlayerBackendClient
void MediaPlayerPrivatePlayStation::networkStateChanged()
{
    callOnMainThreadIfNeeded([this]() {
        switch (m_playstation->networkState()) {
        case mediaplayer::NetworkState::FormatError:
        case mediaplayer::NetworkState::NetworkError:
        case mediaplayer::NetworkState::DecodeError:
        case mediaplayer::NetworkState::NoResourceError:
            m_playstation->cancelLoad();
            break;
        default:
            break;
        }
        m_player->networkStateChanged();
    });
}

void MediaPlayerPrivatePlayStation::readyStateChanged()
{
    callOnMainThreadIfNeeded([this]() {
        m_player->readyStateChanged();
    });
}

void MediaPlayerPrivatePlayStation::volumeChanged(double volume)
{
    callOnMainThreadIfNeeded([this, volume]() {
        m_player->volumeChanged(volume);
    });
}

void MediaPlayerPrivatePlayStation::muteChanged(bool muted)
{
    callOnMainThreadIfNeeded([this, muted]() {
        m_player->muteChanged(muted);
    });
}

void MediaPlayerPrivatePlayStation::timeChanged()
{
    callOnMainThreadIfNeeded([this]() {
        setPauseIfEnded();
        m_player->timeChanged();
    });
}

void MediaPlayerPrivatePlayStation::sizeChanged()
{
    callOnMainThreadIfNeeded([this]() {
        m_player->sizeChanged();
    });
}

void MediaPlayerPrivatePlayStation::rateChanged()
{
    callOnMainThreadIfNeeded([this]() {
        m_playbackRate = m_playstation->rate();
        m_player->rateChanged();
    });
}

void MediaPlayerPrivatePlayStation::playbackStateChanged()
{
    callOnMainThreadIfNeeded([this]() {
        m_isPaused = m_playstation->paused();
        m_player->playbackStateChanged();
    });
}

void MediaPlayerPrivatePlayStation::durationChanged()
{
    callOnMainThreadIfNeeded([this]() {
#if ENABLE(MEDIA_SOURCE)
        if (m_mediaSourcePrivate)
            m_mediaSourcePrivate->notifyDurationChanged();
#endif
        setPauseIfEnded();
        m_player->durationChanged();
    });
}

void MediaPlayerPrivatePlayStation::firstVideoFrameAvailable()
{
    callOnMainThreadIfNeeded([this]() {
        m_player->firstVideoFrameAvailable();
    });
}

void MediaPlayerPrivatePlayStation::characteristicChanged()
{
    callOnMainThreadIfNeeded([this]() {
        m_player->characteristicChanged();
    });
}

void MediaPlayerPrivatePlayStation::repaint()
{
    callOnMainThreadIfNeeded([this]() {
        m_player->repaint();
    });
}

void MediaPlayerPrivatePlayStation::seeked()
{
    callOnMainThreadIfNeeded([this]() {
        m_isSeeking = false;
        timeChanged();
    });
}

void MediaPlayerPrivatePlayStation::updateCanvasHandle(uint32_t canvasHandle)
{
    if (m_canvasHandle == canvasHandle)
        return;

    // FIXME: What is CanvasMode?
    m_canvasHandle = canvasHandle;
#if ENABLE(PUNCH_HOLE)
    m_player->setPunchHoleCanvasHandle(canvasHandle);
    if (m_nicosiaLayer)
        pushNextHolePunchBuffer();
#endif

    callOnMainThreadIfNeeded([this]() {
        m_player->renderingCanBeAccelerated();
        m_player->repaint();
    });
}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivatePlayStation::mediaSourceOpen()
{
    callOnMainThreadIfNeeded([this]() {
        if (m_mediaSourcePrivateClient)
            MediaSourcePrivatePlayStation::open(*this, *m_mediaSourcePrivateClient.get());
    });
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayerPrivatePlayStation::initializationDataEncountered(const std::string& type, const std::vector<uint8_t>& initData)
{
    String strType = String::fromUTF8(type.c_str());
    std::vector<uint8_t> data = initData;

    callOnMainThreadIfNeeded([this, strType = WTFMove(strType), data = WTFMove(data)]() {
        m_player->initializationDataEncountered(strType, ArrayBuffer::create(data.data(), data.size()));
    });
}

void MediaPlayerPrivatePlayStation::waitingForKeyChanged()
{
    callOnMainThreadIfNeeded([this]() {
#if ENABLE(MEDIA_SOURCE)
        if (m_mediaSourcePrivateClient) {
            if (waitingForKey()) {
                if (isTimeBuffered(currentMediaTime()))
                    setReadyState(MediaPlayer::ReadyState::HaveCurrentData);
                else
                    setReadyState(MediaPlayer::ReadyState::HaveMetadata);
            } else
                m_mediaSourcePrivateClient->monitorSourceBuffers();
        }
#endif
        m_player->waitingForKeyChanged();
    });
}
#endif // ENABLE(ENCRYPTED_MEDIA)

// mediaplayer::PSGraphicsContextClient
bool MediaPlayerPrivatePlayStation::paintingDisabled() const
{
    return false;
}

// mediaplayer::PSCDMInstanceClient
#if ENABLE(ENCRYPTED_MEDIA)
std::string MediaPlayerPrivatePlayStation::keySystem() const
{
    if (!m_cdmInstance)
        return { };
    String keySystemString = m_cdmInstance->keySystem();
    return std::string(keySystemString.utf8().data());
}
#endif

#if ENABLE(PUNCH_HOLE)
void MediaPlayerPrivatePlayStation::pushNextHolePunchBuffer()
{
    ASSERT(m_nicosiaLayer);

    auto proxyOperation = [](TextureMapperPlatformLayerProxyGL& proxy, canvas::Canvas::Handle canvasHandle) {
        Locker locker { proxy.lock() };
        std::unique_ptr<TextureMapperPlatformLayerBuffer> layerBuffer = makeUnique<TextureMapperPlatformLayerBuffer>(canvasHandle, TextureMapperGL::ShouldNotBlend);
        proxy.pushNextBuffer(WTFMove(layerBuffer));
    };

    auto& proxy = downcast<Nicosia::ContentLayerTextureMapperImpl>(m_nicosiaLayer->impl()).proxy();
    ASSERT(is<TextureMapperPlatformLayerProxyGL>(proxy));
    proxyOperation(downcast<TextureMapperPlatformLayerProxyGL>(proxy), m_canvasHandle);
}
#endif

void MediaPlayerPrivatePlayStation::ensureLayerForAcceleratedCompositing()
{
    if (m_nicosiaLayer)
        return;

    m_nicosiaLayer = Nicosia::ContentLayer::create(Nicosia::ContentLayerTextureMapperImpl::createFactory(*this));

#if ENABLE(PUNCH_HOLE)
    pushNextHolePunchBuffer();
#endif
}

void MediaPlayerPrivatePlayStation::destroyLayerForAcceleratedCompositing()
{
    if (!m_nicosiaLayer)
        return;

    downcast<Nicosia::ContentLayerTextureMapperImpl>(m_nicosiaLayer->impl()).invalidateClient();
    m_nicosiaLayer = nullptr;
}

void MediaPlayerPrivatePlayStation::swapBuffersIfNeeded()
{
#if ENABLE(PUNCH_HOLE)
    // FIXME: This code should not be needed because swapping buffers is needed only when the canvas handle is updated.
    // But for a workaround to an issue that the video disappears when TextureMapplerLayerProxy is moved to another layer,
    // swapBuffersIfNeeded() needs to be called from CoordinatedGraphicsScene.
    if (m_nicosiaLayer)
        pushNextHolePunchBuffer();
#endif
}

static HashSet<String, ASCIICaseInsensitiveHash>& mimeTypeCache()
{
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cache = []()
    {
        std::list<std::string> supportsTypes;
        mediaplayer::PSMediaPlayerBackend::getSupportedTypes(supportsTypes);

        HashSet<String, ASCIICaseInsensitiveHash> set;
        for (const std::string& type : supportsTypes)
            set.add(String::fromUTF8(type.c_str()));

        return set;
    }();
    return cache;
}

// engine support
void MediaPlayerPrivatePlayStation::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    INFO_LOG(LOGIDENTIFIER);
    types = mimeTypeCache();
}

MediaPlayer::SupportsType MediaPlayerPrivatePlayStation::supportsType(const MediaEngineSupportParameters& parameters)
{
    INFO_LOG(LOGIDENTIFIER);
    String type = parameters.type.containerType();
    if (type.isNull() || type.isEmpty())
        return MediaPlayer::SupportsType::IsNotSupported;

    mediaplayer::PlayableStatus codecCheck = mediaplayer::PSMediaPlayerBackend::supportsType(parameters.type.raw().utf8().data(), parameters.isMediaSource);
    if (codecCheck == mediaplayer::PlayableStatus::PlayableProbably)
        return MediaPlayer::SupportsType::IsSupported;
    if (codecCheck == mediaplayer::PlayableStatus::PlayableMaybe)
        return MediaPlayer::SupportsType::MayBeSupported;
    return MediaPlayer::SupportsType::IsNotSupported;
}

bool MediaPlayerPrivatePlayStation::supportsKeySystem(const String&, const String&)
{
    return false;
}

void MediaPlayerPrivatePlayStation::callOnMainThreadIfNeeded(Function<void()>&& function)
{
    if (isMainThread()) {
        function();
        return;
    }

    callOnMainThread([function = WTFMove(function), weakThis = m_weakThis]() {
        if (!weakThis)
            return;

        function();
    });
}

void MediaPlayerPrivatePlayStation::loadInternal()
{
#if ENABLE(MEDIA_SOURCE)
    if (m_preload == MediaPlayer::Preload::None && !m_mediaSourcePrivateClient)
#else
    if (m_preload == MediaPlayer::Preload::None)
#endif
        m_isDelayingLoad = true;

    if (!m_isDelayingLoad)
        commitLoad();

    acceleratedRenderingStateChanged();
}

void MediaPlayerPrivatePlayStation::commitLoad()
{
    ASSERT(!m_isDelayingLoad);

    mediaplayer::PSMediaPlayerBackendInterface::MediaTagInfo tagInfo;
    tagInfo.url = m_url.utf8().data();
    tagInfo.mimeType = m_player->contentTypeRaw().utf8().data();
    tagInfo.userAgent = m_player->userAgent().utf8().data();
#if ENABLE(MEDIA_SOURCE)
    tagInfo.isMediaSource = m_mediaSourcePrivateClient ? true : false;
#else
    tagInfo.isMediaSource = false;
#endif
    tagInfo.isVideoTag = m_player->isVideoPlayer();
    auto* document = getDocumentPointer();
    tagInfo.isWebMusicPlayer = document ? document->settings().webMusicPlayerEnabled() : false;

    setMuted(m_player->muted());
    setVolumeDouble(m_player->volume());

    m_playstation->setMediaPlayable(m_isMediaPlayable);
    if (!m_playstation->load(tagInfo)) {
        m_player->readyStateChanged();
        m_player->networkStateChanged();
    }
}

void MediaPlayerPrivatePlayStation::setPauseIfEnded()
{
    if (ended()) {
        m_isPaused = true;
        updatePlayStatus();
    }
}

void MediaPlayerPrivatePlayStation::updatePlayStatus()
{
    bool isPaused = m_playstation->paused();

    if (paused() == isPaused)
        return;

    if (paused())
        m_playstation->pause();
    else {
        if (ended() && !m_player->isLooping())
            seek(MediaTime::zeroTime());
        m_playstation->setRate(m_playbackRate);
        m_playstation->play();
    }
}

bool MediaPlayerPrivatePlayStation::isMediaPlayable() const
{
    Document* document = getDocumentPointer();
    if (document && document->page())
        return document->page()->settings().mediaPlayable();
    return false;
}

Document* MediaPlayerPrivatePlayStation::getDocumentPointer() const
{
    CachedResourceLoader* cachedResourceLoader = m_player->cachedResourceLoader();
    if (cachedResourceLoader)
        return cachedResourceLoader->document();
    return nullptr;
}

#if ENABLE(MEDIA_SOURCE)
bool MediaPlayerPrivatePlayStation::isTimeBuffered(const MediaTime &time) const
{
    return (m_mediaSourcePrivateClient && m_mediaSourcePrivateClient->buffered().contain(time));
}

void MediaPlayerPrivatePlayStation::setMediaSourcePrivate(Ref<MediaSourcePrivatePlayStation> mediaSourcePrivate)
{
    m_mediaSourcePrivate = mediaSourcePrivate.ptr();
}

void MediaPlayerPrivatePlayStation::setReadyState(MediaPlayer::ReadyState state)
{
    mediaplayer::ReadyState readyState = mediaplayer::ReadyState::HaveNothing;
    switch (state) {
    case MediaPlayer::ReadyState::HaveNothing:     readyState = mediaplayer::ReadyState::HaveNothing;       break;
    case MediaPlayer::ReadyState::HaveMetadata:    readyState = mediaplayer::ReadyState::HaveMetaData;      break;
    case MediaPlayer::ReadyState::HaveCurrentData: readyState = mediaplayer::ReadyState::HaveCurrentData;   break;
    case MediaPlayer::ReadyState::HaveFutureData:  readyState = mediaplayer::ReadyState::HaveFutureData;    break;
    case MediaPlayer::ReadyState::HaveEnoughData:  readyState = mediaplayer::ReadyState::HaveEnoughData;    break;
    default: readyState = mediaplayer::ReadyState::HaveNothing;                                             break;
    }

    if (m_playstation->readyState() == readyState)
        return;

    m_playstation->setReadyState(readyState);
    m_player->readyStateChanged();
}
#endif

} // namespace WebCore

#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION)
