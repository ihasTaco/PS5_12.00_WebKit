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
#include "CDMUtilPlayStation.h"

#if PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)

namespace WebCore {
namespace CDMUtilPlayStation {

mediaplayer::PSCDMKeySystemConfigurationBackend convertTo(const CDMKeySystemConfiguration config)
{
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig;

    psConfig.label = config.label.utf8().data();

    for (auto type : config.initDataTypes) {
        std::string strType(type.string().utf8().data());
        psConfig.initDataTypes.push_back(strType);
    }

    for (auto audio : config.audioCapabilities) {
        mediaplayer::PSCDMMediaCapabilityBackend psAudio;
        psAudio.contentType = audio.contentType.utf8().data();
        psAudio.robustness = audio.robustness.utf8().data();
        psConfig.audioCapabilities.push_back(psAudio);
    }

    for (auto video : config.videoCapabilities) {
        mediaplayer::PSCDMMediaCapabilityBackend psVideo;
        psVideo.contentType = video.contentType.utf8().data();
        psVideo.robustness = video.robustness.utf8().data();
        psConfig.videoCapabilities.push_back(psVideo);
    }

    psConfig.distinctiveIdentifier = convertTo(config.distinctiveIdentifier);
    psConfig.persistentState = convertTo(config.persistentState);

    for (CDMSessionType sessionTypes : config.sessionTypes) {
        mediaplayer::CDMSessionType psSessionType = convertTo(sessionTypes);
        psConfig.sessionTypes.push_back(psSessionType);
    }

    return psConfig;
}

CDMKeySystemConfiguration convertTo(const mediaplayer::PSCDMKeySystemConfigurationBackend& psConfig)
{
    CDMKeySystemConfiguration config;

    config.label = String::fromUTF8(psConfig.label.c_str());

    for (auto type : psConfig.initDataTypes) {
        auto initDataType = String::fromLatin1(type.c_str());
        config.initDataTypes.append(initDataType);
    }

    for (auto audio : psConfig.audioCapabilities) {
        CDMMediaCapability audioCapabilitie;
        audioCapabilitie.contentType = String::fromUTF8(audio.contentType.c_str());
        audioCapabilitie.robustness = String::fromUTF8(audio.robustness.c_str());
        config.audioCapabilities.append(audioCapabilitie);
    }

    for (auto video : psConfig.videoCapabilities) {
        CDMMediaCapability videoCapabilitie;
        videoCapabilitie.contentType = String::fromUTF8(video.contentType.c_str());
        videoCapabilitie.robustness = String::fromUTF8(video.robustness.c_str());
        config.videoCapabilities.append(videoCapabilitie);
    }

    config.distinctiveIdentifier = convertTo(psConfig.distinctiveIdentifier);
    config.persistentState = convertTo(psConfig.persistentState);

    for (auto psSessionType : psConfig.sessionTypes) {
        CDMSessionType sessionType = convertTo(psSessionType);
        config.sessionTypes.append(sessionType);
    }

    return config;
}

mediaplayer::PSCDMRestrictionsBackend convertTo(const CDMRestrictions restrictions)
{
    mediaplayer::PSCDMRestrictionsBackend psRestrictions;

    psRestrictions.distinctiveIdentifierDenied = restrictions.distinctiveIdentifierDenied;
    psRestrictions.persistentStateDenied = restrictions.persistentStateDenied;

    return psRestrictions;
}

mediaplayer::CDMRequirement convertTo(const CDMRequirement requirement)
{
    switch (requirement) {
    case CDMRequirement::Required:
        return mediaplayer::CDMRequirement::Required;
    case CDMRequirement::Optional:
        return mediaplayer::CDMRequirement::Optional;
    case CDMRequirement::NotAllowed:
        return mediaplayer::CDMRequirement::NotAllowed;
    default:
        return mediaplayer::CDMRequirement::Required;
    }
}

CDMRequirement convertTo(const mediaplayer::CDMRequirement requirement)
{
    switch (requirement) {
    case mediaplayer::CDMRequirement::Required:
        return CDMRequirement::Required;
    case mediaplayer::CDMRequirement::Optional:
        return CDMRequirement::Optional;
    case mediaplayer::CDMRequirement::NotAllowed:
        return CDMRequirement::NotAllowed;
    default:
        return CDMRequirement::Required;
    }
}

mediaplayer::CDMSessionType convertTo(const CDMSessionType sessionType)
{
    switch (sessionType) {
    case CDMSessionType::Temporary:
        return mediaplayer::CDMSessionType::Temporary;
    case CDMSessionType::PersistentUsageRecord:
        return mediaplayer::CDMSessionType::PersistentUsageRecord;
    case CDMSessionType::PersistentLicense:
        return mediaplayer::CDMSessionType::PersistentLicense;
    default:
        return mediaplayer::CDMSessionType::Temporary;
    }
}

CDMSessionType convertTo(const mediaplayer::CDMSessionType sessionType)
{
    switch (sessionType) {
    case mediaplayer::CDMSessionType::Temporary:
        return CDMSessionType::Temporary;
    case mediaplayer::CDMSessionType::PersistentUsageRecord:
        return CDMSessionType::PersistentUsageRecord;
    case mediaplayer::CDMSessionType::PersistentLicense:
        return CDMSessionType::PersistentLicense;
    default:
        return CDMSessionType::Temporary;
    }
}

mediaplayer::LocalStorageAccess convertTo(const CDMPrivate::LocalStorageAccess localStorageAccess)
{
    switch (localStorageAccess) {
    case CDMPrivate::LocalStorageAccess::NotAllowed:
        return mediaplayer::LocalStorageAccess::NotAllowed;
    case CDMPrivate::LocalStorageAccess::Allowed:
        return mediaplayer::LocalStorageAccess::Allowed;
    default:
        return mediaplayer::LocalStorageAccess::NotAllowed;
    }
}

CDMInstance::SuccessValue convertTo(const mediaplayer::SuccessValue value)
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

mediaplayer::PSSharedBufferBackend convertTo(const SharedBuffer& buffer)
{
    mediaplayer::PSSharedBufferBackend psBuffer;
    for (size_t i = 0; i < buffer.size(); i++)
        psBuffer.push_back((buffer.data())[i]);

    return psBuffer;
}

Ref<SharedBuffer> convertTo(const mediaplayer::PSSharedBufferBackend& buffer)
{
    return SharedBuffer::create(buffer.data(), buffer.size());
}

CDMInstanceSession::MessageType convertTo(const mediaplayer::PSCDMInstanceSessionInterface::MessageType messageType)
{
    switch (messageType) {
    case mediaplayer::PSCDMInstanceSessionInterface::MessageType::LicenseRequest:
        return CDMInstanceSession::MessageType::LicenseRequest;
    case mediaplayer::PSCDMInstanceSessionInterface::MessageType::LicenseRenewal:
        return CDMInstanceSession::MessageType::LicenseRenewal;
    case mediaplayer::PSCDMInstanceSessionInterface::MessageType::LicenseRelease:
        return CDMInstanceSession::MessageType::LicenseRelease;
    case mediaplayer::PSCDMInstanceSessionInterface::MessageType::IndividualizationRequest:
        return CDMInstanceSession::MessageType::IndividualizationRequest;
    default:
        return CDMInstanceSession::MessageType::LicenseRequest;
    }
}

CDMInstanceSession::SessionLoadFailure convertTo(const mediaplayer::SessionLoadFailure failure)
{
    switch (failure) {
    case mediaplayer::SessionLoadFailure::NoneFailure:
        return CDMInstanceSession::SessionLoadFailure::None;
    case mediaplayer::SessionLoadFailure::NoSessionData:
        return CDMInstanceSession::SessionLoadFailure::NoSessionData;
    case mediaplayer::SessionLoadFailure::MismatchedSessionType:
        return CDMInstanceSession::SessionLoadFailure::MismatchedSessionType;
    case mediaplayer::SessionLoadFailure::QuotaExceeded:
        return CDMInstanceSession::SessionLoadFailure::QuotaExceeded;
    case mediaplayer::SessionLoadFailure::Other:
        return CDMInstanceSession::SessionLoadFailure::Other;
    default:
        return CDMInstanceSession::SessionLoadFailure::None;
    }
}

CDMInstanceSessionClient::KeyStatus convertTo(const mediaplayer::CDMKeyStatus status)
{
    switch (status) {
    case mediaplayer::CDMKeyStatus::Usable:
        return CDMInstanceSessionClient::KeyStatus::Usable;
    case mediaplayer::CDMKeyStatus::Expired:
        return CDMInstanceSessionClient::KeyStatus::Expired;
    case mediaplayer::CDMKeyStatus::Released:
        return CDMInstanceSessionClient::KeyStatus::Released;
    case mediaplayer::CDMKeyStatus::OutputRestricted:
        return CDMInstanceSessionClient::KeyStatus::OutputRestricted;
    case mediaplayer::CDMKeyStatus::OutputDownscaled:
        return CDMInstanceSessionClient::KeyStatus::OutputDownscaled;
    case mediaplayer::CDMKeyStatus::StatusPending:
        return CDMInstanceSessionClient::KeyStatus::StatusPending;
    case mediaplayer::CDMKeyStatus::InternalError:
        return CDMInstanceSessionClient::KeyStatus::InternalError;
    default:
        return CDMInstanceSessionClient::KeyStatus::Usable;
    }
}

CDMInstanceSessionClient::KeyStatusVector convertTo(const mediaplayer::PSCDMInstanceSessionClient::KeyStatusVector keyStatus)
{
    CDMInstanceSessionClient::KeyStatusVector keys;

    for (auto psKey : keyStatus) {
        auto keybuf = psKey.first;
        auto status = convertTo(psKey.second);
        std::pair<Ref<SharedBuffer>, CDMInstanceSessionClient::KeyStatus> key(SharedBuffer::create(keybuf.data(), keybuf.size()), status);

        keys.append(WTFMove(key));
    }

    return keys;
}

}
}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
