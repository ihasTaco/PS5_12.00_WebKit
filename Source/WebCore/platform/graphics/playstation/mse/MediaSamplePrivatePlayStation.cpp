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
#include "MediaSamplePrivatePlayStation.h"

#include "NotImplemented.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

MediaSamplePrivatePlayStation::MediaSamplePrivatePlayStation(std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface> backend)
    : m_playstation(WTFMove(backend))
{
    m_pts = MediaTime::createWithDouble(m_playstation->presentationTime(), MediaTime::DefaultTimeScale);
    m_dts = MediaTime::createWithDouble(m_playstation->decodeTime(), MediaTime::DefaultTimeScale);
    m_duration = MediaTime::createWithDouble(m_playstation->duration(), MediaTime::DefaultTimeScale);
    m_trackId = String::fromLatin1(m_playstation->trackID().c_str());
}

MediaTime MediaSamplePrivatePlayStation::presentationTime() const
{
    return m_pts;
}

MediaTime MediaSamplePrivatePlayStation::decodeTime() const
{
    return m_dts;
}

MediaTime MediaSamplePrivatePlayStation::duration() const
{
    return m_duration;
}

size_t MediaSamplePrivatePlayStation::sizeInBytes() const
{
    return m_playstation->sizeInBytes();
}

FloatSize MediaSamplePrivatePlayStation::presentationSize() const
{
    mediaplayer::FloatSize size = m_playstation->presentationSize();
    return FloatSize(size.m_width, size.m_height);
}

void MediaSamplePrivatePlayStation::offsetTimestampsBy(const MediaTime& newOffset)
{
    m_pts += newOffset;
    m_dts += newOffset;
    m_playstation->setTimestamps(m_pts.toDouble(), m_dts.toDouble());
}

void MediaSamplePrivatePlayStation::setTimestamps(const MediaTime& pts, const MediaTime& dts)
{
    m_pts = pts;
    m_dts = dts;
    m_playstation->setTimestamps(m_pts.toDouble(), m_dts.toDouble());
}

bool MediaSamplePrivatePlayStation::isDivisable() const
{
    return m_playstation->isDivisable();
}

std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> MediaSamplePrivatePlayStation::divide(const MediaTime& presentationTime, UseEndTime)
{
    std::pair<std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface>, std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface>> backends = m_playstation->divide(presentationTime.toDouble());

    RefPtr<MediaSample> first;
    RefPtr<MediaSample> second;
    if (backends.first)
        first = MediaSamplePrivatePlayStation::create(WTFMove(backends.first));
    if (backends.second)
        second = MediaSamplePrivatePlayStation::create(WTFMove(backends.second));

    return std::make_pair(first, second);
}

Ref<MediaSample> MediaSamplePrivatePlayStation::createNonDisplayingCopy() const
{
    std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface> backend = m_playstation->createNonDisplayingCopy(m_playstation.get());
    return adoptRef(*new MediaSamplePrivatePlayStation(WTFMove(backend)));
}

MediaSample::SampleFlags MediaSamplePrivatePlayStation::flags() const
{
    mediaplayer::PSMediaSampleBackendInterface::SampleFlags psflags = m_playstation->flags();

    SampleFlags flags = None;
    if (psflags & mediaplayer::PSMediaSampleBackendInterface::SampleFlags::IsSync)
        flags = static_cast<SampleFlags>(flags | IsSync);
    if (psflags & mediaplayer::PSMediaSampleBackendInterface::SampleFlags::IsNonDisplaying)
        flags = static_cast<SampleFlags>(flags | IsNonDisplaying);
    if (psflags & mediaplayer::PSMediaSampleBackendInterface::SampleFlags::HasAlpha)
        flags = static_cast<SampleFlags>(flags | HasAlpha);

    return flags;
}

PlatformSample MediaSamplePrivatePlayStation::platformSample() const
{
    PlatformSample sample = { PlatformSample::PSMediaSampleType, { .psSample = reinterpret_cast<PSMediaSampleRef>(m_playstation.get()) } };
    return sample;
}

void MediaSamplePrivatePlayStation::dump(PrintStream& out) const
{
    out.print("{PTS(", presentationTime(), "), DTS(", decodeTime(), "), duration(", duration(), "), flags(");

    bool anyFlags = false;
    auto appendFlag = [&out, &anyFlags](const char* flagName) {
        if (anyFlags)
            out.print(",");
        out.print(flagName);
        anyFlags = true;
    };

    if (flags() & MediaSample::IsSync)
        appendFlag("sync");
    if (flags() & MediaSample::IsNonDisplaying)
        appendFlag("non-displaying");
    if (flags() & MediaSample::HasAlpha)
        appendFlag("has-alpha");
    if (flags() & ~(MediaSample::IsSync | MediaSample::IsNonDisplaying | MediaSample::HasAlpha))
        appendFlag("unknown-flag");

    out.print("), trackId(", trackID().string(), "), presentationSize(", presentationSize().width(), "x", presentationSize().height(), "), sizeInBytes(", sizeInBytes(), ")}");
}

}
#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
