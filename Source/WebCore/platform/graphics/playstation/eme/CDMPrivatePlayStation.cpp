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
#include "CDMPrivatePlayStation.h"

#if PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)

#include "CDMInstancePlayStation.h"
#include "CDMUtilPlayStation.h"
#include "InitDataRegistry.h"

namespace WebCore {

std::unique_ptr<CDMPrivatePlayStation> CDMPrivatePlayStation::createCDM(const String& keySystem)
{
    return makeUnique<CDMPrivatePlayStation>(keySystem);
}

CDMPrivatePlayStation::CDMPrivatePlayStation(const String& keySystem)
{
    std::string keySystemStr(keySystem.utf8().data());
    m_playstation = mediaplayer::PSCDMInstancePrivateBackend::create(keySystemStr);
}

CDMPrivatePlayStation::~CDMPrivatePlayStation()
{
}

Vector<AtomString> CDMPrivatePlayStation::supportedInitDataTypes() const
{
    const std::vector<std::string> psInitDataTypes = m_playstation->supportedInitDataTypes();

    Vector<AtomString> initDataTypes;
    for (auto psInitDataType : psInitDataTypes)
        initDataTypes.append(AtomString::fromLatin1(psInitDataType.c_str()));

    return initDataTypes;
}

bool CDMPrivatePlayStation::supportsConfiguration(const CDMKeySystemConfiguration& config) const
{
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);

    return m_playstation->supportsConfiguration(psConfig);
}

bool CDMPrivatePlayStation::supportsConfigurationWithRestrictions(const CDMKeySystemConfiguration& config, const CDMRestrictions& restrictions) const
{
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);
    mediaplayer::PSCDMRestrictionsBackend psRestrictions = CDMUtilPlayStation::convertTo(restrictions);

    return m_playstation->supportsConfigurationWithRestrictions(psConfig, psRestrictions);
}

bool CDMPrivatePlayStation::supportsSessionTypeWithConfiguration(const CDMSessionType& sessionType, const CDMKeySystemConfiguration& config) const
{
    mediaplayer::CDMSessionType psSessionType = CDMUtilPlayStation::convertTo(sessionType);
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);

    return m_playstation->supportsSessionTypeWithConfiguration(psSessionType, psConfig);
}

Vector<AtomString> CDMPrivatePlayStation::supportedRobustnesses() const
{
    const std::vector<std::string> psRobustnesses = m_playstation->supportedRobustnesses();

    Vector<AtomString> robustnesses;
    for (auto psRobustness : psRobustnesses)
        robustnesses.append(AtomString::fromLatin1(psRobustness.c_str()));

    return robustnesses;
}

CDMRequirement CDMPrivatePlayStation::distinctiveIdentifiersRequirement(const CDMKeySystemConfiguration& config, const CDMRestrictions& restrictions) const
{
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);
    mediaplayer::PSCDMRestrictionsBackend psRestrictions = CDMUtilPlayStation::convertTo(restrictions);

    return CDMUtilPlayStation::convertTo(m_playstation->distinctiveIdentifiersRequirement(psConfig, psRestrictions));
}

CDMRequirement CDMPrivatePlayStation::persistentStateRequirement(const CDMKeySystemConfiguration& config, const CDMRestrictions& restrictions) const
{
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);
    mediaplayer::PSCDMRestrictionsBackend psRestrictions = CDMUtilPlayStation::convertTo(restrictions);

    return CDMUtilPlayStation::convertTo(m_playstation->persistentStateRequirement(psConfig, psRestrictions));
}

bool CDMPrivatePlayStation::distinctiveIdentifiersAreUniquePerOriginAndClearable(const CDMKeySystemConfiguration& config) const
{
    mediaplayer::PSCDMKeySystemConfigurationBackend psConfig = CDMUtilPlayStation::convertTo(config);

    return m_playstation->distinctiveIdentifiersAreUniquePerOriginAndClearable(psConfig);
}

RefPtr<CDMInstance> CDMPrivatePlayStation::createInstance()
{
    auto instance = adoptRef(new CDMInstancePlayStation(m_playstation));
    if (!instance)
        return nullptr;

    instance->setWebMusicPlayerEnabled(m_webMusicPlayerEnabled);
    return instance;
}

void CDMPrivatePlayStation::loadAndInitialize()
{
    m_playstation->loadAndInitialize();
}

bool CDMPrivatePlayStation::supportsServerCertificates() const
{
    return m_playstation->supportsServerCertificates();
}

bool CDMPrivatePlayStation::supportsSessions() const
{
    return m_playstation->supportsSessions();
}

bool CDMPrivatePlayStation::supportsInitData(const AtomString& initDataType, const SharedBuffer& initData) const
{
    std::string psInitDataType(initDataType.string().utf8().data());
    mediaplayer::PSSharedBufferBackend psInitData = CDMUtilPlayStation::convertTo(initData);

    return m_playstation->supportsInitData(psInitDataType, psInitData);
}

RefPtr<SharedBuffer> CDMPrivatePlayStation::sanitizeResponse(const SharedBuffer& response) const
{
    mediaplayer::PSSharedBufferBackend psResponse = CDMUtilPlayStation::convertTo(response);
    mediaplayer::PSSharedBufferBackend retResponse;

    retResponse = m_playstation->sanitizeResponse(psResponse);

    return SharedBuffer::create(retResponse.data(), retResponse.size());
}

std::optional<String> CDMPrivatePlayStation::sanitizeSessionId(const String& sessionId) const
{
    std::string psSessionId(sessionId.utf8().data());
    std::string retSessionId = m_playstation->sanitizeSessionId(psSessionId);
    String Id = String::fromUTF8(retSessionId.data());

    return Id;
}

}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
