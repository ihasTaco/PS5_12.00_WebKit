/*
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
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
#include "CDMInstancePlayStation.h"

#if PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)

#include "CDMInstanceSessionPlayStation.h"
#include "CDMUtilPlayStation.h"
#include <wtf/RefPtr.h>

namespace WebCore {

CDMInstancePlayStation::CDMInstancePlayStation(std::shared_ptr<mediaplayer::PSCDMInstancePrivateInterface> backend)
{
    m_playstation = backend;
}

CDMInstancePlayStation::~CDMInstancePlayStation()
{
}

CDMInstance::ImplementationType CDMInstancePlayStation::implementationType() const
{
    return ImplementationType::Playstation;
}

void CDMInstancePlayStation::initializeWithConfiguration(const CDMKeySystemConfiguration& config, AllowDistinctiveIdentifiers allowDistinctiveIdentifiers, AllowPersistentState allowPersistentState, SuccessCallback&& callback)
{
    initializeWithConfigurationSuccessCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = WeakPtr(this)](mediaplayer::SuccessValue psSuccessValue) {
        callOnMainThread([weakThis, psSuccessValue]() {
            if (!weakThis)
                return;

            SuccessCallback&& callbackfunc = weakThis->initializeWithConfigurationSuccessCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            callbackfunc(weakThis->convertToSuccessValue(psSuccessValue));
        });
    };

    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);
    psConfig.isWebMusicPlayer = m_webMusicPlayerEnabled;
    bool distinctiveIdentifiers = (allowDistinctiveIdentifiers == AllowDistinctiveIdentifiers::Yes ? true : false);
    bool persistentState = (allowPersistentState == AllowPersistentState::Yes ? true : false);

    m_playstation->initializeWithConfiguration(psConfig, distinctiveIdentifiers, persistentState, WTFMove(psCallback));
}

void CDMInstancePlayStation::setServerCertificate(Ref<SharedBuffer>&& certificate, SuccessCallback&& callback)
{
    setServerCertificateSuccessCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = WeakPtr(this)](mediaplayer::SuccessValue psSuccessValue) {
        callOnMainThread([weakThis, psSuccessValue]() {
            if (!weakThis)
                return;

            SuccessCallback&& callbackfunc = weakThis->setServerCertificateSuccessCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            callbackfunc(weakThis->convertToSuccessValue(psSuccessValue));
        });
    };

    mediaplayer::PSSharedBufferBackend psCertificate = CDMUtilPlayStation::convertTo(certificate->makeContiguous());
    m_playstation->setServerCertificate(WTFMove(psCertificate), WTFMove(psCallback));
}

void CDMInstancePlayStation::setStorageDirectory(const String& directory)
{
    std::string psDirectory = directory.utf8().data();

    m_playstation->setStorageDirectory(psDirectory);
}

const String& CDMInstancePlayStation::keySystem() const
{
    static auto strKeySystem = String::fromLatin1(m_playstation->keySystem().data());

    return strKeySystem;
}

RefPtr<CDMInstanceSession> CDMInstancePlayStation::createSession()
{
    RefPtr<CDMInstanceSessionPlayStation> cdmInstanceSession = CDMInstanceSessionPlayStation::create();

    m_playstation->createSession(*(cdmInstanceSession->backend().get()));

    return cdmInstanceSession;
}

CDMInstance::SuccessValue CDMInstancePlayStation::convertToSuccessValue(mediaplayer::SuccessValue value)
{
    switch (value) {
    case mediaplayer::SuccessValue::Failed:
        return CDMInstance::SuccessValue::Failed;
    case mediaplayer::SuccessValue::Succeeded:
        return CDMInstance::SuccessValue::Succeeded;
    default:
        return CDMInstance::SuccessValue::Failed;
    }
}
}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
