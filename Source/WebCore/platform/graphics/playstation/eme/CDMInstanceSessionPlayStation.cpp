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
#include "CDMInstanceSessionPlayStation.h"

#if PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)

#include "CDMUtilPlayStation.h"
#include <wtf/RefPtr.h>

namespace WebCore {

RefPtr<CDMInstanceSessionPlayStation> CDMInstanceSessionPlayStation::create()
{
    return adoptRef(*new CDMInstanceSessionPlayStation());
}

CDMInstanceSessionPlayStation::CDMInstanceSessionPlayStation()
    : m_weakThis(WeakPtr(this))
{
    m_playstation = mediaplayer::PSCDMSessionBackend::create();
}

CDMInstanceSessionPlayStation::~CDMInstanceSessionPlayStation()
{
}

void CDMInstanceSessionPlayStation::setClient(WeakPtr<CDMInstanceSessionClient>&& client)
{
    m_client = client;
    m_playstation->setClient(this);
}

void CDMInstanceSessionPlayStation::clearClient()
{
    m_client = nullptr;
    m_playstation->clearClient();
}

void CDMInstanceSessionPlayStation::requestLicense(LicenseType type, const AtomString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback&& callback)
{
    mediaplayer::PSCDMInstanceSessionInterface::LicenseType psLicenseType = CDMUtilPlayStation::convertTo(type);
    std::string psInitDataType = initDataType.string().utf8().data();
    mediaplayer::PSSharedBufferBackend psInitData = CDMUtilPlayStation::convertTo(initData);

    licenseCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = m_weakThis](mediaplayer::PSSharedBufferBackend&& psMessage,
        const std::string& psSessionId,
        bool psNeedsIndividualization,
        mediaplayer::SuccessValue psSuccessValue) {

        callOnMainThread([weakThis, psMessage = WTFMove(psMessage), psSessionId, psNeedsIndividualization, psSuccessValue]() {

            if (!weakThis)
                return;

            LicenseCallback&& callbackfunc = weakThis->licenseCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            String sessionId = String::fromUTF8(psSessionId.data());
            CDMInstanceSession::SuccessValue successValue = weakThis->convertToSuccessValue(psSuccessValue);
            callbackfunc(SharedBuffer::create(psMessage.data(), psMessage.size()), sessionId, psNeedsIndividualization, successValue);
        });
    };

    m_playstation->requestLicense(psLicenseType, psInitDataType, WTFMove(psInitData), WTFMove(psCallback));
}

void CDMInstanceSessionPlayStation::updateLicense(const String& sessionId, LicenseType type, Ref<SharedBuffer>&& response, LicenseUpdateCallback&& callback)
{
    const std::string psSessionId(sessionId.utf8().data());
    mediaplayer::PSCDMInstanceSessionInterface::LicenseType psLicenseType = CDMUtilPlayStation::convertTo(type);
    mediaplayer::PSSharedBufferBackend psResponse = CDMUtilPlayStation::convertTo(response);

    licenseUpdateCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = m_weakThis](bool psSessionWasClosed,
        mediaplayer::PSCDMInstanceSessionInterface::KeyStatusVector&& psKeyStatus,
        double&& psChangedExpiration,
        mediaplayer::PSCDMInstanceSessionInterface::Message&& psMessage,
        mediaplayer::SuccessValue psSuccessValue) {

        callOnMainThread([weakThis, psSessionWasClosed, psKeyStatus = WTFMove(psKeyStatus), psChangedExpiration, psMessage = WTFMove(psMessage), psSuccessValue]() {

            if (!weakThis)
                return;

            LicenseUpdateCallback&& callbackfunc = weakThis->licenseUpdateCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            std::optional<CDMInstanceSession::KeyStatusVector> keys;
            if (psKeyStatus.isValid)
                keys = CDMUtilPlayStation::convertTo(psKeyStatus.statuses);
            else
                keys = std::nullopt;

            std::optional<double> changedExpiration = psChangedExpiration;

            CDMInstanceSession::MessageType msgType = CDMUtilPlayStation::convertTo(psMessage.type);

            std::optional<CDMInstanceSession::Message> message = std::nullopt;
            if (psMessage.isValid)
                message = CDMInstanceSession::Message(msgType, SharedBuffer::create(psMessage.buffer.data(), psMessage.buffer.size()));

            CDMInstanceSession::SuccessValue successValue = weakThis->convertToSuccessValue(psSuccessValue);
            callbackfunc(psSessionWasClosed, WTFMove(keys), WTFMove(changedExpiration), WTFMove(message), successValue);
        });
    };

    m_playstation->updateLicense(psSessionId, psLicenseType, WTFMove(psResponse), WTFMove(psCallback));
}

void CDMInstanceSessionPlayStation::loadSession(LicenseType type, const String& sessionId, const String& origin, LoadSessionCallback&& callback)
{
    mediaplayer::PSCDMInstanceSessionInterface::LicenseType psLicenseType = CDMUtilPlayStation::convertTo(type);
    const std::string psSessionId(sessionId.utf8().data());
    const std::string psOrigin(origin.utf8().data());

    loadSessionCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = m_weakThis](mediaplayer::PSCDMInstanceSessionInterface::KeyStatusVector&& psKeyStatus,
        double&& psChangedExpiration,
        mediaplayer::PSCDMInstanceSessionInterface::Message&& psMessage,
        mediaplayer::SuccessValue psSuccessValue,
        mediaplayer::SessionLoadFailure psSessionLoadFailure) {

        callOnMainThread([weakThis, psKeyStatus = WTFMove(psKeyStatus), psChangedExpiration, psMessage = WTFMove(psMessage), psSuccessValue, psSessionLoadFailure]() {

            if (!weakThis)
                return;

            LoadSessionCallback&& callbackfunc = weakThis->loadSessionCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            std::optional<CDMInstanceSession::KeyStatusVector> keys;
            if (psKeyStatus.isValid)
                keys = CDMUtilPlayStation::convertTo(psKeyStatus.statuses);
            else
                keys = std::nullopt;

            CDMInstanceSession::MessageType msgType = CDMUtilPlayStation::convertTo(psMessage.type);

            std::optional<double> changedExpiration = psChangedExpiration;

            std::optional<CDMInstanceSession::Message> message = std::nullopt;
            if (psMessage.isValid)
                message = CDMInstanceSession::Message(msgType, SharedBuffer::create(psMessage.buffer.data(), psMessage.buffer.size()));

            CDMInstanceSession::SuccessValue successValue = weakThis->convertToSuccessValue(psSuccessValue);
            CDMInstanceSession::SessionLoadFailure sessionLoadFailure = CDMUtilPlayStation::convertTo(psSessionLoadFailure);
            callbackfunc(WTFMove(keys), WTFMove(changedExpiration), WTFMove(message), successValue, sessionLoadFailure);
        });
    };

    m_playstation->loadSession(psLicenseType, psSessionId, psOrigin, WTFMove(psCallback));
}

void CDMInstanceSessionPlayStation::closeSession(const String& sessionId, CloseSessionCallback&& callback)
{
    const std::string psSessionId(sessionId.utf8().data());

    closeSessionCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = m_weakThis]() {

        callOnMainThread([weakThis]() {

            if (!weakThis)
                return;

            CloseSessionCallback&& callbackfunc = weakThis->closeSessionCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            callbackfunc();
        });
    };

    m_playstation->closeSession(psSessionId, WTFMove(psCallback));
}

void CDMInstanceSessionPlayStation::removeSessionData(const String& sessionId, LicenseType type, RemoveSessionDataCallback&& callback)
{
    const std::string psSessionId(sessionId.utf8().data());
    mediaplayer::PSCDMInstanceSessionInterface::LicenseType psLicenseType = CDMUtilPlayStation::convertTo(type);

    removeSessionDataCallbackQueue().enqueue(WTFMove(callback));

    auto psCallback = [weakThis = m_weakThis](mediaplayer::PSCDMInstanceSessionInterface::KeyStatusVector&& psKeyStatus,
        mediaplayer::PSCDMInstanceSessionInterface::Message&& psMessage,
        mediaplayer::SuccessValue psSuccessValue) {

        callOnMainThread([weakThis, psKeyStatus = WTFMove(psKeyStatus), psMessage = WTFMove(psMessage), psSuccessValue]() {

            if (!weakThis)
                return;

            RemoveSessionDataCallback&& callbackfunc = weakThis->removeSessionDataCallbackQueue().dequeue();
            if (!callbackfunc)
                return;

            // Note: If isValid is false, the message event has already been fired in CMMInstanceSessionPlayStation::sendMessage.
            // Set std::nullopt to message to prevent extra message event from being fired.
            RefPtr<SharedBuffer> message;
            if (psMessage.isValid)
                message = SharedBuffer::create(psMessage.buffer.data(), psMessage.buffer.size());
            else
                message = nullptr;

            CDMInstanceSession::KeyStatusVector keys = CDMUtilPlayStation::convertTo(psKeyStatus.statuses);
            CDMInstanceSession::SuccessValue successValue = weakThis->convertToSuccessValue(psSuccessValue);
            callbackfunc(WTFMove(keys), WTFMove(message), successValue);
        });
    };

    m_playstation->removeSessionData(psSessionId, psLicenseType, WTFMove(psCallback));
}

void CDMInstanceSessionPlayStation::storeRecordOfKeyUsage(const String& sessionId)
{
    std::string psSessionId(sessionId.utf8().data());

    m_playstation->storeRecordOfKeyUsage(psSessionId);
}

void CDMInstanceSessionPlayStation::updateKeyStatuses(mediaplayer::PSCDMInstanceSessionClient::KeyStatusVector&& psKeyStatus)
{
    callOnMainThread([weakThis = m_weakThis, psKeyStatus]() {
        if (!weakThis || !weakThis->client())
            return;

        CDMInstanceSessionClient::KeyStatusVector keys = CDMUtilPlayStation::convertTo(psKeyStatus);
        weakThis->client()->updateKeyStatuses(WTFMove(keys));
    });
}

void CDMInstanceSessionPlayStation::sendMessage(mediaplayer::CDMMessageType psMessageType, mediaplayer::PSSharedBufferBackend&& psMessage)
{
    callOnMainThread([weakThis = m_weakThis, psMessageType, psMessage = WTFMove(psMessage)]() {
        if (!weakThis || !weakThis->client())
            return;

        CDMInstanceSession::MessageType msgType = CDMUtilPlayStation::convertTo(psMessageType);
        Ref<SharedBuffer> message = CDMUtilPlayStation::convertTo(psMessage);

        weakThis->client()->sendMessage(msgType, WTFMove(message));
    });
}

void CDMInstanceSessionPlayStation::sessionIdChanged(const std::string& psSessionId)
{
    callOnMainThread([weakThis = m_weakThis, psSessionId]() {
        if (!weakThis || !weakThis->client())
            return;

        weakThis->client()->sessionIdChanged(String::fromLatin1(psSessionId.c_str()));
    });
}

mediaplayer::PSCDMInstanceSessionClient::PlatformDisplayID CDMInstanceSessionPlayStation::displayID()
{
    return client() ? client()->displayID() : 0;
}

CDMInstanceSession::SuccessValue CDMInstanceSessionPlayStation::convertToSuccessValue(mediaplayer::SuccessValue value)
{
    switch (value) {
    case mediaplayer::SuccessValue::Failed:
        return CDMInstanceSession::SuccessValue::Failed;
    case mediaplayer::SuccessValue::Succeeded:
        return CDMInstanceSession::SuccessValue::Succeeded;
    default:
        return CDMInstanceSession::SuccessValue::Failed;
    }
}

}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
