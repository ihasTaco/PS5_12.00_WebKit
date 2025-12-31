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

#pragma once

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

#include "VideoTrackPrivate.h"

#include <mediaplayer/PSTrackBackend.h>
#include <wtf/text/AtomString.h>

namespace WebCore {

class VideoTrackPrivatePlayStation : public VideoTrackPrivate {
public:
    static RefPtr<VideoTrackPrivatePlayStation> create(std::unique_ptr<mediaplayer::VideoTrackBackend> videoTrack)
    {
        return adoptRef(*new VideoTrackPrivatePlayStation(WTFMove(videoTrack)));
    }

    AtomString id() const override { return AtomString::fromLatin1(m_playstation ? m_playstation->id().c_str() : ""); }
    Kind kind() const override { return convertToVideoTrackPlayStation(m_playstation ? m_playstation->kind() : ""); }
    AtomString label() const override { return AtomString::fromLatin1(m_playstation ? m_playstation->label().c_str() : ""); }
    AtomString language() const override { return AtomString::fromLatin1(m_playstation ? m_playstation->language().c_str() : ""); }
    bool selected() const override { return (m_playstation ? m_playstation->isSelected() : false); }
    void setSelected(bool) override;
    int trackIndex() const override { return (m_playstation ? m_playstation->trackIndex() : 0); }

private:
    VideoTrackPrivatePlayStation(std::unique_ptr<mediaplayer::VideoTrackBackend>);
    ~VideoTrackPrivatePlayStation() = default;

    Kind convertToVideoTrackPlayStation(std::string) const;

private:
    std::unique_ptr<mediaplayer::VideoTrackBackend> m_playstation;
};

}

#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
