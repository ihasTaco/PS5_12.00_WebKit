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
#include "InbandTextTrackPrivatePlayStation.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

InbandTextTrackPrivatePlayStation::InbandTextTrackPrivatePlayStation(CueFormat format, mediaplayer::TextTrackBackend& textTrack)
    : InbandTextTrackPrivate(format)

{
    m_kind = convertToTextTrackPlayStation(textTrack.kind());
    m_label = AtomString::fromLatin1(textTrack.label().c_str());
    m_language = AtomString::fromLatin1(textTrack.language().c_str());
    m_id = AtomString::fromLatin1(textTrack.id().c_str());
    m_inBandMetadataTrackDispatchType = AtomString::fromLatin1(textTrack.inBandMetadataTrackDispatchType().c_str());
    m_trackIndex = textTrack.trackIndex();

}

InbandTextTrackPrivate::Kind InbandTextTrackPrivatePlayStation::convertToTextTrackPlayStation(mediaplayer::TextTrackKind textKind) const
{
    switch (textKind) {
    case mediaplayer::TextTrackKind::subtitles:     return Kind::Subtitles;
    case mediaplayer::TextTrackKind::captions:      return Kind::Captions;
    case mediaplayer::TextTrackKind::descriptions:  return Kind::Descriptions;
    case mediaplayer::TextTrackKind::chapters:      return Kind::Chapters;
    case mediaplayer::TextTrackKind::metadata:      return Kind::Metadata;
    default:                                        return Kind::None;
    };
}

}
#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
