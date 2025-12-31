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

#include "config.h"
#include "CurlTMServiceRequest.h"

#if PLATFORM(PLAYSTATION) // for downstream-only

#include "CurlRequest.h"
#include "CurlRequestScheduler.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "SynchronousLoaderClient.h"
#include <WebSecurity/WebSecurity.h>
#include <wtf/Language.h>

namespace WebCore {

Ref<CurlRequest> CurlTMServiceRequest::create(const ResourceRequest& request, CurlRequestClient& client, CaptureNetworkLoadMetrics captureMetrics, bool isMainResource)
{
    if (WebSecurity::activatedStatus())
        return adoptRef(*new CurlTMServiceRequest(request, &client, captureMetrics, isMainResource));

    return CurlRequest::create(request, client, captureMetrics);
}

CurlTMServiceRequest::CurlTMServiceRequest(const ResourceRequest& request, CurlRequestClient* client, CaptureNetworkLoadMetrics captureMetrics, bool isMainResource)
    : CurlRequest(request, client, captureMetrics)
    , m_isMainResource(isMainResource)
{
}

CurlTMServiceRequest::~CurlTMServiceRequest()
{
    cancel();
}

void CurlTMServiceRequest::resume()
{
    if (m_didCallResume)
        return;

    m_didCallResume = true;

    m_responseHeader.clear();
    m_responseData.clear();

    char* queryURL = nullptr;
    WebSecurity::convertUrl(m_isMainResource, m_request.url().string().utf8().data(), &queryURL);

    if (queryURL) {
        m_tmServiceRequest = createCurlTMServiceRequest(URL(URL(), String::fromUTF8(queryURL)));
        m_tmServiceRequest->resume();
    } else
        CurlRequest::resume();

    WebSecurity::free(queryURL);
}

void CurlTMServiceRequest::cancel()
{
    if (isCancelled())
        return;

    if (m_tmServiceRequest) {
        m_tmServiceRequest->cancel();
        m_tmServiceRequest = nullptr;
    }

    CurlRequest::cancel();
}

Ref<CurlRequest> CurlTMServiceRequest::createCurlTMServiceRequest(URL&& queryURL)
{
    ResourceRequest request;
    request.setURL(WTFMove(queryURL));
    request.addHTTPHeaderField(String::fromLatin1("Accept-Language"), m_request.httpHeaderField(HTTPHeaderName::AcceptLanguage));
    request.addHTTPHeaderField(String::fromLatin1("User-Agent"), String::fromUTF8(WebSecurity::userAgent()));

    return CurlRequest::create(request.isolatedCopy(), *this);
}

void CurlTMServiceRequest::curlDidReceiveResponse(CurlRequest& curlRequest, CurlResponse&& receivedResponse)
{
    ASSERT(isMainThread());

    if (isCancelled())
        return;

    for (const auto& header : receivedResponse.headers)
        m_responseHeader.append(header.utf8().data(), header.utf8().length());

    if (willSendRequestIfNeeded(ResourceResponse(receivedResponse)))
        return;

    curlRequest.completeDidReceiveResponse();
}

void CurlTMServiceRequest::curlDidReceiveData(CurlRequest&, const SharedBuffer& data)
{
    ASSERT(isMainThread());

    if (isCancelled())
        return;

    m_responseData.append(data.data(), data.size());
}

void CurlTMServiceRequest::curlDidComplete(CurlRequest&, NetworkLoadMetrics&&)
{
    ASSERT(isMainThread());

    if (isCancelled())
        return;

    char* evalURL = 0;
    unsigned evalURLSize = 0;
    bool showPage = false;

    WebSecurity::evalResponse(reinterpret_cast<char*>(m_responseHeader.data()), m_responseHeader.size(), reinterpret_cast<char*>(m_responseData.data()), m_responseData.size(), m_request.url().string().utf8().data(), &evalURL, &evalURLSize, showPage);
    ASSERT(showPage);

    if (evalURL && strncmp(m_request.url().string().utf8().data(), evalURL, evalURLSize)) {
        CurlResponse response;
        response.url = m_request.url();
        response.statusCode = 302;
        response.headers.append(String::fromLatin1("HTTP/1.1 302 Blocked\r\n"));
        response.headers.append(String::fromLatin1("Content-Type: text/html; charset=UTF-8\r\n"));
        response.headers.append(makeString("Location: ", evalURL, "\r\n"));

        callClient([response = WTFMove(response)](CurlRequest& request, CurlRequestClient& client) mutable {
            client.curlDidReceiveResponse(request, WTFMove(response));
        });
    } else
        CurlRequest::resume();

    WebSecurity::free(evalURL);
}

void CurlTMServiceRequest::curlDidFailWithError(CurlRequest&, ResourceError&& resourceError, CertificateInfo&& info)
{
    ASSERT(isMainThread());

    if (isCancelled())
        return;

    callClient([error = resourceError.isolatedCopy(), info = info.isolatedCopy()](CurlRequest& request, CurlRequestClient& client) mutable {
        client.curlDidFailWithError(request, WTFMove(error), WTFMove(info));
    });
}

bool CurlTMServiceRequest::willSendRequestIfNeeded(ResourceResponse&& response)
{
    static const int maxRedirects = 20;

    auto statusCode = response.httpStatusCode();
    if (statusCode < 300 || statusCode >= 400)
        return false;

    // Some 3xx status codes aren't actually redirects.
    if (statusCode == 300 || statusCode == 304 || statusCode == 305 || statusCode == 306)
        return false;

    if (response.httpHeaderField(HTTPHeaderName::Location).isEmpty())
        return false;

    if (m_redirectCount++ > maxRedirects) {
        callClient([error = ResourceError(CURLE_TOO_MANY_REDIRECTS, response.url())](CurlRequest& request, CurlRequestClient& client) mutable {
            client.curlDidFailWithError(request, WTFMove(error), CertificateInfo());
        });
        return true;
    }

    String location = response.httpHeaderField(HTTPHeaderName::Location);
    URL newURL = URL(response.url(), location);

    m_tmServiceRequest->cancel();

    m_responseHeader.clear();
    m_responseData.clear();

    m_tmServiceRequest = createCurlTMServiceRequest(WTFMove(newURL));
    m_tmServiceRequest->resume();

    return true;
}

}

#endif
