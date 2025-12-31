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

#include "MediaSample.h"

#include <mediaplayer/PSMediaSampleBackend.h>

namespace WebCore {

class PSMediaSampleBackendInterface;

class MediaSamplePrivatePlayStation final : public MediaSample {
public:
    static Ref<MediaSamplePrivatePlayStation> create(std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface> sample)
    {
        return adoptRef(*new MediaSamplePrivatePlayStation(WTFMove(sample)));
    }
    ~MediaSamplePrivatePlayStation() = default;

    MediaTime presentationTime() const final;
    MediaTime decodeTime() const final;
    MediaTime duration() const final;
    AtomString trackID() const final { return AtomString(m_trackId); };
    size_t sizeInBytes() const final;
    FloatSize presentationSize() const final;
    void offsetTimestampsBy(const MediaTime&) final;
    void setTimestamps(const MediaTime&, const MediaTime&) final;
    bool isDivisable() const final;
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime, UseEndTime) final;
    Ref<MediaSample> createNonDisplayingCopy() const final;

    SampleFlags flags() const final;
    PlatformSample platformSample() const final;
    PlatformSample::Type platformSampleType() const final { return PlatformSample::PSMediaSampleType; }
    std::optional<ByteRange> byteRange() const final { return std::nullopt; }

    void dump(PrintStream&) const final;

private:
    MediaSamplePrivatePlayStation(std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface>);

private:
    std::unique_ptr<mediaplayer::PSMediaSampleBackendInterface> m_playstation;

    MediaTime m_pts;
    MediaTime m_dts;
    MediaTime m_duration;
    String m_trackId;
};

}

#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
