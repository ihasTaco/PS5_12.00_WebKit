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
#include "MediaSourcePrivatePlayStation.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

#include "ContentType.h"
#include "MediaPlayerPrivatePlayStation.h"
#include "MediaSourcePrivateClient.h"
#include "SourceBufferPrivatePlayStation.h"
#include "TimeRanges.h"

#include <wtf/RefPtr.h>

namespace WebCore {

void MediaSourcePrivatePlayStation::open(MediaPlayerPrivatePlayStation& mediaPlayer, MediaSourcePrivateClient& client)
{
    switch (mediaPlayer.networkState()) {
    case MediaPlayer::NetworkState::FormatError:
    case MediaPlayer::NetworkState::NetworkError:
    case MediaPlayer::NetworkState::DecodeError:
        break;
    default:
        RefPtr<MediaSourcePrivatePlayStation> mediaSourcePrivate = adoptRef(*new MediaSourcePrivatePlayStation(client, mediaPlayer));
        client.setPrivateAndOpen(*mediaSourcePrivate);
        mediaPlayer.setMediaSourcePrivate(*mediaSourcePrivate);
        break;
    }
}

MediaSourcePrivatePlayStation::MediaSourcePrivatePlayStation(MediaSourcePrivateClient& client, MediaPlayerPrivatePlayStation& playerPrivate)
    : m_client(client)
    , m_playerPrivate(playerPrivate)
{
    ASSERT(playerPrivate.backend());
    m_playstation = mediaplayer::PSMediaSourceBackend::create(this, playerPrivate.backend());
}

MediaSourcePrivatePlayStation::~MediaSourcePrivatePlayStation()
{
    for (auto& sourceBufferPrivate : m_sourceBuffers)
        sourceBufferPrivate->clearMediaSource();

    m_playerPrivate.clearMediaSourcePrivateClient();
}

MediaSourcePrivate::AddStatus MediaSourcePrivatePlayStation::addSourceBuffer(const ContentType& contentType, bool webMParserEnabled, RefPtr<SourceBufferPrivate>& sourceBufferPrivate)
{
    UNUSED_PARAM(webMParserEnabled);

    sourceBufferPrivate = SourceBufferPrivatePlayStation::create(this, contentType);

    RefPtr<SourceBufferPrivatePlayStation> sourceBufferPrivatePlaystation = static_cast<SourceBufferPrivatePlayStation*>(sourceBufferPrivate.get());
    m_sourceBuffers.add(sourceBufferPrivatePlaystation);

    mediaplayer::AddStatus status = mediaplayer::AddStatus::Ok;
    status = m_playstation->addSourceBuffer(contentType.raw().utf8().data(), (static_cast<SourceBufferPrivatePlayStation*>(sourceBufferPrivate.get()))->backend());
    return convertTo(status);
}

void MediaSourcePrivatePlayStation::removeSourceBuffer(SourceBufferPrivate* sourceBufferPrivate)
{
    RefPtr<SourceBufferPrivatePlayStation> sourceBufferPrivatePlaystation = static_cast<SourceBufferPrivatePlayStation*>(sourceBufferPrivate);
    ASSERT(m_sourceBuffers.contains(sourceBufferPrivatePlaystation));

    sourceBufferPrivatePlaystation->clearMediaSource();
    m_sourceBuffers.remove(sourceBufferPrivatePlaystation);
}

void MediaSourcePrivatePlayStation::durationChanged(const MediaTime& duration)
{
    m_playstation->setDuration(duration.toDouble());
}

void MediaSourcePrivatePlayStation::markEndOfStream(EndOfStreamStatus status)
{
    m_playstation->markEndOfStream(convertTo(status));
    m_isEnded = true;
}

void MediaSourcePrivatePlayStation::unmarkEndOfStream()
{
    m_playstation->unmarkEndOfStream();
    m_isEnded = false;
}

MediaPlayer::ReadyState MediaSourcePrivatePlayStation::readyState() const
{
    return m_playerPrivate.readyState();
}

void MediaSourcePrivatePlayStation::setReadyState(MediaPlayer::ReadyState state)
{
    m_playerPrivate.setReadyState(state);
}

void MediaSourcePrivatePlayStation::waitForSeekCompleted()
{
    m_playstation->waitForSeekCompleted();
}

void MediaSourcePrivatePlayStation::seekCompleted()
{
    m_playstation->seekCompleted();
}

MediaTime MediaSourcePrivatePlayStation::duration() const
{
    return m_client ? m_client->duration() : MediaTime::invalidTime();
}

MediaTime MediaSourcePrivatePlayStation::currentMediaTime() const
{
    return m_playerPrivate.currentMediaTime();
}

void MediaSourcePrivatePlayStation::sourceBufferPrivateDidChangeActiveState(SourceBufferPrivatePlayStation* sourceBufferPrivate, bool isActive)
{
#if 0
    if (!isActive)
        m_activeSourceBuffers.remove(sourceBufferPrivate);
    else if (!m_activeSourceBuffers.contains(sourceBufferPrivate))
        m_activeSourceBuffers.add(sourceBufferPrivate);
#else
    UNUSED_PARAM(sourceBufferPrivate);
    UNUSED_PARAM(isActive);
#endif
}

void MediaSourcePrivatePlayStation::notifyAppendComplete()
{
    m_playstation->notifyAppendComplete();
}

void MediaSourcePrivatePlayStation::notifyDurationChanged()
{
    m_playstation->notifyDurationChanged();
}

// PSMediaSourceBackendClient
std::unique_ptr<mediaplayer::TimeRanges> MediaSourcePrivatePlayStation::buffered() const
{
    mediaplayer::TimeRanges timeRanges;

    if (!m_client)
        return nullptr;

    for (unsigned index = 0; m_client->buffered().length(); index++) {
        double start = m_client->buffered().start(index).toDouble();
        double end = m_client->buffered().end(index).toDouble();
        timeRanges.pushRange(start, end);
    }

    return std::unique_ptr<mediaplayer::TimeRanges>(&timeRanges);
}

void MediaSourcePrivatePlayStation::seekToTime(double time)
{
    m_client->seekToTime(MediaTime::createWithDouble(time));
}

void MediaSourcePrivatePlayStation::monitorSourceBuffers()
{
    if (m_client)
        m_client->monitorSourceBuffers();
}

void MediaSourcePrivatePlayStation::setReadyForMoreSamples(bool isReady)
{
    for (auto& sourceBufferPrivate : m_sourceBuffers)
        sourceBufferPrivate->setReadyForMoreSamples(isReady);
}

MediaSourcePrivate::AddStatus MediaSourcePrivatePlayStation::convertTo(mediaplayer::AddStatus addStatus)
{
    switch (addStatus) {
    case mediaplayer::AddStatus::Ok:                return MediaSourcePrivate::AddStatus::Ok;
    case mediaplayer::AddStatus::NotSupported:      return MediaSourcePrivate::AddStatus::NotSupported;
    case mediaplayer::AddStatus::ReachedIdLimit:    return MediaSourcePrivate::AddStatus::ReachedIdLimit;
    }
    ASSERT_NOT_REACHED();
    return MediaSourcePrivate::AddStatus::NotSupported;
}

mediaplayer::EndOfStreamStatus MediaSourcePrivatePlayStation::convertTo(MediaSourcePrivate::EndOfStreamStatus endOfStreamStatus)
{
    switch (endOfStreamStatus) {
    case MediaSourcePrivate::EosNoError:        return mediaplayer::EndOfStreamStatus::EosNoError;
    case MediaSourcePrivate::EosNetworkError:   return mediaplayer::EndOfStreamStatus::EosNetworkError;
    case MediaSourcePrivate::EosDecodeError:    return mediaplayer::EndOfStreamStatus::EosDecodeError;
    }
    ASSERT_NOT_REACHED();
    return mediaplayer::EndOfStreamStatus::EosDecodeError;
}

mediaplayer::ReadyState MediaSourcePrivatePlayStation::convertTo(MediaPlayer::ReadyState readyState)
{
    switch (readyState) {
    case MediaPlayer::ReadyState::HaveNothing:      return mediaplayer::ReadyState::HaveNothing;
    case MediaPlayer::ReadyState::HaveMetadata:     return mediaplayer::ReadyState::HaveMetaData;
    case MediaPlayer::ReadyState::HaveCurrentData:  return mediaplayer::ReadyState::HaveCurrentData;
    case MediaPlayer::ReadyState::HaveFutureData:   return mediaplayer::ReadyState::HaveFutureData;
    case MediaPlayer::ReadyState::HaveEnoughData:   return mediaplayer::ReadyState::HaveEnoughData;
    }
    ASSERT_NOT_REACHED();
    return mediaplayer::ReadyState::HaveNothing;
}

WebCore::MediaPlayer::ReadyState MediaSourcePrivatePlayStation::convertTo(mediaplayer::ReadyState readyState) const
{
    switch (readyState) {
    case mediaplayer::ReadyState::HaveNothing:      return MediaPlayer::ReadyState::HaveNothing;
    case mediaplayer::ReadyState::HaveMetaData:     return MediaPlayer::ReadyState::HaveMetadata;
    case mediaplayer::ReadyState::HaveCurrentData:  return MediaPlayer::ReadyState::HaveCurrentData;
    case mediaplayer::ReadyState::HaveFutureData:   return MediaPlayer::ReadyState::HaveFutureData;
    case mediaplayer::ReadyState::HaveEnoughData:   return MediaPlayer::ReadyState::HaveEnoughData;
    }
    ASSERT_NOT_REACHED();
    return MediaPlayer::ReadyState::HaveNothing;
}

}
#endif
