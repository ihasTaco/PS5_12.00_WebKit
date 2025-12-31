/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPunchHole.h"

#if ENABLE(PUNCH_HOLE)

using namespace WebCore;

namespace WebKit {

void WebPunchHole::encode(IPC::Encoder& encoder) const
{
    encoder << m_data.quad;
    encoder << m_data.opacity;
    encoder << m_data.canvasHandle;
    encoder << m_data.flags;
}

std::optional<WebPunchHole> WebPunchHole::decode(IPC::Decoder& decoder)
{
    WebPunchHole result;
    if (!decoder.decode(result.m_data.quad))
        return std::nullopt;
    if (!decoder.decode(result.m_data.opacity))
        return std::nullopt;
    if (!decoder.decode(result.m_data.canvasHandle))
        return std::nullopt;
    if (!decoder.decode(result.m_data.flags))
        return std::nullopt;

    return WTFMove(result);
}

} // namespace WebKit
#endif
