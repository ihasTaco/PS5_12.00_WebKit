/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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
#include "SourceBufferPrivatePlayStation.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

#include "AudioTrackPrivatePlayStation.h"
#include "ContentType.h"
#include "InbandTextTrackPrivatePlayStation.h"
#include "MediaDescriptionPlayStation.h"
#include "MediaPlayerPrivatePlayStation.h"
#include "MediaSample.h"
#include "MediaSamplePrivatePlayStation.h"
#include "MediaSourcePrivatePlayStation.h"
#include "NotImplemented.h"
#include "VideoTrackPrivatePlayStation.h"

namespace WebCore {

Ref<SourceBufferPrivatePlayStation> SourceBufferPrivatePlayStation::create(MediaSourcePrivatePlayStation* mediaSource, const ContentType& contentType)
{
    return adoptRef(*new SourceBufferPrivatePlayStation(mediaSource, contentType));
}

SourceBufferPrivatePlayStation::SourceBufferPrivatePlayStation(MediaSourcePrivatePlayStation* mediaSource, const ContentType& contentType)
    : m_weakThis(WeakPtr(this))
    , m_mediaSource(mediaSource)
    , m_type(contentType)
{
    m_playstation = mediaplayer::PSSourceBufferBackend::create(this);
}

void SourceBufferPrivatePlayStation::appendInternal(Ref<SharedBuffer>&& data)
{
    ASSERT(m_mediaSource);

    if (!m_playstation) {
        SourceBufferPrivate::appendCompleted(false, m_mediaSource ? m_mediaSource->isEnded() : true);
        return;
    }
    m_playstation->append(data->data(), data->size());
}

void SourceBufferPrivatePlayStation::resetParserStateInternal()
{
    if (m_playstation)
        m_playstation->resetParserState();
}

void SourceBufferPrivatePlayStation::removedFromMediaSource()
{
    if (m_mediaSource)
        m_mediaSource->removeSourceBuffer(this);

    if (m_playstation) {
        m_playstation->removedFromMediaSource();
        m_playstation.reset();
    }
}

MediaPlayer::ReadyState SourceBufferPrivatePlayStation::readyState() const
{
    return m_mediaSource->readyState();
}

void SourceBufferPrivatePlayStation::setReadyState(MediaPlayer::ReadyState state)
{
    m_mediaSource->setReadyState(state);
}

bool SourceBufferPrivatePlayStation::canSwitchToType(const ContentType& contentType)
{
    if (!m_playstation)
        return false;

    const std::string contentTypeStr(contentType.raw().utf8().data());
    return m_playstation->canSwitchToType(contentTypeStr);
}

void SourceBufferPrivatePlayStation::startChangingType()
{
    if (m_playstation) {
        // Set pending initialization segment for changeType flag to true.
        SourceBufferPrivate::startChangingType();
        m_playstation->startChangingType();
    }
}

void SourceBufferPrivatePlayStation::removeCodedFrames(const MediaTime& start, const MediaTime& end, const MediaTime& currentTime, bool isEnded, CompletionHandler<void()>&& completionHandler)
{
    SourceBufferPrivate::removeCodedFrames(start, end, currentTime, isEnded, WTFMove(completionHandler));
    if (m_playstation)
        m_playstation->finishRemovingSamples();
}

void SourceBufferPrivatePlayStation::flush(const AtomString& trackId)
{
    if (m_playstation) {
        std::string trackIdStr(trackId.string().utf8().data());
        m_playstation->flush(trackIdStr);
    }
}

void SourceBufferPrivatePlayStation::enqueueSample(Ref<MediaSample>&& sample, const AtomString& trackId)
{
    if (m_playstation) {
        std::string trackIdStr(trackId.string().utf8().data());
        m_playstation->enqueueSample(sample->platformSample().sample.psSample, trackIdStr);
    }
}

void SourceBufferPrivatePlayStation::allSamplesInTrackEnqueued(const AtomString& trackId)
{
    if (m_playstation) {
        std::string trackIdStr(trackId.string().utf8().data());
        m_playstation->allSamplesInTrackEnqueued(trackIdStr);
    }
}

bool SourceBufferPrivatePlayStation::isReadyForMoreSamples(const AtomString& trackId)
{
    if (!m_playstation)
        return false;

    std::string trackIdStr(trackId.string().utf8().data());
    return m_playstation->isReadyForMoreSamples(trackIdStr);
}

void SourceBufferPrivatePlayStation::setReadyForMoreSamples(bool isReady)
{
    ASSERT(WTF::isMainThread());
    if (m_playstation)
        m_playstation->setReadyForMoreSamples(isReady);
}

void SourceBufferPrivatePlayStation::setActive(bool isActive)
{
    m_isActive = isActive;

    if (m_mediaSource)
        m_mediaSource->sourceBufferPrivateDidChangeActiveState(this, isActive);
}

bool SourceBufferPrivatePlayStation::isActive() const
{
    return m_isActive;
}

void SourceBufferPrivatePlayStation::notifyClientWhenReadyForMoreSamples(const AtomString& trackId)
{
    ASSERT(WTF::isMainThread());
    if (m_playstation) {
        std::string trackIdStr(trackId.string().utf8().data());
        m_playstation->notifyClientWhenReadyForMoreSamples(trackIdStr);
    }
}

bool SourceBufferPrivatePlayStation::isSeeking() const
{
    return m_mediaSource && m_mediaSource->isSeeking();
}

MediaTime SourceBufferPrivatePlayStation::currentMediaTime() const
{
    return m_mediaSource ? m_mediaSource->currentMediaTime() : MediaTime::zeroTime();
}

MediaTime SourceBufferPrivatePlayStation::duration() const
{
    return m_mediaSource ? m_mediaSource->duration() : MediaTime::zeroTime();
}

void SourceBufferPrivatePlayStation::reenqueueMediaIfNeeded(const MediaTime& currentMediaTime)
{
    SourceBufferPrivate::reenqueueMediaIfNeeded(currentMediaTime);

    if (m_playstation) {
        m_playstation->finishSettingTimestampsOffset();
        m_playstation->finishRemovingSamples();
        m_playstation->finishEnqueuingSamples({ });
    }
}

void SourceBufferPrivatePlayStation::seekToTime(const MediaTime& time)
{
    SourceBufferPrivate::seekToTime(time);

    if (m_playstation) {
        m_playstation->finishSettingTimestampsOffset();
        m_playstation->finishEnqueuingSamples({ });
    }
}

// mediaplayer::PSSourceBufferBackendClient
void SourceBufferPrivatePlayStation::sourceBufferBackendDidReceiveInitializationSegment(std::unique_ptr<mediaplayer::PSSourceBufferBackendClient::InitializationSegment>&& segment)
{
    callOnMainThreadIfNeeded([this, segment = WTFMove(segment)]() {

        SourceBufferPrivateClient::InitializationSegment initSegment;

        initSegment.duration = MediaTime::createWithDouble(segment->duration);

        // audioTracks.
        for (unsigned long i = 0; i < segment->audioTracks.size(); i++) {
            SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation trackInfo;

            trackInfo.description = MediaDescriptionPlayStation::create(segment->audioTracks[i].description);
            trackInfo.track = AudioTrackPrivatePlayStation::create(WTFMove(segment->audioTracks[i].track));
            initSegment.audioTracks.append(trackInfo);
        }

        // videoTracks.
        for (unsigned long i = 0; i < segment->videoTracks.size(); i++) {
            SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation trackInfo;

            trackInfo.description = MediaDescriptionPlayStation::create(segment->videoTracks[i].description);
            trackInfo.track = VideoTrackPrivatePlayStation::create(WTFMove(segment->videoTracks[i].track));
            initSegment.videoTracks.append(trackInfo);
        }

        // textTracks.
        for (auto textTrack : segment->textTracks) {
            SourceBufferPrivateClient::InitializationSegment::TextTrackInformation trackInfo;

            trackInfo.description = MediaDescriptionPlayStation::create(textTrack.description);
            trackInfo.track = InbandTextTrackPrivatePlayStation::create(InbandTextTrackPrivate::CueFormat::Data, textTrack.track);
            initSegment.textTracks.append(trackInfo);
        }

        SourceBufferPrivate::didReceiveInitializationSegment(WTFMove(initSegment),
            [](auto&) {
                return true;
            },
            [](SourceBufferPrivateClient::ReceiveResult) { });
    });
}

void SourceBufferPrivatePlayStation::sourceBufferBackendDidReceiveSample(std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface>&& sample)
{
    RefPtr<MediaSamplePrivatePlayStation> mediaSample = WebCore::MediaSamplePrivatePlayStation::create(WTFMove(sample));

    callOnMainThreadIfNeeded([this, mediaSample = WTFMove(mediaSample)]() {
        SourceBufferPrivate::didReceiveSample(*mediaSample);
    });
}

bool SourceBufferPrivatePlayStation::sourceBufferBackendHasAudio() const
{
    return false;
}

bool SourceBufferPrivatePlayStation::sourceBufferBackendHasVideo() const
{
    return false;
}

void SourceBufferPrivatePlayStation::sourceBufferBackendReenqueSamples(const std::string&)
{
}

void SourceBufferPrivatePlayStation::sourceBufferBackendDidBecomeReadyForMoreSamples(const std::string& trackID)
{
    callOnMainThreadIfNeeded([this, trackID]() {
        SourceBufferPrivate::provideMediaData(AtomString::fromLatin1(trackID.c_str()));

        if (m_playstation) {
            m_playstation->finishSettingTimestampsOffset();
            m_playstation->finishEnqueuingSamples(trackID);
        }
    });
}


double SourceBufferPrivatePlayStation::sourceBufferBackendFastSeekTimeForMediaTime(double targetTime, double, double)
{
    return targetTime;
}

void SourceBufferPrivatePlayStation::sourceBufferBackendSeekToTime(double)
{
}

void SourceBufferPrivatePlayStation::sourceBufferBackendAppendComplete(mediaplayer::AppendResult result)
{
    if (m_mediaSource)
        m_mediaSource->notifyAppendComplete();

    callOnMainThreadIfNeeded([this, result]() {
        SourceBufferPrivate::appendCompleted(result == mediaplayer::AppendResult::AppendSucceeded ? true : false, m_mediaSource ? m_mediaSource->isEnded() : true);
    });
}

void SourceBufferPrivatePlayStation::sourceBufferBackendDidReceiveRenderingError(mediaplayer::ErrorCode errorCode)
{
    callOnMainThreadIfNeeded([this, errorCode]() {
        if (m_client)
            m_client->sourceBufferPrivateDidReceiveRenderingError(static_cast<int>(errorCode));
    });
}

void SourceBufferPrivatePlayStation::callOnMainThreadIfNeeded(Function<void()>&& function)
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

}
#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
