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

#pragma once

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION)

#include "CallbackQueuePlayStation.h"
#include "MediaPlayerPrivate.h"
#include "TextureMapperPlatformLayer.h"
#include <mediaplayer/PSMediaPlayerBackend.h>
#include <wtf/WeakPtr.h>

#if !(USE(NICOSIA) && USE(TEXTURE_MAPPER_GL))
#error "MediaPlayerPrivatePlayStation supports only NICOSIA and TEXTURE_MAPPER_GL"
#endif

#include "NicosiaContentLayerTextureMapperImpl.h"

namespace WebCore {

class Document;
class MediaPlayer;
#if ENABLE(MEDIA_SOURCE)
class MediaSourcePrivateClient;
class MediaSourcePrivatePlayStation;
#endif
class GraphicsContext;
#if ENABLE(ENCRYPTED_MEDIA)
class CDMInstancePlayStation;
#endif

class MediaPlayerPrivatePlayStation : public MediaPlayerPrivateInterface
    , public CanMakeWeakPtr<MediaPlayerPrivatePlayStation>
    , public mediaplayer::PSMediaPlayerBackendClient
    , public mediaplayer::PSGraphicsContextClient
#if ENABLE(ENCRYPTED_MEDIA)
    , public mediaplayer::PSCDMInstanceClient
#endif
    , public Nicosia::ContentLayerTextureMapperImpl::Client
{
public:
    explicit MediaPlayerPrivatePlayStation(MediaPlayer*);
    virtual ~MediaPlayerPrivatePlayStation();

    static void registerMediaEngine(MediaEngineRegistrar);

    void load(const String&) override;
#if ENABLE(MEDIA_SOURCE)
    void load(const URL&, const ContentType&, MediaSourcePrivateClient&) final;
#endif
#if ENABLE(MEDIA_STREAM)
    void load(MediaStreamPrivate&) override;
#endif
    void cancelLoad() override;

    void prepareToPlay() override;
    PlatformLayer* platformLayer() const override;

    long platformErrorCode() const override;

    void play() override;
    void pause() override;
    void setBufferingPolicy(MediaPlayer::BufferingPolicy) override;

    bool supportsPictureInPicture() const override;
    bool supportsFullscreen() const override;
    bool supportsScanning() const override;
    bool requiresImmediateCompositing() const override;

    bool canSaveMediaData() const override;

    FloatSize naturalSize() const override;

    bool hasVideo() const override;
    bool hasAudio() const override;

    void setPageIsVisible(bool) final;

    float duration() const override;
    double durationDouble() const override;
    MediaTime durationMediaTime() const override;

    float currentTime() const override;
    double currentTimeDouble() const override;
    MediaTime currentMediaTime() const override;

    MediaTime getStartDate() const override;

    void seek(float) override;
    void seekDouble(double) override;
    void seek(const MediaTime&) override;
    void seekWithTolerance(const MediaTime&, const MediaTime&, const MediaTime&) override;

    bool seeking() const override;

    MediaTime startTime() const override;
    MediaTime initialTime() const override;

    void setRate(float) override;
    void setRateDouble(double) override;
    double rate() const override;

    void setPreservesPitch(bool) override;

    bool paused() const override;

    void setVolume(float) override;
    void setVolumeDouble(double) override;

    void setMuted(bool) override;

    bool hasClosedCaptions() const override;
    void setClosedCaptionsVisible(bool) override;

    double maxFastForwardRate() const override;
    double minFastReverseRate() const override;

    MediaPlayer::NetworkState networkState() const override;
    MediaPlayer::ReadyState readyState() const override;

    const PlatformTimeRanges& seekable() const override;
    float maxTimeSeekable() const override;
    MediaTime maxMediaTimeSeekable() const override;
    double minTimeSeekable() const override;
    MediaTime minMediaTimeSeekable() const override;
    const PlatformTimeRanges& buffered() const override;
    double seekableTimeRangesLastModifiedTime() const override;
    double liveUpdateInterval() const override;

    unsigned long long totalBytes() const override;
    bool didLoadingProgress() const override;

    void setPresentationSize(const IntSize&) override;

    void paint(GraphicsContext&, const FloatRect&) override;

    DestinationColorSpace colorSpace() override;

    void setPreload(MediaPlayer::Preload) override;

    bool hasAvailableVideoFrame() const override;

#if USE(NATIVE_FULLSCREEN_VIDEO)
    void enterFullscreen() override;
    void exitFullscreen() override;
#endif

#if USE(NATIVE_FULLSCREEN_VIDEO)
    bool canEnterFullscreen() const override;
#endif

    // whether accelerated rendering is supported by the media engine for the current media.
    bool supportsAcceleratedRendering() const override;
    // called when the rendering system flips the into or out of accelerated rendering mode.
    void acceleratedRenderingStateChanged() override;

    void setShouldMaintainAspectRatio(bool) override;

    bool didPassCORSAccessCheck() const override;

    MediaPlayer::MovieLoadType movieLoadType() const override;

    void prepareForRendering() override;

    // Time value in the movie's time scale. It is only necessary to override this if the media
    // engine uses rational numbers to represent media time.
    MediaTime mediaTimeForTimeValue(const MediaTime& timeValue) const override;

    // Overide this if it is safe for HTMLMediaElement to cache movie time and report
    // 'currentTime' as [cached time + elapsed wall time]. Returns the maximum wall time
    // it is OK to calculate movie time before refreshing the cached time.
    double maximumDurationToCacheMediaTime() const override;

    unsigned decodedFrameCount() const override;
    unsigned droppedFrameCount() const override;
    unsigned audioDecodedByteCount() const override;
    unsigned videoDecodedByteCount() const override;

    // not support Media Cache
    // HashSet<RefPtr<SecurityOrigin>> originsInMediaCache(const String&);
    // void clearMediaCache(const String&, std::chrono::system_clock::time_point);
    // void clearMediaCacheForOrigins(const String&, const HashSet<RefPtr<SecurityOrigin>>&) ;

    void setPrivateBrowsingMode(bool) override;

    String engineDescription() const override;

#if ENABLE(WEB_AUDIO)
    AudioSourceProvider* audioSourceProvider() override;
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    void cdmInstanceAttached(CDMInstance&) override;
    void cdmInstanceDetached(CDMInstance&) override;
    void attemptToDecryptWithInstance(CDMInstance&) override;
    bool waitingForKey() const override;
#endif

    bool requiresTextTrackRepresentation() const override;
    void setTextTrackRepresentation(TextTrackRepresentation*) override;
    void syncTextTrackBounds() override;
    void tracksChanged() override;

    String languageOfPrimaryAudioTrack() const override;

    size_t extraMemoryCost() const override;

    unsigned long long fileSize() const override;

    bool ended() const override;

#if ENABLE(MEDIA_SOURCE)
    std::optional<VideoPlaybackQualityMetrics> videoPlaybackQualityMetrics() override;
#endif

    void notifyActiveSourceBuffersChanged() override;

    void setShouldDisableSleep(bool) override;

    void applicationWillResignActive() override;
    void applicationDidBecomeActive() override;

    bool performTaskAtMediaTime(Function<void()>&&, const MediaTime&) override;

    bool shouldIgnoreIntrinsicSize() override;

    void setPreferredDynamicRangeMode(DynamicRangeMode) override;

    uint32_t canvasHandle() const;

    // interval timer for PS4
    void updateStatusTimerFired();

    // madiaplayer::PSMediaPlayerBackendClient
    void networkStateChanged() override;
    void readyStateChanged() override;
    void volumeChanged(double) override;
    void muteChanged(bool) override;
    void timeChanged() override;
    void sizeChanged() override;
    void rateChanged() override;
    void playbackStateChanged() override;
    void durationChanged() override;
    void firstVideoFrameAvailable() override;
    void characteristicChanged() override;
    void repaint() override;

#if ENABLE(ENCRYPTED_MEDIA)
    void initializationDataEncountered(const std::string&, const std::vector<uint8_t>&) override;
    void waitingForKeyChanged() override;
#endif

    void seeked() override;
    void updateCanvasHandle(uint32_t canvasHandle) override;
#if ENABLE(MEDIA_SOURCE)
    void mediaSourceOpen() override;
#endif

    // mediaplayer::PSGraphicsContextClient
    bool paintingDisabled() const override;

    // mediaplayer::PSCDMInstanceClient
#if ENABLE(ENCRYPTED_MEDIA)
    std::string keySystem() const override;
#endif

    // NicosiaContentLayerTextureMapperImpl
    void swapBuffersIfNeeded() final;

#if ENABLE(MEDIA_SOURCE)
    mediaplayer::PSMediaPlayerBackendInterface* backend() { return m_playstation.get(); }
    void setMediaSourcePrivate(Ref<MediaSourcePrivatePlayStation>);
    void clearMediaSourcePrivateClient() { m_mediaSourcePrivateClient = nullptr; }
    void setReadyState(MediaPlayer::ReadyState);
#endif

#if ENABLE(PUNCH_HOLE)
    bool needsSwapBuffersOnCompositingThread() override { return true; }
#endif

private:
    friend class MediaPlayerFactoryPlayStation;
    // engine support
    static void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);
    static bool supportsKeySystem(const String& keySystem, const String& mimeType);

    CallbackQueuePlayStation<Function<void()>>& performTaskAtMediaTimeCallbackQueue() { return m_performTaskAtMediaTimeCallbackQueue; }

    void callOnMainThreadIfNeeded(Function<void()>&&);

    void loadInternal();
    void commitLoad();
    void setPauseIfEnded();
    void updatePlayStatus();
    bool isMediaPlayable() const;
    Document* getDocumentPointer() const;
#if ENABLE(MEDIA_SOURCE)
    bool isTimeBuffered(const MediaTime&) const;
#endif

#if ENABLE(PUNCH_HOLE)
    void pushNextHolePunchBuffer();
#endif
    void ensureLayerForAcceleratedCompositing();
    void destroyLayerForAcceleratedCompositing();

private:
    WeakPtr<MediaPlayerPrivatePlayStation> m_weakThis;
    MediaPlayer* m_player;
    std::shared_ptr<mediaplayer::PSMediaPlayerBackendInterface> m_playstation;

    Timer m_updateStatusTimer;
    String m_url;
    bool m_isDelayingLoad;
    MediaPlayer::Preload m_preload;
    bool m_isPaused;
    double m_playbackRate;
    double m_volume;
    bool m_isMuted;
    bool m_isSeeking;
    MediaTime m_seekTime;
    bool m_isMediaPlayable;

    CallbackQueuePlayStation<Function<void()>> m_performTaskAtMediaTimeCallbackQueue;

    uint32_t m_canvasHandle;

#if ENABLE(ENCRYPTED_MEDIA)
    RefPtr<const CDMInstancePlayStation> m_cdmInstance;
#endif

#if ENABLE(MEDIA_SOURCE)
    WeakPtr<MediaSourcePrivateClient> m_mediaSourcePrivateClient;
    RefPtr<MediaSourcePrivatePlayStation> m_mediaSourcePrivate;
#endif

    RefPtr<Nicosia::ContentLayer> m_nicosiaLayer;

    mutable PlatformTimeRanges m_buffered;
};

} // namespace WebCore

#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION)
