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
#include "WKWebsiteDataStoreRefPlayStation.h"

#include "WKAPICast.h"
#include "WKArray.h"
#include "WKData.h"
#include "WebsiteDataStore.h"

#include <WebCore/CertificateInfo.h>

using namespace WebKit;

void WKWebsiteDataStoreSetAdditionalRootCA(WKWebsiteDataStoreRef dataStoreRef, WKStringRef rootCA)
{
    // for downstream-only
    toImpl(dataStoreRef)->setAdditionalRootCA(toWTFString(rootCA));
}

void WKWebsiteDataStoreSetClientCertificate(WKWebsiteDataStoreRef dataStoreRef, WKCertificateInfoRef certificateInfoRef)
{
    // for downstream-only
    // FIXME: WebCertificateInfo is removed.
}

void WKWebsiteDataStoreSetClientCertificateV2(WKWebsiteDataStoreRef dataStoreRef, WKDataRef certificateRef, WKDataRef privateKeyRef, WKStringRef privateKeyPasswordRef)
{
    // for downstream-only
    WebCore::CertificateInfo::CertificateChain certificateChain;
    auto certificate = WebCore::CertificateInfo::makeCertificate(WKDataGetBytes(certificateRef), WKDataGetSize(certificateRef));
    certificateChain.append(WTFMove(certificate));

    auto privateKey = WebCore::CertificateInfo::makePrivateKey(WKDataGetBytes(privateKeyRef), WKDataGetSize(privateKeyRef));

    WebCore::CertificateInfo certificateInfo { 0, WTFMove(certificateChain), WTFMove(privateKey), WebKit::toWTFString(privateKeyPasswordRef) };

    toImpl(dataStoreRef)->setClientCertificate(WTFMove(certificateInfo));
}

void WKWebsiteDataStoreSetNetworkBandwidth(WKWebsiteDataStoreRef dataStoreRef, WKNetworkBandwidthMode mode)
{
    // for downstream-only
    toImpl(dataStoreRef)->setNetworkBandwidth(mode);
}

int WKWebsiteDataStoreGetWebSecurityTMService(WKWebsiteDataStoreRef dataStoreRef)
{
    // for downstream-only
    return toImpl(dataStoreRef)->webSecurityTMService();
}

void WKWebsiteDataStoreSetWebSecurityTMService(WKWebsiteDataStoreRef dataStoreRef, int service)
{
    // for downstream-only
    toImpl(dataStoreRef)->setWebSecurityTMService(service);
}
