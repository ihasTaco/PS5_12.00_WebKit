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
#include "CDMKeySystemConfiguration.h"
#include "CDMPrivate.h"
#include "CDMRequirement.h"
#include "CDMRestrictions.h"
#include "CDMSessionType.h"
#include "CDMUtilPlayStation.h"
#include "CallbackQueuePlayStation.h"
#include "SharedBuffer.h"
#include <mediaplayer/PSCDMInstancePrivateBackend.h>
#include <mediaplayer/PSCDMKeySystemConfigurationBackend.h>
#include <mediaplayer/PSCDMRestrictionsBackend.h>
#include <mediaplayer/PSEnumBackend.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CDMPrivatePlayStation : public CDMPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static std::unique_ptr<CDMPrivatePlayStation> createCDM(const String& keySystem);
    CDMPrivatePlayStation(const String& keySystem);
    virtual ~CDMPrivatePlayStation();

    Vector<AtomString> supportedInitDataTypes() const override;
    bool supportsConfiguration(const CDMKeySystemConfiguration&) const override;
    bool supportsConfigurationWithRestrictions(const CDMKeySystemConfiguration&, const CDMRestrictions&) const override;
    bool supportsSessionTypeWithConfiguration(const CDMSessionType&, const CDMKeySystemConfiguration&) const override;
    Vector<AtomString> supportedRobustnesses() const override;
    CDMRequirement distinctiveIdentifiersRequirement(const CDMKeySystemConfiguration&, const CDMRestrictions&) const override;
    CDMRequirement persistentStateRequirement(const CDMKeySystemConfiguration&, const CDMRestrictions&) const override;
    bool distinctiveIdentifiersAreUniquePerOriginAndClearable(const CDMKeySystemConfiguration&) const override;
    RefPtr<CDMInstance> createInstance() override;
    void loadAndInitialize() override;
    bool supportsServerCertificates() const override;
    bool supportsSessions() const override;
    bool supportsInitData(const AtomString&, const SharedBuffer&) const override;
    RefPtr<SharedBuffer> sanitizeResponse(const SharedBuffer&) const override;
    std::optional<String> sanitizeSessionId(const String&) const override;
    void setWebMusicPlayerEnabled(bool enabled) { m_webMusicPlayerEnabled = enabled; }

private:
    CallbackQueuePlayStation<SupportedConfigurationCallback>& supportedConfigurationCallbackQueue() { return m_supportedConfigurationCallbackQueue; }

private:
    std::shared_ptr<mediaplayer::PSCDMInstancePrivateInterface> m_playstation;
    CallbackQueuePlayStation<SupportedConfigurationCallback> m_supportedConfigurationCallbackQueue;
    bool m_webMusicPlayerEnabled { false };
};

}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
