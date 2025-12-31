/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#pragma once

#include "CertificateSummary.h"
#include "NotImplemented.h"
#include <wtf/Vector.h>
#include <wtf/persistence/PersistentCoders.h>
#include <wtf/persistence/PersistentDecoder.h>
#include <wtf/persistence/PersistentEncoder.h>

namespace WebCore {

class CertificateInfo {
    WTF_MAKE_FAST_ALLOCATED;
public:
    using Certificate = Vector<uint8_t>;
    using CertificateChain = Vector<Certificate>;
#if PLATFORM(PLAYSTATION) // for downstream-only
    using PrivateKey = Vector<uint8_t>;
#endif

    CertificateInfo() = default;
    WEBCORE_EXPORT CertificateInfo(int verificationError, CertificateChain&&);
#if PLATFORM(PLAYSTATION) // for downstream-only
    CertificateInfo(int, CertificateChain&&, PrivateKey&&, String&&);
#endif

    WEBCORE_EXPORT CertificateInfo isolatedCopy() const;

    int verificationError() const { return m_verificationError; }
    WEBCORE_EXPORT String verificationErrorDescription() const;
    const Vector<Certificate>& certificateChain() const { return m_certificateChain; }

#if PLATFORM(PLAYSTATION) // for downstream-only
    const PrivateKey& privateKey() const { return m_privateKey; }
    const String& privateKeyPassword() const { return m_privateKeyPassword; }
#endif

    bool containsNonRootSHA1SignedCertificate() const { notImplemented(); return false; }

    std::optional<CertificateSummary> summary() const;

#if PLATFORM(PLAYSTATION) // for downstream-only
    bool isEmpty() const { return m_certificateChain.isEmpty() && m_privateKey.isEmpty() && m_privateKeyPassword.isEmpty(); }
    bool hasPrivateKey() const { return !m_privateKey.isEmpty(); }
#else
    bool isEmpty() const { return m_certificateChain.isEmpty(); }
#endif

    static Certificate makeCertificate(const uint8_t*, size_t);
#if PLATFORM(PLAYSTATION) // for downstream-only
    static PrivateKey makePrivateKey(const uint8_t*, size_t);
#endif

    bool operator==(const CertificateInfo& other) const
    {
#if PLATFORM(PLAYSTATION) // for downstream-only
        return verificationError() == other.verificationError()
            && certificateChain() == other.certificateChain()
            && privateKey() == other.privateKey()
            && privateKeyPassword() == other.privateKeyPassword();
#else
        return verificationError() == other.verificationError()
            && certificateChain() == other.certificateChain();
#endif
    }
    bool operator!=(const CertificateInfo& other) const { return !(*this == other); }

private:
    int m_verificationError { 0 };
    CertificateChain m_certificateChain;

#if PLATFORM(PLAYSTATION) // for downstream-only
    PrivateKey m_privateKey;
    String m_privateKeyPassword;
#endif
};

} // namespace WebCore
