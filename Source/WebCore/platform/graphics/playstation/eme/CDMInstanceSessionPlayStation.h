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

#if PLATFORM(PLAYSTATION) && ENABLE(MEDIA_SOURCE)

#include "CDMInstanceSession.h"
#include "CDMUtilPlayStation.h"
#include "CallbackQueuePlayStation.h"
#include "SharedBuffer.h"

#include <mediaplayer/PSCDMInstanceSessionBackend.h>
#include <mutex>
#include <wtf/Ref.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/AtomString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CDMInstanceSessionPlayStation : public CDMInstanceSession, public CanMakeWeakPtr<CDMInstanceSessionPlayStation>, public mediaplayer::PSCDMInstanceSessionClient {
public:
    static RefPtr<CDMInstanceSessionPlayStation> create();
    virtual ~CDMInstanceSessionPlayStation();

    void setClient(WeakPtr<CDMInstanceSessionClient>&&) override;
    void clearClient() override;

    void requestLicense(LicenseType, const AtomString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback&&) override;
    void updateLicense(const String& sessionId, LicenseType, Ref<SharedBuffer>&& response, LicenseUpdateCallback&&) override;
    void loadSession(LicenseType, const String& sessionId, const String& origin, LoadSessionCallback&&) override;
    void closeSession(const String& sessionId, CloseSessionCallback&&) override;
    void removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback&&) override;
    void storeRecordOfKeyUsage(const String& sessionId) override;

    void updateKeyStatuses(mediaplayer::PSCDMInstanceSessionClient::KeyStatusVector&&) override;
    void sendMessage(mediaplayer::CDMMessageType, mediaplayer::PSSharedBufferBackend&& psMessage) override;
    void sessionIdChanged(const std::string&) override;
    mediaplayer::PSCDMInstanceSessionClient::PlatformDisplayID displayID() override;

    std::shared_ptr<mediaplayer::PSCDMInstanceSessionInterface> backend() const { return m_playstation; }

private:
    CDMInstanceSessionPlayStation();

    WeakPtr<CDMInstanceSessionClient> client() { return m_client; }

    CDMInstanceSession::SuccessValue convertToSuccessValue(mediaplayer::SuccessValue);

    CallbackQueuePlayStation<LicenseCallback>& licenseCallbackQueue() { return m_licenseCallbackQueue; }
    CallbackQueuePlayStation<LicenseUpdateCallback>& licenseUpdateCallbackQueue() { return m_licenseUpdateCallbackQueue; }
    CallbackQueuePlayStation<LoadSessionCallback>& loadSessionCallbackQueue() { return m_loadSessionCallbackQueue; }
    CallbackQueuePlayStation<CloseSessionCallback>& closeSessionCallbackQueue() { return m_closeSessionCallbackQueue; }
    CallbackQueuePlayStation<RemoveSessionDataCallback>& removeSessionDataCallbackQueue() { return m_removeSessionDataCallbackQueue; }

private:
    WeakPtr<CDMInstanceSessionPlayStation> m_weakThis;

    std::shared_ptr<mediaplayer::PSCDMInstanceSessionInterface> m_playstation { nullptr };
    WeakPtr<CDMInstanceSessionClient> m_client { nullptr };

    CallbackQueuePlayStation<LicenseCallback> m_licenseCallbackQueue;
    CallbackQueuePlayStation<LicenseUpdateCallback> m_licenseUpdateCallbackQueue;
    CallbackQueuePlayStation<LoadSessionCallback> m_loadSessionCallbackQueue;
    CallbackQueuePlayStation<CloseSessionCallback> m_closeSessionCallbackQueue;
    CallbackQueuePlayStation<RemoveSessionDataCallback> m_removeSessionDataCallbackQueue;
};

}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
