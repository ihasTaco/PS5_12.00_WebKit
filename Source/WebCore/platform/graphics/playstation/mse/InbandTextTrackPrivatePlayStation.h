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

#include "InbandTextTrackPrivate.h"

#include <mediaplayer/PSEnumBackend.h>
#include <mediaplayer/PSTrackBackend.h>
#include <wtf/text/AtomString.h>

namespace WebCore {

class InbandTextTrackPrivatePlayStation : public InbandTextTrackPrivate {
public:
    static RefPtr<InbandTextTrackPrivatePlayStation> create(CueFormat format, mediaplayer::TextTrackBackend& textTrack)
    {
        return adoptRef(*new InbandTextTrackPrivatePlayStation(format, textTrack));
    }

    Kind kind() const override { return m_kind; }
    AtomString label() const override { return m_label; }
    AtomString language() const override { return m_language; }
    AtomString id() const override { return m_id; }
    AtomString inBandMetadataTrackDispatchType() const override { return m_inBandMetadataTrackDispatchType; }

private:
    InbandTextTrackPrivatePlayStation(CueFormat, mediaplayer::TextTrackBackend&);
    ~InbandTextTrackPrivatePlayStation() = default;

    Kind convertToTextTrackPlayStation(mediaplayer::TextTrackKind) const;

private:
    Kind m_kind { Kind::None };
    AtomString m_label;
    AtomString m_language;
    AtomString m_id;
    AtomString m_inBandMetadataTrackDispatchType;
    int m_trackIndex { 0 };
};

}

#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
