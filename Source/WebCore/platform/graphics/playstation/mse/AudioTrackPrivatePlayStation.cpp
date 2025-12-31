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
#include "AudioTrackPrivatePlayStation.h"

#if ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

AudioTrackPrivatePlayStation::AudioTrackPrivatePlayStation(std::unique_ptr<mediaplayer::AudioTrackBackend> audioTrack)
{
    m_playstation = WTFMove(audioTrack);
}

AudioTrackPrivate::Kind AudioTrackPrivatePlayStation::convertToAudioTrackPlayStation(std::string strKind) const
{
    AudioTrackPrivate::Kind kind = None;

    if (strKind == "alternative")
        kind = Alternative;
    else if (strKind == "descriptions")
        kind = Description;
    else if (strKind == "main")
        kind = Main;
    else if (strKind == "main-desc")
        kind = MainDesc;
    else if (strKind == "translation")
        kind = Translation;
    else if (strKind == "commentary")
        kind = Commentary;
    else
        kind = None;

    return kind;
}

void AudioTrackPrivatePlayStation::setEnabled(bool enabled)
{
    if (this->enabled() == enabled)
        return;

    if (m_playstation) {
        m_playstation->setEnabled(enabled);
        if (client())
            client()->enabledChanged(this->enabled());
    }
}

}
#endif // ENABLE(VIDEO) && PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)
