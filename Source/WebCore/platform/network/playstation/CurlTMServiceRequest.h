/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
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

#pragma once

#if PLATFORM(PLAYSTATION) // for downstream-only

#include "CurlRequest.h"
#include "CurlRequestClient.h"

namespace WebCore {

class CurlResponse;
class ResourceError;
class ResourceResponse;
class SharedBuffer;

class CurlTMServiceRequest : public CurlRequest, public CurlRequestClient {
    WTF_MAKE_NONCOPYABLE(CurlTMServiceRequest);
public:
    static Ref<CurlRequest> create(const ResourceRequest&, CurlRequestClient&, CaptureNetworkLoadMetrics captureMetrics = CaptureNetworkLoadMetrics::Basic, bool isMainResource = false);

    void cancel() override;
    void resume() override;

    void ref() override { ThreadSafeRefCounted<CurlRequest>::ref(); }
    void deref() override { ThreadSafeRefCounted<CurlRequest>::deref(); }

private:
    CurlTMServiceRequest(const ResourceRequest&, CurlRequestClient*, CaptureNetworkLoadMetrics captureMetrics, bool isMainResource);
    virtual ~CurlTMServiceRequest();

    Ref<CurlRequest> createCurlTMServiceRequest(URL&&);

    void curlDidSendData(CurlRequest&, unsigned long long, unsigned long long) override { }
    void curlDidReceiveResponse(CurlRequest&, CurlResponse&&) override;
    void curlDidReceiveData(CurlRequest&, const SharedBuffer&) override;
    void curlDidComplete(CurlRequest&, NetworkLoadMetrics&&) override;
    void curlDidFailWithError(CurlRequest&, ResourceError&&, CertificateInfo&&) override;

    bool willSendRequestIfNeeded(ResourceResponse&&);

    bool m_isMainResource { false };
    bool m_didCallResume { false };
    Vector<uint8_t> m_responseHeader;
    Vector<uint8_t> m_responseData;
    unsigned m_redirectCount { 0 };
    RefPtr<CurlRequest> m_tmServiceRequest;
};

} // namespace WebCore

#endif
