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

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

#include "ContentType.h"
#include "MediaPlayerPrivatePlayStation.h"
#include "SourceBufferPrivate.h"
#include "SourceBufferPrivateClient.h"

#include <mediaplayer/PSMediaSampleBackend.h>
#include <mediaplayer/PSSourceBufferBackend.h>
#include <mediaplayer/PSSourceBufferBackendClient.h>

#include <wtf/WeakPtr.h>

namespace WebCore {

class MediaSourcePrivatePlayStation;

class SourceBufferPrivatePlayStation final : public SourceBufferPrivate, public mediaplayer::PSSourceBufferBackendClient {
public:
    static Ref<SourceBufferPrivatePlayStation> create(MediaSourcePrivatePlayStation*, const ContentType&);
    virtual ~SourceBufferPrivatePlayStation() = default;

    void clearMediaSource() { m_mediaSource = nullptr; }

    void appendInternal(Ref<SharedBuffer>&&) final;
    void resetParserStateInternal() final;
    void removedFromMediaSource() final;
    MediaPlayer::ReadyState readyState() const final;
    void setReadyState(MediaPlayer::ReadyState) final;

    bool canSwitchToType(const ContentType&) final;
    void startChangingType() final;

    void removeCodedFrames(const MediaTime&, const MediaTime&, const MediaTime&, bool, CompletionHandler<void()>&& = [] { }) final;

    void flush(const AtomString&) final;
    void enqueueSample(Ref<MediaSample>&&, const AtomString&) final;
    void allSamplesInTrackEnqueued(const AtomString&) final;
    bool isReadyForMoreSamples(const AtomString&) final;
    void setActive(bool) final;
    bool isActive() const final;
    void notifyClientWhenReadyForMoreSamples(const AtomString&) final;
    bool isSeeking() const final;
    MediaTime currentMediaTime() const final;
    MediaTime duration() const final;

    void reenqueueMediaIfNeeded(const MediaTime&) final;
    void seekToTime(const MediaTime&) final;

    void setReadyForMoreSamples(bool);

    ContentType type() const { return m_type; }

    // mediaplayer::PSSourceBufferBackendClient
    void sourceBufferBackendDidReceiveInitializationSegment(std::unique_ptr<mediaplayer::PSSourceBufferBackendClient::InitializationSegment>&&) override;
    void sourceBufferBackendDidReceiveSample(std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface>&&) override;
    bool sourceBufferBackendHasAudio() const override;
    bool sourceBufferBackendHasVideo() const override;

    void sourceBufferBackendReenqueSamples(const std::string& trackID) override;
    void sourceBufferBackendDidBecomeReadyForMoreSamples(const std::string& trackID) override;

    double sourceBufferBackendFastSeekTimeForMediaTime(double, double, double) override;
    void sourceBufferBackendSeekToTime(double) override;

    void sourceBufferBackendAppendComplete(mediaplayer::AppendResult) override;
    void sourceBufferBackendDidReceiveRenderingError(mediaplayer::ErrorCode) override;

    std::shared_ptr<mediaplayer::PSSourceBufferBackendInterface> backend() { return m_playstation; }

private:
    SourceBufferPrivatePlayStation(MediaSourcePrivatePlayStation*, const ContentType&);

    void callOnMainThreadIfNeeded(Function<void()>&&);

private:
    WeakPtr<SourceBufferPrivatePlayStation> m_weakThis;

    std::shared_ptr<mediaplayer::PSSourceBufferBackendInterface> m_playstation;

    MediaSourcePrivatePlayStation* m_mediaSource;
    bool m_isActive { };
    ContentType m_type;
};

}

#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
