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
#include "WebPasteboardProxy.h"

#include <WebCore/NotImplemented.h>
#include <wtf/CompletionHandler.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebPasteboardProxy::getPasteboardTypes(CompletionHandler<void(Vector<String>&&)>&&)
{
    notImplemented();
}

void WebPasteboardProxy::readStringFromPasteboard(IPC::Connection&, size_t index, const String& pasteboardType, const String& pasteboardName, std::optional<WebCore::PageIdentifier>, CompletionHandler<void(String&&)>&& completionHandler)
{
    notImplemented();
    completionHandler({ });
}

void WebPasteboardProxy::writeWebContentToPasteboard(const WebCore::PasteboardWebContent&)
{
    notImplemented();
}

void WebPasteboardProxy::writeStringToPasteboard(const String& pasteboardType, const String&)
{
    notImplemented();
}

} // namespace WebKit
