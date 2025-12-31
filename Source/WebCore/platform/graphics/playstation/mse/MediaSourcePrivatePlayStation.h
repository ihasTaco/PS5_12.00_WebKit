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

#include "MediaSourcePrivate.h"

#include <mediaplayer/PSMediaSourceBackend.h>

namespace WebCore {

class SourceBufferPrivatePlayStation;
class MediaSourceClientPlayStation;
class MediaPlayerPrivatePlayStation;
class PlatformTimeRanges;

class MediaSourcePrivatePlayStation final : public MediaSourcePrivate
    , public mediaplayer::PSMediaSourceBackendClient {
public:
    static void open(MediaPlayerPrivatePlayStation&, MediaSourcePrivateClient&);
    virtual ~MediaSourcePrivatePlayStation();

    AddStatus addSourceBuffer(const ContentType&, bool webMParserEnabled, RefPtr<SourceBufferPrivate>&) override;
    void removeSourceBuffer(SourceBufferPrivate*);
    void durationChanged(const MediaTime&) override;
    void markEndOfStream(EndOfStreamStatus) override;
    void unmarkEndOfStream() override;
    bool isEnded() const override { return m_isEnded; }

    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

    void waitForSeekCompleted() override;
    void seekCompleted() override;

    MediaTime duration() const;
    MediaTime currentMediaTime() const;

    void sourceBufferPrivateDidChangeActiveState(SourceBufferPrivatePlayStation*, bool);

    void notifyAppendComplete();
    void notifyDurationChanged();

    // PSMediaSourceBackendClient
    std::unique_ptr<mediaplayer::TimeRanges> buffered() const override;
    void seekToTime(double) override;
    void monitorSourceBuffers() override;

    void setReadyForMoreSamples(bool);

private:
    MediaSourcePrivatePlayStation(MediaSourcePrivateClient&, MediaPlayerPrivatePlayStation&);
    MediaSourcePrivate::AddStatus convertTo(mediaplayer::AddStatus);
    mediaplayer::EndOfStreamStatus convertTo(MediaSourcePrivate::EndOfStreamStatus);
    mediaplayer::ReadyState convertTo(MediaPlayer::ReadyState);
    MediaPlayer::ReadyState convertTo(mediaplayer::ReadyState) const;

private:
    WeakPtr<MediaSourcePrivateClient> m_client;
    MediaPlayerPrivatePlayStation& m_playerPrivate;
    std::shared_ptr<mediaplayer::PSMediaSourceBackendInterface> m_playstation;
    bool m_isEnded { };

    HashSet<RefPtr<SourceBufferPrivatePlayStation>> m_sourceBuffers;
};

}

#endif
