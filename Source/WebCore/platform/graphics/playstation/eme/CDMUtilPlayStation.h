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
#include "CDMInstanceSession.h"
#include "CDMKeySystemConfiguration.h"
#include "CDMPrivate.h"
#include "CDMRequirement.h"
#include "CDMRestrictions.h"
#include "CDMSessionType.h"
#include "SharedBuffer.h"

#include <mediaplayer/PSCDMInstanceSessionBackend.h>
#include <mediaplayer/PSCDMKeySystemConfigurationBackend.h>
#include <mediaplayer/PSCDMRestrictionsBackend.h>
#include <mediaplayer/PSEnumBackend.h>
#include <mediaplayer/PSSharedBufferBackend.h>

namespace WebCore {
namespace CDMUtilPlayStation {

mediaplayer::PSCDMKeySystemConfigurationBackend convertTo(const CDMKeySystemConfiguration);
CDMKeySystemConfiguration convertTo(const mediaplayer::PSCDMKeySystemConfigurationBackend&);

mediaplayer::PSCDMRestrictionsBackend convertTo(const CDMRestrictions);

mediaplayer::CDMRequirement convertTo(const CDMRequirement);
CDMRequirement convertTo(const mediaplayer::CDMRequirement);

mediaplayer::CDMSessionType convertTo(const CDMSessionType);
CDMSessionType convertTo(const mediaplayer::CDMSessionType);

mediaplayer::LocalStorageAccess convertTo(const CDMPrivate::LocalStorageAccess);

CDMInstance::SuccessValue convertTo(const mediaplayer::SuccessValue);

mediaplayer::PSSharedBufferBackend convertTo(const SharedBuffer&);
Ref<SharedBuffer> convertTo(const mediaplayer::PSSharedBufferBackend&);

CDMInstanceSession::MessageType convertTo(const mediaplayer::PSCDMInstanceSessionInterface::MessageType);
CDMInstanceSession::SessionLoadFailure convertTo(const mediaplayer::SessionLoadFailure);
CDMInstanceSessionClient::KeyStatus convertTo(const mediaplayer::CDMKeyStatus);
CDMInstanceSessionClient::KeyStatusVector convertTo(const mediaplayer::PSCDMInstanceSessionClient::KeyStatusVector);
}
}
#endif // PLATFORM(PLAYSTATION) && ENABLE(ENCRYPTED_MEDIA)
