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
#include "VideoTrackPrivatePlayStation.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

VideoTrackPrivatePlayStation::VideoTrackPrivatePlayStation(std::unique_ptr<mediaplayer::VideoTrackBackend> videoTrack)
{
    m_playstation = WTFMove(videoTrack);
}

VideoTrackPrivate::Kind VideoTrackPrivatePlayStation::convertToVideoTrackPlayStation(std::string strKind) const
{
    VideoTrackPrivate::Kind kind = None;

    if (strKind == "alternative")
        kind = Alternative;
    else if (strKind == "captions")
        kind = Captions;
    else if (strKind == "main")
        kind = Main;
    else if (strKind == "sign")
        kind = Sign;
    else if (strKind == "subtitles")
        kind = Subtitles;
    else if (strKind == "commentary")
        kind = Commentary;
    else
        kind = None;

    return kind;
}

void VideoTrackPrivatePlayStation::setSelected(bool selected)
{
    if (this->selected() == selected)
        return;

    if (m_playstation) {
        m_playstation->setSelected(selected);
        if (client())
            client()->selectedChanged(this->selected());
    }
}

}
#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
