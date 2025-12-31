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

#pragma once

#if PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)

#include "CDMInstance.h"
#include "CDMInstanceSessionPlayStation.h"
#include "CDMKeySystemConfiguration.h"
#include "SharedBuffer.h"
#include <mediaplayer/PSCDMInstancePrivateBackend.h>
#include <mediaplayer/PSCDMKeySystemConfigurationBackend.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CDMInstancePlayStation final : public CDMInstance, public CanMakeWeakPtr<CDMInstancePlayStation> {
public:
    CDMInstancePlayStation(std::shared_ptr<mediaplayer::PSCDMInstancePrivateInterface>);
    virtual ~CDMInstancePlayStation();

    ImplementationType implementationType() const final;

    void initializeWithConfiguration(const CDMKeySystemConfiguration&, AllowDistinctiveIdentifiers, AllowPersistentState, SuccessCallback&&) override;
    void setServerCertificate(Ref<SharedBuffer>&&, SuccessCallback&&) override;
    void setStorageDirectory(const String&) override;
    const String& keySystem() const override;
    RefPtr<CDMInstanceSession> createSession() override;

    std::shared_ptr<mediaplayer::PSCDMInstancePrivateInterface> backend() const { return m_playstation; }
    void setWebMusicPlayerEnabled(bool enabled) { m_webMusicPlayerEnabled = enabled; }

private:
    CDMInstance::SuccessValue convertToSuccessValue(mediaplayer::SuccessValue);
    CallbackQueuePlayStation<SuccessCallback>& setServerCertificateSuccessCallbackQueue() { return m_setServerCertificateSuccessCallbackQueue; }
    CallbackQueuePlayStation<SuccessCallback>& initializeWithConfigurationSuccessCallbackQueue() { return m_initializeWithConfigurationSuccessCallbackQueue; }

private:
    std::shared_ptr<mediaplayer::PSCDMInstancePrivateInterface> m_playstation;
    CallbackQueuePlayStation<SuccessCallback> m_setServerCertificateSuccessCallbackQueue;
    CallbackQueuePlayStation<SuccessCallback> m_initializeWithConfigurationSuccessCallbackQueue;
    bool m_webMusicPlayerEnabled { false };
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstancePlayStation, WebCore::CDMInstance::ImplementationType::Playstation);

#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
