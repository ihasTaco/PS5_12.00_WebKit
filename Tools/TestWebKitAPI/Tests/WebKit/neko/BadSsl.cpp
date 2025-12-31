/*
 * Copyright (C) 2023 Sony Interactive Entertainment Inc.
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

#if WK_HAVE_C_SPI

#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <WebKit/WKAuthenticationChallenge.h>
#include <WebKit/WKAuthenticationDecisionListener.h>
#include <WebKit/WKCertificateInfoCurl.h>
#include <WebKit/WKContextPrivate.h>
#include <WebKit/WKCredential.h>
#include <WebKit/WKProtectionSpace.h>
#include <WebKit/WKProtectionSpaceCurl.h>
#include <openssl/x509_vfy.h>

namespace TestWebKitAPI {

// Note: If the server certificate has expired, only one certificate chain will be returned,
//       even though there are actually multiple certificate chains. We need to investigate
//       whether this is by design or a bug.
#define LIMIT_TO_ONE_CERTIFICATE 0

// Note: If you want to test tls-v1-x on SIE LAN, you need to set up a proxy.
#define ENABLE_TLS_V1_TEST 0

struct State {
    enum class RejectMode : bool {
        Reject,
        Accept
    };

    enum class EndMode : uint8_t {
        Finish,
        Fail
    };

    State() = default;

    State(RejectMode mode)
        : canAcceptServerCertificate(mode == RejectMode::Accept)
    {
    }

    State(String&& hostArg, EndMode mode, bool serverTrustEvaluationRequested = false, int error = X509_V_OK, Vector<String>&& certsArg = { })
        : didFinish(mode == EndMode::Finish)
        , didFail(mode == EndMode::Fail)
        , didReceiveServerTrustEvaluationRequested(serverTrustEvaluationRequested)
        , verificationError(error)
        , host(WTFMove(hostArg))
        , certs(WTFMove(certsArg))
    {
    }

    bool canAcceptServerCertificate { false };

    bool testDone { false };
    bool didFinish { false };
    bool didFail { false };

    bool didReceiveServerTrustEvaluationRequested { false };
    int verificationError = 0;
    String host;
    Vector<String> certs;
};

static String createString(WKStringRef wkString)
{
    size_t bufferSize = WKStringGetMaximumUTF8CStringSize(wkString);
    std::vector<char> buffer(bufferSize);
    size_t size = WKStringGetUTF8CString(wkString, buffer.data(), bufferSize);
    return String { buffer.data(), static_cast<unsigned>(size - 1) };
}

static void didFinishNavigation(WKPageRef, WKNavigationRef, WKTypeRef, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));

    state->didFinish = true;
    state->testDone = true;
}

static void didFailProvisionalNavigation(WKPageRef, WKNavigationRef, WKErrorRef, WKTypeRef, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));

    state->didFail = true;
    state->testDone = true;
}

static void didReceiveAuthenticationChallenge(WKPageRef, WKAuthenticationChallengeRef challenge, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));

    auto protectionSpace = WKAuthenticationChallengeGetProtectionSpace(challenge);
    auto decisionListener = WKAuthenticationChallengeGetDecisionListener(challenge);
    auto authenticationScheme = WKProtectionSpaceGetAuthenticationScheme(protectionSpace);

    EXPECT_EQ(authenticationScheme, kWKProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested);

    if (authenticationScheme == kWKProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested) {
        state->didReceiveServerTrustEvaluationRequested = true;
        state->verificationError = WKProtectionSpaceGetCertificateVerificationError(protectionSpace);
        state->host = createString(adoptWK(WKProtectionSpaceCopyHost(protectionSpace)).get());

        auto chain = adoptWK(WKProtectionSpaceCopyCertificateChain(protectionSpace));
        auto chainSize = WKArrayGetSize(chain.get());

        for (auto i = 0; i < chainSize; i++) {
            auto item = WKArrayGetItemAtIndex(chain.get(), i);
            auto certificate = static_cast<WKDataRef>(item);
            auto size = WKDataGetSize(certificate);
            auto data = WKDataGetBytes(certificate);
            state->certs.append(String(data, size).removeCharacters([](auto c) {
                return c == '\r' || c == '\n';
            }));
        }
    }

    if (state->canAcceptServerCertificate) {
        auto username = adoptWK(WKStringCreateWithUTF8CString("accept server trust"));
        auto password = adoptWK(WKStringCreateWithUTF8CString(""));
        auto wkCredential = adoptWK(WKCredentialCreate(username.get(), password.get(), kWKCredentialPersistenceForSession));
        WKAuthenticationDecisionListenerUseCredential(decisionListener, wkCredential.get());
    } else
        WKAuthenticationDecisionListenerUseCredential(decisionListener, nullptr);
}

static void setupClient(WKPageRef page, void* clientInfo)
{
    WKPageNavigationClientV0 loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));

    loaderClient.base.version = 0;
    loaderClient.base.clientInfo = clientInfo;
    loaderClient.didFinishNavigation = didFinishNavigation;
    loaderClient.didFailProvisionalNavigation = didFailProvisionalNavigation;
    loaderClient.didReceiveAuthenticationChallenge = didReceiveAuthenticationChallenge;

    WKPageSetPageNavigationClient(page, &loaderClient.base);
}

static void setupClientWithHost(WKPageRef page, void* clientInfo, const String& host)
{
    setupClient(page, clientInfo);

    auto url = makeString("https://"_s, host, "/"_s);
    WKPageLoadURL(page, adoptWK(WKURLCreateWithUTF8CString(url.utf8().data())).get());
}

static void runTestWithAccept(const String& host)
{
    // Test the case that the server certificate accepted after the server trust evaluation failed
    WKRetainPtr<WKContextRef> context = adoptWK(WKContextCreateWithConfiguration(nullptr));
    PlatformWebView webView(context.get());

    State actual(State::RejectMode::Accept);
    setupClientWithHost(webView.page(), &actual, host);

    Util::run(&actual.testDone);

    EXPECT_TRUE(actual.didFinish);
    EXPECT_FALSE(actual.didFail);
    EXPECT_TRUE(actual.didReceiveServerTrustEvaluationRequested);
}

static void runTest(const State& expected)
{
    // Test the case where the content loads successfully or the server trust evaluation failed.
    WKRetainPtr<WKContextRef> context = adoptWK(WKContextCreateWithConfiguration(nullptr));
    PlatformWebView webView(context.get());

    State actual;
    setupClientWithHost(webView.page(), &actual, expected.host);

    Util::run(&actual.testDone);

    EXPECT_EQ(expected.didFinish, actual.didFinish);
    EXPECT_EQ(expected.didFail, actual.didFail);

    EXPECT_EQ(expected.didReceiveServerTrustEvaluationRequested, actual.didReceiveServerTrustEvaluationRequested);
    if (!expected.didReceiveServerTrustEvaluationRequested || !actual.didReceiveServerTrustEvaluationRequested)
        return;

    EXPECT_EQ(expected.verificationError, actual.verificationError);
    EXPECT_EQ(expected.host, actual.host);

    EXPECT_EQ(expected.certs.size(), actual.certs.size());
    if (expected.certs.size() == actual.certs.size()) {
        for (auto i = 0; i < actual.certs.size(); i++)
            EXPECT_WK_STREQ(expected.certs[i].ascii().data(), actual.certs[i].ascii().data());
    };

    if (expected.didFail)
        runTestWithAccept(expected.host);
}

TEST(BadSsl, Expired)
{
    Vector<String> certs {
        //  0 s:OU = Domain Control Validated, OU = PositiveSSL Wildcard, CN = *.badssl.com
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Domain Validation Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIFSzCCBDOgAwIBAgIQSueVSfqavj8QDxekeOFpCTANBgkqhkiG9w0BAQsFADCB"
        "kDELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxNjA0BgNV"
        "BAMTLUNPTU9ETyBSU0EgRG9tYWluIFZhbGlkYXRpb24gU2VjdXJlIFNlcnZlciBD"
        "QTAeFw0xNTA0MDkwMDAwMDBaFw0xNTA0MTIyMzU5NTlaMFkxITAfBgNVBAsTGERv"
        "bWFpbiBDb250cm9sIFZhbGlkYXRlZDEdMBsGA1UECxMUUG9zaXRpdmVTU0wgV2ls"
        "ZGNhcmQxFTATBgNVBAMUDCouYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD"
        "ggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5lZ6xKyx2PmzA"
        "S2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2QULFRtMWhyef"
        "dOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk0TG23U3AxPxT"
        "uW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFXRNsKEqveww9H"
        "dFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EFy+b+a1SYQCeF"
        "xxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaOCAdUwggHRMB8GA1Ud"
        "IwQYMBaAFJCvajqUWgvYkOoSVnPfQ7Q6KNrnMB0GA1UdDgQWBBSd7sF7gQs6R2lx"
        "GH0RN5O8pRs/+zAOBgNVHQ8BAf8EBAMCBaAwDAYDVR0TAQH/BAIwADAdBgNVHSUE"
        "FjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTwYDVR0gBEgwRjA6BgsrBgEEAbIxAQIC"
        "BzArMCkGCCsGAQUFBwIBFh1odHRwczovL3NlY3VyZS5jb21vZG8uY29tL0NQUzAI"
        "BgZngQwBAgEwVAYDVR0fBE0wSzBJoEegRYZDaHR0cDovL2NybC5jb21vZG9jYS5j"
        "b20vQ09NT0RPUlNBRG9tYWluVmFsaWRhdGlvblNlY3VyZVNlcnZlckNBLmNybDCB"
        "hQYIKwYBBQUHAQEEeTB3ME8GCCsGAQUFBzAChkNodHRwOi8vY3J0LmNvbW9kb2Nh"
        "LmNvbS9DT01PRE9SU0FEb21haW5WYWxpZGF0aW9uU2VjdXJlU2VydmVyQ0EuY3J0"
        "MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wIwYDVR0RBBww"
        "GoIMKi5iYWRzc2wuY29tggpiYWRzc2wuY29tMA0GCSqGSIb3DQEBCwUAA4IBAQBq"
        "evHa/wMHcnjFZqFPRkMOXxQhjHUa6zbgH6QQFezaMyV8O7UKxwE4PSf9WNnM6i1p"
        "OXy+l+8L1gtY54x/v7NMHfO3kICmNnwUW+wHLQI+G1tjWxWrAPofOxkt3+IjEBEH"
        "fnJ/4r+3ABuYLyw/zoWaJ4wQIghBK4o+gk783SHGVnRwpDTysUCeK1iiWQ8dSO/r"
        "ET7BSp68ZVVtxqPv1dSWzfGuJ/ekVxQ8lEEFeouhN0fX9X3c+s5vMaKwjOrMEpsi"
        "8TRwz311SotoKQwe6Zaoz7ASH1wq7mcvf71z81oBIgxw+s1F73hczg36TuHvzmWf"
        "RwxPuzZEaFZcVlmtqoq8"
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Domain Validation Secure Server CA
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Certification Authority
        "-----BEGIN CERTIFICATE-----"
        "MIIGCDCCA/CgAwIBAgIQKy5u6tl1NmwUim7bo3yMBzANBgkqhkiG9w0BAQwFADCB"
        "hTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxKzApBgNV"
        "BAMTIkNPTU9ETyBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTQwMjEy"
        "MDAwMDAwWhcNMjkwMjExMjM1OTU5WjCBkDELMAkGA1UEBhMCR0IxGzAZBgNVBAgT"
        "EkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEaMBgGA1UEChMR"
        "Q09NT0RPIENBIExpbWl0ZWQxNjA0BgNVBAMTLUNPTU9ETyBSU0EgRG9tYWluIFZh"
        "bGlkYXRpb24gU2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEP"
        "ADCCAQoCggEBAI7CAhnhoFmk6zg1jSz9AdDTScBkxwtiBUUWOqigwAwCfx3M28Sh"
        "bXcDow+G+eMGnD4LgYqbSRutA776S9uMIO3Vzl5ljj4Nr0zCsLdFXlIvNN5IJGS0"
        "Qa4Al/e+Z96e0HqnU4A7fK31llVvl0cKfIWLIpeNs4TgllfQcBhglo/uLQeTnaG6"
        "ytHNe+nEKpooIZFNb5JPJaXyejXdJtxGpdCsWTWM/06RQ1A/WZMebFEh7lgUq/51"
        "UHg+TLAchhP6a5i84DuUHoVS3AOTJBhuyydRReZw3iVDpA3hSqXttn7IzW3uLh0n"
        "c13cRTCAquOyQQuvvUSH2rnlG51/ruWFgqUCAwEAAaOCAWUwggFhMB8GA1UdIwQY"
        "MBaAFLuvfgI9+qbxPISOre44mOzZMjLUMB0GA1UdDgQWBBSQr2o6lFoL2JDqElZz"
        "30O0Oija5zAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNV"
        "HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwGwYDVR0gBBQwEjAGBgRVHSAAMAgG"
        "BmeBDAECATBMBgNVHR8ERTBDMEGgP6A9hjtodHRwOi8vY3JsLmNvbW9kb2NhLmNv"
        "bS9DT01PRE9SU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDBxBggrBgEFBQcB"
        "AQRlMGMwOwYIKwYBBQUHMAKGL2h0dHA6Ly9jcnQuY29tb2RvY2EuY29tL0NPTU9E"
        "T1JTQUFkZFRydXN0Q0EuY3J0MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21v"
        "ZG9jYS5jb20wDQYJKoZIhvcNAQEMBQADggIBAE4rdk+SHGI2ibp3wScF9BzWRJ2p"
        "mj6q1WZmAT7qSeaiNbz69t2Vjpk1mA42GHWx3d1Qcnyu3HeIzg/3kCDKo2cuH1Z/"
        "e+FE6kKVxF0NAVBGFfKBiVlsit2M8RKhjTpCipj4SzR7JzsItG8kO3KdY3RYPBps"
        "P0/HEZrIqPW1N+8QRcZs2eBelSaz662jue5/DJpmNXMyYE7l3YphLG5SEXdoltMY"
        "dVEVABt0iN3hxzgEQyjpFv3ZBdRdRydg1vs4O2xyopT4Qhrf7W8GjEXCBgCq5Ojc"
        "2bXhc3js9iPc0d1sjhqPpepUfJa3w/5Vjo1JXvxku88+vZbrac2/4EjxYoIQ5QxG"
        "V/Iz2tDIY+3GH5QFlkoakdH368+PUq4NCNk+qKBR6cGHdNXJ93SrLlP7u3r7l+L4"
        "HyaPs9Kg4DdbKDsx5Q5XLVq4rXmsXiBmGqW5prU5wfWYQ//u+aen/e7KJD2AFsQX"
        "j4rBYKEMrltDR5FL1ZoXX/nUh8HCjLfn4g8wGTeGrODcQgPmlKidrv0PJFGUzpII"
        "0fxQ8ANAe4hZ7Q7drNJ3gjTcBpUC2JD5Leo31Rpg0Gcg19hCC0Wvgmje3WYkN5Ap"
        "lBlGGSW4gNfL1IYoakRwJiNiqZ+Gb7+6kHDSVneFeO/qJakXzlByjAA6quPbYzSf"
        "+AZxAeKCINT+b72x"
        "-----END CERTIFICATE-----"_s,
        // 2 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Certification Authority
        //   i:C = SE, O = AddTrust AB, OU = AddTrust External TTP Network, CN = AddTrust External CA Root
        "-----BEGIN CERTIFICATE-----"
        "MIIFdDCCBFygAwIBAgIQJ2buVutJ846r13Ci/ITeIjANBgkqhkiG9w0BAQwFADBv"
        "MQswCQYDVQQGEwJTRTEUMBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFk"
        "ZFRydXN0IEV4dGVybmFsIFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBF"
        "eHRlcm5hbCBDQSBSb290MB4XDTAwMDUzMDEwNDgzOFoXDTIwMDUzMDEwNDgzOFow"
        "gYUxCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAO"
        "BgNVBAcTB1NhbGZvcmQxGjAYBgNVBAoTEUNPTU9ETyBDQSBMaW1pdGVkMSswKQYD"
        "VQQDEyJDT01PRE8gUlNBIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIICIjANBgkq"
        "hkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAkehUktIKVrGsDSTdxc9EZ3SZKzejfSNw"
        "AHG8U9/E+ioSj0t/EFa9n3Byt2F/yUsPF6c947AEYe7/EZfH9IY+Cvo+XPmT5jR6"
        "2RRr55yzhaCCenavcZDX7P0N+pxs+t+wgvQUfvm+xKYvT3+Zf7X8Z0NyvQwA1onr"
        "ayzT7Y+YHBSrfuXjbvzYqOSSJNpDa2K4Vf3qwbxstovzDo2a5JtsaZn4eEgwRdWt"
        "4Q08RWD8MpZRJ7xnw8outmvqRsfHIKCxH2XeSAi6pE6p8oNGN4Tr6MyBSENnTnIq"
        "m1y9TBsoilwie7SrmNnu4FGDwwlGTm0+mfqVF9p8M1dBPI1R7Qu2XK8sYxrfV8g/"
        "vOldxJuvRZnio1oktLqpVj3Pb6r/SVi+8Kj/9Lit6Tf7urj0Czr56ENCHonYhMsT"
        "8dm74YlguIwoVqwUHZwK53Hrzw7dPamWoUi9PPevtQ0iTMARgexWO/bTouJbt7IE"
        "IlKVgJNp6I5MZfGRAy1wdALqi2cVKWlSArvX31BqVUa/oKMoYX9w0MOiqiwhqkfO"
        "KJwGRXa/ghgntNWutMtQ5mv0TIZxMOmm3xaG4Nj/QN370EKIf6MzOi5cHkERgWPO"
        "GHFrK+ymircxXDpqR+DDeVnWIBqv8mqYqnK8V0rSS527EPywTEHl7R09XiidnMy/"
        "s1Hap0flhFMCAwEAAaOB9DCB8TAfBgNVHSMEGDAWgBStvZh6NLQm9/rEJlTvA73g"
        "JMtUGjAdBgNVHQ4EFgQUu69+Aj36pvE8hI6t7jiY7NkyMtQwDgYDVR0PAQH/BAQD"
        "AgGGMA8GA1UdEwEB/wQFMAMBAf8wEQYDVR0gBAowCDAGBgRVHSAAMEQGA1UdHwQ9"
        "MDswOaA3oDWGM2h0dHA6Ly9jcmwudXNlcnRydXN0LmNvbS9BZGRUcnVzdEV4dGVy"
        "bmFsQ0FSb290LmNybDA1BggrBgEFBQcBAQQpMCcwJQYIKwYBBQUHMAGGGWh0dHA6"
        "Ly9vY3NwLnVzZXJ0cnVzdC5jb20wDQYJKoZIhvcNAQEMBQADggEBAGS/g/FfmoXQ"
        "zbihKVcN6Fr30ek+8nYEbvFScLsePP9NDXRqzIGCJdPDoCpdTPW6i6FtxFQJdcfj"
        "Jw5dhHk3QBN39bSsHNA7qxcS1u80GH4r6XnTq1dFDK8o+tDb5VCViLvfhVdpfZLY"
        "Uspzgb8c8+a4bmYRBbMelC1/kZWSWfFMzqORcUx8Rww7Cxn2obFshj5cqsQugsv5"
        "B5a6SE2Q8pTIqXOi6wZ7I53eovNNVZ96YUWYGGjHXkBrI/V5eu+MtWuLt29G9Hvx"
        "PUsE2JOAWVrgQSQdso8VYFhH2+9uRv0V9dlfmrPb2LjkQLPNlzmuhbsdjrzch5vR"
        "pu/xO28QOG8="
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "expired.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, WrongHost)
{
    const auto x509VErrUnmatchedCommonName = 100;

    Vector<String> certs {
        //  0 s:CN = *.badssl.com
        //    i:C = US, O = Let's Encrypt, CN = R3
        "-----BEGIN CERTIFICATE-----"
        "MIIE8DCCA9igAwIBAgISA4mqZntfCH8MYyIVqUF1XZgpMA0GCSqGSIb3DQEBCwUA"
        "MDIxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQD"
        "EwJSMzAeFw0yMzEwMTkxNTUwMjlaFw0yNDAxMTcxNTUwMjhaMBcxFTATBgNVBAMM"
        "DCouYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAONj"
        "dsqxZsR+pDzWX6GLCy6ImoAT60LNYvs9U6BIQ+fatIWbMELAFD6jY+IP25hrVEr1"
        "bgwRWmAAOnUc2qKXdtx6KXXO3cAJoCSHFNBDEZqzg/+exj+3emQH8dVZiYAS2Rpd"
        "nL9uKc3xgDDb74p1m7J4JdMewHmebRUmMt0MbA0f8sxvhbv9wIXkgAZd6dKYPGzJ"
        "KJlCoQifPiJ66JwYk8WVGEJH9m8LNDse388MscfsuwvAAh9tt2Fq6rmV9s21P6qf"
        "JgjePl65e8fVjsEWBAvC/aMYvTUs7Gdqej0qByjESpt1LZClNomJDvIgqA9+5KsU"
        "yCXigT6OiPjtZhdhgw0CAwEAAaOCAhkwggIVMA4GA1UdDwEB/wQEAwIFoDAdBgNV"
        "HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0TAQH/BAIwADAdBgNVHQ4E"
        "FgQUqryJ2HM8bXniU+mPXLakkgUQobMwHwYDVR0jBBgwFoAUFC6zF7dYVsuuUAlA"
        "5h+vnYsUwsYwVQYIKwYBBQUHAQEESTBHMCEGCCsGAQUFBzABhhVodHRwOi8vcjMu"
        "by5sZW5jci5vcmcwIgYIKwYBBQUHMAKGFmh0dHA6Ly9yMy5pLmxlbmNyLm9yZy8w"
        "IwYDVR0RBBwwGoIMKi5iYWRzc2wuY29tggpiYWRzc2wuY29tMBMGA1UdIAQMMAow"
        "CAYGZ4EMAQIBMIIBAwYKKwYBBAHWeQIEAgSB9ASB8QDvAHUA2ra/az+1tiKfm8K7"
        "XGvocJFxbLtRhIU0vaQ9MEjX+6sAAAGLSNh8nwAABAMARjBEAiBkJnQowOqs+tDj"
        "7qXXu0PlDCvgvtEemuw1OvInlaHSrAIgcCZV5dJmGVrS1voinEpAzScJejhGB0vb"
        "G8dfKhJZD+wAdgA7U3d1Pi25gE6LMFsG/kA7Z9hPw/THvQANLXJv4frUFwAAAYtI"
        "2HyZAAAEAwBHMEUCIQCJ+gamX0P/hGiIuu70hn8d0svHSOAMJs3D+eOjMVqsywIg"
        "JXR/lAknUTRU+SyfySDoQ22bDSXfYWZGHLFgAkiRo48wDQYJKoZIhvcNAQELBQAD"
        "ggEBAGE3PDg7p2N8aZyAyO0pGVb/ob9opu12g+diNIdRSjsKIE+TO3uClM2OxT0t"
        "5GBz6Owbe010MQtqBKmX4Zm2LSLUm1kVhPh2ohWmA4hTyN3RG5W0IJ3red6VjrJY"
        "URhZQoXQb0gonxMs+zC+4GQ7+yqzWA1UkrWrURjjJCuljyoWF9sE7qEweomSQWnV"
        "v6bIF599/di1R2l5vcRq1DsQDgKaFY4IpKnvh3RhgO19YxlSS9ERRGBem3Aml9tb"
        "Yac12RmyuxsEAr0v75YeL3pAuq/1Rd5OeKfkm+K06Px3LxwcF92RljXkH6T2U8VM"
        "PEFKedHjYjAag3DUMqSuuGI+ONU="
        "-----END CERTIFICATE-----"_s,
        //  1 s:C = US, O = Let's Encrypt, CN = R3
        //    i:C = US, O = Internet Security Research Group, CN = ISRG Root X1
        "-----BEGIN CERTIFICATE-----"
        "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"
        "nLRbwHOoq7hHwg=="
        "-----END CERTIFICATE-----"_s,
        //  2 s:C = US, O = Internet Security Research Group, CN = ISRG Root X1
        //    i:O = Internet Security Research Group, CN = ISRG Root X1
        "-----BEGIN CERTIFICATE-----"
        "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"
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "wrong.host.badssl.com"_s, State::EndMode::Fail, true, x509VErrUnmatchedCommonName, WTFMove(certs) });
}

TEST(BadSsl, SelfSigned)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = *.badssl.com
        //    i:C = US, ST = California, L = San Francisco, O = BadSSL, CN = *.badssl.com
        "-----BEGIN CERTIFICATE-----"
        "MIIDeTCCAmGgAwIBAgIJANFXLfAvHAYrMA0GCSqGSIb3DQEBCwUAMGIxCzAJBgNV"
        "BAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNp"
        "c2NvMQ8wDQYDVQQKDAZCYWRTU0wxFTATBgNVBAMMDCouYmFkc3NsLmNvbTAeFw0y"
        "MzEwMjcyMjA2MzdaFw0yNTEwMjYyMjA2MzdaMGIxCzAJBgNVBAYTAlVTMRMwEQYD"
        "VQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMQ8wDQYDVQQK"
        "DAZCYWRTU0wxFTATBgNVBAMMDCouYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcNAQEB"
        "BQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5lZ6xKyx2"
        "PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2QULFRtMW"
        "hyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk0TG23U3A"
        "xPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFXRNsKEqve"
        "ww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EFy+b+a1SY"
        "QCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaMyMDAwCQYDVR0T"
        "BAIwADAjBgNVHREEHDAaggwqLmJhZHNzbC5jb22CCmJhZHNzbC5jb20wDQYJKoZI"
        "hvcNAQELBQADggEBACBjhuBhZ2Q6Lct7WmLYaaOMMR4+DSa7nFiUSl/Bpyhv9/Au"
        "comqcBn0ubhotHn39IVcf4LG0XRItbOYRxRQEgozdVGU7DkNN3Piz5wPhNL1knbj"
        "Tdzs2w3BZ43iM+ivLZWfOqpVM4t/of01wyTjCz1682uRUWFChCxBOfikBEfbeeEc"
        "gyqef76me9ZQa1JnFtfw+/VdNWsF6CBcHLfoCXm2zhTfb5q4YB5hpYZJvQfKwGrX"
        "VCir1v/w2EgixqM45bKx5qbgs7bLYX/eJ2nxLje2XQlFQbGq1PWrEEM8CWFd0YWm"
        "oVZ+9O4YO0V0ZK4cB5okJs8c90vRkzuW9bpj4fA="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "self-signed.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT, WTFMove(certs) });
}

TEST(BadSsl, UntrustedRoot)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = *.badssl.com
        //    i:C = US, ST = California, L = San Francisco, O = BadSSL, CN = BadSSL Untrusted Root Certificate Authority
        "-----BEGIN CERTIFICATE-----"
        "MIIEmTCCAoGgAwIBAgIJALj6bZQg6pWYMA0GCSqGSIb3DQEBCwUAMIGBMQswCQYD"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j"
        "aXNjbzEPMA0GA1UECgwGQmFkU1NMMTQwMgYDVQQDDCtCYWRTU0wgVW50cnVzdGVk"
        "IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MB4XDTIzMTAyNzIyMDYzN1oXDTI1"
        "MTAyNjIyMDYzN1owYjELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3JuaWEx"
        "FjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDzANBgNVBAoMBkJhZFNTTDEVMBMGA1UE"
        "AwwMKi5iYWRzc2wuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA"
        "wgTs+IzuBMKz2FDVcFjMkxjrXKhoSbAitfmVnrErLHY+bMBLYExM6rK0wA+AtrD5"
        "csmGAvlcQV0TK39xxEu86ZQuUDemZxxhjPZBQsVG0xaHJ5906wqdEVImIXNshEx5"
        "VeTRa+gGPUgVUq2zKNuq/27/YJVKd2s58STRMbbdTcDE/FO5bUKttXz+rvUV0jNI"
        "5yJxx8IUemwo6jdK3+pstXK0flqiFtxpsVdE2woSq97DD0d0XEEi4Zr5G5PmrSIG"
        "KS6xukkcDCeeo/uL90ByAKySCNmMV4RTgQXL5v5rVJhAJ4XHELtzcO9pGEEHRVV8"
        "+WQ/PSzDqXzrkxpMhtHKhQIDAQABozIwMDAJBgNVHRMEAjAAMCMGA1UdEQQcMBqC"
        "DCouYmFkc3NsLmNvbYIKYmFkc3NsLmNvbTANBgkqhkiG9w0BAQsFAAOCAgEAZ0Jq"
        "em2eFxcmZaz55uhJoQUttQkPTBPcKcDCzYCluB7vTDIlDB0hO8NHNrG3jkZ0hWp2"
        "zioeL0EuAGTyXe09rXaIuj+kAkRcyOH6p96bUVwr2LfNphYU4eXOTO7vGDcKHqfG"
        "XaCsRNTAkPENWEijCS6sPUBqCF6hCogtWjysjc1KPEzaKiCDHfeNKFuGfUTxgzgi"
        "tBYnygt1iElp+HHzm6slC3iMgUqJQuehoPi4LCulWGg3KyExt/VVdo8C9MMPUzhh"
        "JUbiV233L8DE4w+bjfa24gfF9YGtyHYHtjZm2/RpEa4tyXXte/aBa5QuTJ8UVzHd"
        "Kgp07ay2zlEhurc5Wx5oftaPhA4/vEuCUxfFUqrq7g61xCoEe9dEMVaz9tCFnvP2"
        "Eh9cBk/xLMi1eeXezd2C2FDsTEZoLLt1fmtrYHKGxcTNja/rgkeynfxIK2FKARH0"
        "sjiWHG5JfypF58QEsONg1Zmf8KDofItHtIQGspL2yuNHkJe386yt8dxjL3Ougjma"
        "wdzNR6u8/t4ZI5ByXrPsW+T9ku+oEhkDC+hN0hRsI2oJ1N13VVeSO+S6gH8QubBm"
        "CGOhXRcoLLve5unZKRse8DWyfISrmYumYWTSFmp+DdDqT4+fNjkhJtG/zhlaBbB3"
        "mFjFwcxsQrSAAncIdeXU7Yt3PsUbdTdOopz51WI="
        "-----END CERTIFICATE-----"_s,
        // 1 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = BadSSL Untrusted Root Certificate Authority
        //   i:C = US, ST = California, L = San Francisco, O = BadSSL, CN = BadSSL Untrusted Root Certificate Authority
        "-----BEGIN CERTIFICATE-----"
        "MIIGfjCCBGagAwIBAgIJAJeg/PrX5Sj9MA0GCSqGSIb3DQEBCwUAMIGBMQswCQYD"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j"
        "aXNjbzEPMA0GA1UECgwGQmFkU1NMMTQwMgYDVQQDDCtCYWRTU0wgVW50cnVzdGVk"
        "IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MB4XDTE2MDcwNzA2MzEzNVoXDTM2"
        "MDcwMjA2MzEzNVowgYExCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlh"
        "MRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMQ8wDQYDVQQKDAZCYWRTU0wxNDAyBgNV"
        "BAMMK0JhZFNTTCBVbnRydXN0ZWQgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkw"
        "ggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDKQtPMhEH073gis/HISWAi"
        "bOEpCtOsatA3JmeVbaWal8O/5ZO5GAn9dFVsGn0CXAHR6eUKYDAFJLa/3AhjBvWa"
        "tnQLoXaYlCvBjodjLEaFi8ckcJHrAYG9qZqioRQ16Yr8wUTkbgZf+er/Z55zi1yn"
        "CnhWth7kekvrwVDGP1rApeLqbhYCSLeZf5W/zsjLlvJni9OrU7U3a9msvz8mcCOX"
        "fJX9e3VbkD/uonIbK2SvmAGMaOj/1k0dASkZtMws0Bk7m1pTQL+qXDM/h3BQZJa5"
        "DwTcATaa/Qnk6YHbj/MaS5nzCSmR0Xmvs/3CulQYiZJ3kypns1KdqlGuwkfiCCgD"
        "yWJy7NE9qdj6xxLdqzne2DCyuPrjFPS0mmYimpykgbPnirEPBF1LW3GJc9yfhVXE"
        "Cc8OY8lWzxazDNNbeSRDpAGbBeGSQXGjAbliFJxwLyGzZ+cG+G8lc+zSvWjQu4Xp"
        "GJ+dOREhQhl+9U8oyPX34gfKo63muSgo539hGylqgQyzj+SX8OgK1FXXb2LS1gxt"
        "VIR5Qc4MmiEG2LKwPwfU8Yi+t5TYjGh8gaFv6NnksoX4hU42gP5KvjYggDpR+NSN"
        "CGQSWHfZASAYDpxjrOo+rk4xnO+sbuuMk7gORsrl+jgRT8F2VqoR9Z3CEdQxcCjR"
        "5FsfTymZCk3GfIbWKkaeLQIDAQABo4H2MIHzMB0GA1UdDgQWBBRvx4NzSbWnY/91"
        "3m1u/u37l6MsADCBtgYDVR0jBIGuMIGrgBRvx4NzSbWnY/913m1u/u37l6MsAKGB"
        "h6SBhDCBgTELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNV"
        "BAcMDVNhbiBGcmFuY2lzY28xDzANBgNVBAoMBkJhZFNTTDE0MDIGA1UEAwwrQmFk"
        "U1NMIFVudHJ1c3RlZCBSb290IENlcnRpZmljYXRlIEF1dGhvcml0eYIJAJeg/PrX"
        "5Sj9MAwGA1UdEwQFMAMBAf8wCwYDVR0PBAQDAgEGMA0GCSqGSIb3DQEBCwUAA4IC"
        "AQBQU9U8+jTRT6H9AIFm6y50tXTg/ySxRNmeP1Ey9Zf4jUE6yr3Q8xBv9gTFLiY1"
        "qW2qfkDSmXVdBkl/OU3+xb5QOG5hW7wVolWQyKREV5EvUZXZxoH7LVEMdkCsRJDK"
        "wYEKnEErFls5WPXY3bOglBOQqAIiuLQ0f77a2HXULDdQTn5SueW/vrA4RJEKuWxU"
        "iD9XPnVZ9tPtky2Du7wcL9qhgTddpS/NgAuLO4PXh2TQ0EMCll5reZ5AEr0NSLDF"
        "c/koDv/EZqB7VYhcPzr1bhQgbv1dl9NZU0dWKIMkRE/T7vZ97I3aPZqIapC2ulrf"
        "KrlqjXidwrGFg8xbiGYQHPx3tHPZxoM5WG2voI6G3s1/iD+B4V6lUEvivd3f6tq7"
        "d1V/3q1sL5DNv7TvaKGsq8g5un0TAkqaewJQ5fXLigF/yYu5a24/GUD783MdAPFv"
        "gWz8F81evOyRfpf9CAqIswMF+T6Dwv3aw5L9hSniMrblkg+ai0K22JfoBcGOzMtB"
        "Ke/Ps2Za56dTRoY/a4r62hrcGxufXd0mTdPaJLw3sJeHYjLxVAYWQq4QKJQWDgTS"
        "dAEWyN2WXaBFPx5c8KIW95Eu8ShWE00VVC3oA4emoZ2nrzBXLrUScifY6VaYYkkR"
        "2O2tSqU8Ri3XRdgpNPDWp8ZL49KhYGYo3R/k98gnMHiY5g=="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "untrusted-root.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN, WTFMove(certs) });
}

TEST(BadSsl, Revoked)
{
    // Note: PlayStation does not support revoked, so the page should load successfully.
    //       But We're getting a bad cert error because the server certificate has expired.

    Vector<String> certs {
        // 0 s:CN = revoked.badssl.com
        //   i:C = US, O = DigiCert Inc, CN = RapidSSL TLS DV RSA Mixed SHA256 2020 CA-1
        "-----BEGIN CERTIFICATE-----"
        "MIIGhjCCBW6gAwIBAgIQDS5nopiFO5pUUuOihaRXLzANBgkqhkiG9w0BAQsFADBZ"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMTMwMQYDVQQDEypS"
        "YXBpZFNTTCBUTFMgRFYgUlNBIE1peGVkIFNIQTI1NiAyMDIwIENBLTEwHhcNMjEx"
        "MDI3MDAwMDAwWhcNMjIxMDI3MjM1OTU5WjAdMRswGQYDVQQDExJyZXZva2VkLmJh"
        "ZHNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCwdi1VZtxy"
        "iqCehZI4f1vhk42tBsit6Ym07x53WzNFFmB9MzhoBNfJg0KD2TBLVEkUyu2+DHa6"
        "X6ZcM3g/OfJJqIgy7lMhFNOqXFg8Ocz3gLEnH1R5e2yL/0GqOSSVX3G8Sb85O6XV"
        "4aXeHUCBJdyKR4L+zXxLLAS70ydWUaBh8tLLVQglKoXbLAaNDWHCWz6bRtxY/xMn"
        "vgpEHmj+4fa33p+ObMS1GfrX009VqGF522Evapws8cSBu57SAgW6nBSg+fNUeX1p"
        "2bpmHIeVQVAO+V7ht731MSTFISEDis9teFje2TB9A0JS1rAbuclUG1royFPwrCuC"
        "ECemqXAlrvinAgMBAAGjggOEMIIDgDAfBgNVHSMEGDAWgBSkjeW+fHnkcCNtLik0"
        "rSNY3PUxfzAdBgNVHQ4EFgQUsMjOILJ4zB0j7/D+1g4pS6wVcjwwHQYDVR0RBBYw"
        "FIIScmV2b2tlZC5iYWRzc2wuY29tMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUEFjAU"
        "BggrBgEFBQcDAQYIKwYBBQUHAwIwgZsGA1UdHwSBkzCBkDBGoESgQoZAaHR0cDov"
        "L2NybDMuZGlnaWNlcnQuY29tL1JhcGlkU1NMVExTRFZSU0FNaXhlZFNIQTI1NjIw"
        "MjBDQS0xLmNybDBGoESgQoZAaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL1JhcGlk"
        "U1NMVExTRFZSU0FNaXhlZFNIQTI1NjIwMjBDQS0xLmNybDA+BgNVHSAENzA1MDMG"
        "BmeBDAECATApMCcGCCsGAQUFBwIBFhtodHRwOi8vd3d3LmRpZ2ljZXJ0LmNvbS9D"
        "UFMwgYUGCCsGAQUFBwEBBHkwdzAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGln"
        "aWNlcnQuY29tME8GCCsGAQUFBzAChkNodHRwOi8vY2FjZXJ0cy5kaWdpY2VydC5j"
        "b20vUmFwaWRTU0xUTFNEVlJTQU1peGVkU0hBMjU2MjAyMENBLTEuY3J0MAkGA1Ud"
        "EwQCMAAwggF9BgorBgEEAdZ5AgQCBIIBbQSCAWkBZwB2ACl5vvCeOTkh8FZzn2Ol"
        "d+W+V32cYAr4+U1dJlwlXceEAAABfMOk9zcAAAQDAEcwRQIgd7B5GPPeNHD68hvC"
        "MjnIyJWwyHqPYiNY3a35G76Ele0CIQDdJWhHo4RflbHq57wKCZL5WlZyMewH1saX"
        "TUx7kHVkrgB2AFGjsPX9AXmcVm24N3iPDKR6zBsny/eeiEKaDf7UiwXlAAABfMOk"
        "92QAAAQDAEcwRQIgTCL/ZTlrfnsVIXlEwuu4TCrJpceszl9qXei3JMV27BkCIQCU"
        "XgLuFGCAlrwOORYBqDefFbm5ug+iDFoXkKXhMzZF8gB1AEHIyrHfIkZKEMahOglC"
        "h15OMYsbA+vrS8do8JBilgb2AAABfMOk9t8AAAQDAEYwRAIgaIpfULd22n40MqV3"
        "Aqb6p4e720FcgEAsBeUJ3T/MbZ8CIHsdZEhhGXW2N9E8Hjh4hnryeRQIQujdD/84"
        "Ojw22b/ZMA0GCSqGSIb3DQEBCwUAA4IBAQDVjL2+5NyUpLfzSa/EmSbaJ2ja6LjB"
        "usYwthaqUP70dwfrmfLa3XcdGYL3JCo7oGPg2wm+EH/FH4G6r55JzjIwSRePdMbW"
        "zWrYO0d78OAMu8COOh2jf5Ksfo3cpLUwKlcTI6fuJcY37UiyStAB/IXlweLg3Ixh"
        "dKqvaCgmRZSjsUzJXMeSomxKgG/dSPpPBLJKcxfy+R6OXOkj7FP/PseKthiJvHdF"
        "Z0uac3VrV8jAasuEHfTt73AWd47zGo67lfPr+FrkqbHfHTarCt2Rry1xPKuXGAPc"
        "XBqpsdu2SEDHGaeBFAsNzjhv2s/OD2QTKPNNZxss0RZUGW+qCFSjTWdk"
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = DigiCert Inc, CN = RapidSSL TLS DV RSA Mixed SHA256 2020 CA-1
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert Global Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIFUTCCBDmgAwIBAgIQB5g2A63jmQghnKAMJ7yKbDANBgkqhkiG9w0BAQsFADBh"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
        "QTAeFw0yMDA3MTYxMjI1MjdaFw0yMzA1MzEyMzU5NTlaMFkxCzAJBgNVBAYTAlVT"
        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKlJhcGlkU1NMIFRMUyBE"
        "ViBSU0EgTWl4ZWQgU0hBMjU2IDIwMjAgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQAD"
        "ggEPADCCAQoCggEBANpuQ1VVmXvZlaJmxGVYotAMFzoApohbJAeNpzN+49LbgkrM"
        "Lv2tblII8H43vN7UFumxV7lJdPwLP22qa0sV9cwCr6QZoGEobda+4pufG0aSfHQC"
        "QhulaqKpPcYYOPjTwgqJA84AFYj8l/IeQ8n01VyCurMIHA478ts2G6GGtEx0ucnE"
        "fV2QHUL64EC2yh7ybboo5v8nFWV4lx/xcfxoxkFTVnAIRgHrH2vUdOiV9slOix3z"
        "5KPs2rK2bbach8Sh5GSkgp2HRoS/my0tCq1vjyLJeP0aNwPd3rk5O8LiffLev9j+"
        "UKZo0tt0VvTLkdGmSN4h1mVY6DnGfOwp1C5SK0MCAwEAAaOCAgswggIHMB0GA1Ud"
        "DgQWBBSkjeW+fHnkcCNtLik0rSNY3PUxfzAfBgNVHSMEGDAWgBQD3lA1VtFMu2bw"
        "o+IbG8OXsj3RVTAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUHAwEG"
        "CCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAwNAYIKwYBBQUHAQEEKDAmMCQG"
        "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wewYDVR0fBHQwcjA3"
        "oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v"
        "dENBLmNybDA3oDWgM4YxaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0"
        "R2xvYmFsUm9vdENBLmNybDCBzgYDVR0gBIHGMIHDMIHABgRVHSAAMIG3MCgGCCsG"
        "AQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMIGKBggrBgEFBQcC"
        "AjB+DHxBbnkgdXNlIG9mIHRoaXMgQ2VydGlmaWNhdGUgY29uc3RpdHV0ZXMgYWNj"
        "ZXB0YW5jZSBvZiB0aGUgUmVseWluZyBQYXJ0eSBBZ3JlZW1lbnQgbG9jYXRlZCBh"
        "dCBodHRwczovL3d3dy5kaWdpY2VydC5jb20vcnBhLXVhMA0GCSqGSIb3DQEBCwUA"
        "A4IBAQAi49xtSOuOygBycy50quCThG45xIdUAsQCaXFVRa9asPaB/jLINXJL3qV9"
        "J0Gh2bZM0k4yOMeAMZ57smP6JkcJihhOFlfQa18aljd+xNc6b+GX6oFcCHGr+gsE"
        "yPM8qvlKGxc5T5eHVzV6jpjpyzl6VEKpaxH6gdGVpQVgjkOR9yY9XAUlFnzlOCpq"
        "sm7r2ZUKpDfrhUnVzX2nSM15XSj48rVBBAnGJWkLPijlACd3sWFMVUiKRz1C5PZy"
        "el2l7J/W4d99KFLSYgoy5GDmARpwLc//fXfkr40nMY8ibCmxCsjXQTe0fJbtrrLL"
        "yWQlk9VDV296EI/kQOJNLVEkJ54P"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "revoked.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, Pinning)
{
    runTest({ "pinning-test.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, NoCommonName)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron, OU = Multi-Domain SSL
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Organization Validation Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIFcTCCBFmgAwIBAgIQb2K/ExoExTaU4F70HuUTJTANBgkqhkiG9w0BAQsFADCB"
        "ljELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxPDA6BgNV"
        "BAMTM0NPTU9ETyBSU0EgT3JnYW5pemF0aW9uIFZhbGlkYXRpb24gU2VjdXJlIFNl"
        "cnZlciBDQTAeFw0xNzAzMjMwMDAwMDBaFw0yMDA2MjIyMzU5NTlaMGsxCzAJBgNV"
        "BAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxXYWxudXQgQ3Jl"
        "ZWsxFTATBgNVBAoTDEx1Y2FzIEdhcnJvbjEZMBcGA1UECxMQTXVsdGktRG9tYWlu"
        "IFNTTDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ"
        "1XBYzJMY61yoaEmwIrX5lZ6xKyx2PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/"
        "ccRLvOmULlA3pmccYYz2QULFRtMWhyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKt"
        "syjbqv9u/2CVSndrOfEk0TG23U3AxPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3"
        "St/qbLVytH5aohbcabFXRNsKEqveww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7"
        "i/dAcgCskgjZjFeEU4EFy+b+a1SYQCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865Ma"
        "TIbRyoUCAwEAAaOCAeMwggHfMB8GA1UdIwQYMBaAFJrzK9rPrU+2L7sqSEgqErcb"
        "QsEkMB0GA1UdDgQWBBSd7sF7gQs6R2lxGH0RN5O8pRs/+zAOBgNVHQ8BAf8EBAMC"
        "BaAwDAYDVR0TAQH/BAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw"
        "UAYDVR0gBEkwRzA7BgwrBgEEAbIxAQIBAwQwKzApBggrBgEFBQcCARYdaHR0cHM6"
        "Ly9zZWN1cmUuY29tb2RvLmNvbS9DUFMwCAYGZ4EMAQICMFoGA1UdHwRTMFEwT6BN"
        "oEuGSWh0dHA6Ly9jcmwuY29tb2RvY2EuY29tL0NPTU9ET1JTQU9yZ2FuaXphdGlv"
        "blZhbGlkYXRpb25TZWN1cmVTZXJ2ZXJDQS5jcmwwgYsGCCsGAQUFBwEBBH8wfTBV"
        "BggrBgEFBQcwAoZJaHR0cDovL2NydC5jb21vZG9jYS5jb20vQ09NT0RPUlNBT3Jn"
        "YW5pemF0aW9uVmFsaWRhdGlvblNlY3VyZVNlcnZlckNBLmNydDAkBggrBgEFBQcw"
        "AYYYaHR0cDovL29jc3AuY29tb2RvY2EuY29tMCQGA1UdEQQdMBuCGW5vLWNvbW1v"
        "bi1uYW1lLmJhZHNzbC5jb20wDQYJKoZIhvcNAQELBQADggEBAGn8JUbAnmbzu/gW"
        "f+YaNETISJzuGKI6a0TNe3ygcII1ju8modJKiPP6nDuf6X6rpvLc4ptad4cXti30"
        "reCtSi+5d8wZVcG+pNiUu6/ujD+YM6mvv0hiuTUSVmgzLfyT8mYWUrhqRVq3bdid"
        "jOYsUZTMTsCwP4psz9mSLYW1STBxLtZnZAT6e7PShbKx69Oj9k1ChGmzuyTs7bim"
        "wu1LloluOp6TPIJlYfnQaHzNSKEJTaxCZSG/QGFPA00kmuKr0mchm//8sjEdAhcb"
        "riynJbzZtK70hzX9bPDrPgOyyw7Epylr844hOLJMRtLDvzLqV79pSxQV5vTKaizt"
        "F0Y7e2Y="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        // 1 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Organization Validation Secure Server CA
        //   i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Certification Authority
        "-----BEGIN CERTIFICATE-----"
        "MIIGDjCCA/agAwIBAgIQNoJef7WkgZN+9tFza7k8pjANBgkqhkiG9w0BAQwFADCB"
        "hTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxKzApBgNV"
        "BAMTIkNPTU9ETyBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTQwMjEy"
        "MDAwMDAwWhcNMjkwMjExMjM1OTU5WjCBljELMAkGA1UEBhMCR0IxGzAZBgNVBAgT"
        "EkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEaMBgGA1UEChMR"
        "Q09NT0RPIENBIExpbWl0ZWQxPDA6BgNVBAMTM0NPTU9ETyBSU0EgT3JnYW5pemF0"
        "aW9uIFZhbGlkYXRpb24gU2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEB"
        "BQADggEPADCCAQoCggEBALkU2YXyQURX/zBEHtw8RKMXuG4B+KNfwqkhHc5Z9Ozz"
        "iKkJMjyxi2OkPic284/5OGYuB5dBj0um3cNfnnM858ogDU98MgXPwS5IZUqF0B9W"
        "MW2O5cYy1Bu8n32W/JjXT/j0WFb440W+kRiC5Iq+r81SN1GHTx6Xweg6rvn/RuRl"
        "Pz/DR4MvzLhCXi1+91porl1LwKY1IfWGo8hJi5hjYA3JIUjCkjBlRrKGNQRCJX6t"
        "p05LEkAAeohoXG+fo6R4ESGuPQsOvkUUI8/rddf2oPG8RWxevKEy7PNYeEIoCzoB"
        "dvDFoJ7BaXDej0umed/ydrbjDxN8GDuxUWxqIDnOnmkCAwEAAaOCAWUwggFhMB8G"
        "A1UdIwQYMBaAFLuvfgI9+qbxPISOre44mOzZMjLUMB0GA1UdDgQWBBSa8yvaz61P"
        "ti+7KkhIKhK3G0LBJDAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIB"
        "ADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwGwYDVR0gBBQwEjAGBgRV"
        "HSAAMAgGBmeBDAECAjBMBgNVHR8ERTBDMEGgP6A9hjtodHRwOi8vY3JsLmNvbW9k"
        "b2NhLmNvbS9DT01PRE9SU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDBxBggr"
        "BgEFBQcBAQRlMGMwOwYIKwYBBQUHMAKGL2h0dHA6Ly9jcnQuY29tb2RvY2EuY29t"
        "L0NPTU9ET1JTQUFkZFRydXN0Q0EuY3J0MCQGCCsGAQUFBzABhhhodHRwOi8vb2Nz"
        "cC5jb21vZG9jYS5jb20wDQYJKoZIhvcNAQEMBQADggIBAGmKNmiaHjtlC+B8z6ar"
        "cTuvYaQ/5GQBSRDTHY/i1e1n055bl71CHgf50Ltt9zKVWiIpYvgMnFlWJzagIhIR"
        "+kf0UclZeylKpUg1fMWXZuAnJTsVejJ1SpH7pmue4lP6DYwT+yO4CxIsru3bHUeQ"
        "1dCTaXaROBU01xjqfrxrWN4qOZADRARKVtho5fV8aX6efVRL0NiGq2dmE1deiSoX"
        "rS2uvUAOZu2K/1S0wQHLqeBHuhFhj62uI0gqxiV5iRxBBJXAEepXK9a0l/qx6RVi"
        "7Epxd/3zoZza9msAKcUy5/pO6rMqpxiXHFinQjZf7BTP+HsO993MiBWamlzI8SDH"
        "0YZyoRebrrr+bKgy0QB2SXP3PyeHPLbJLfqqkJDJCgmfyWkfBxmpv966+AuIgkQW"
        "EH8HwIAiX3+8MN66zQd5ZFbY//NPnDC7bh5RS+bNvRfExb/IP46xH4pGtwZDb2It"
        "z1GdRcqK6ROLwMeRvlu2+jdKif7wndoTJiIsBpA+ixOYoBnW3dpKSH89D4mdJHJL"
        "DntE/9Q2toN2I1iLFGy4XfdhbTl27d0SPWuHiJeRvsBGAh52HN22r1xP9QDWnE2p"
        "4J6ijvyxFnlcIdNFgZoMOWxtKNcl0rcRkND23m9e9Pqki2Z3ci+bkEAsUhJg+f+1"
        "cC6JmnkJiYEt7Fx4b4GH8fxV"
        "-----END CERTIFICATE-----"_s,
        // 2 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Certification Authority
        //   i:C = SE, O = AddTrust AB, OU = AddTrust External TTP Network, CN = AddTrust External CA Root
        "-----BEGIN CERTIFICATE-----"
        "MIIFdDCCBFygAwIBAgIQJ2buVutJ846r13Ci/ITeIjANBgkqhkiG9w0BAQwFADBv"
        "MQswCQYDVQQGEwJTRTEUMBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFk"
        "ZFRydXN0IEV4dGVybmFsIFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBF"
        "eHRlcm5hbCBDQSBSb290MB4XDTAwMDUzMDEwNDgzOFoXDTIwMDUzMDEwNDgzOFow"
        "gYUxCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAO"
        "BgNVBAcTB1NhbGZvcmQxGjAYBgNVBAoTEUNPTU9ETyBDQSBMaW1pdGVkMSswKQYD"
        "VQQDEyJDT01PRE8gUlNBIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIICIjANBgkq"
        "hkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAkehUktIKVrGsDSTdxc9EZ3SZKzejfSNw"
        "AHG8U9/E+ioSj0t/EFa9n3Byt2F/yUsPF6c947AEYe7/EZfH9IY+Cvo+XPmT5jR6"
        "2RRr55yzhaCCenavcZDX7P0N+pxs+t+wgvQUfvm+xKYvT3+Zf7X8Z0NyvQwA1onr"
        "ayzT7Y+YHBSrfuXjbvzYqOSSJNpDa2K4Vf3qwbxstovzDo2a5JtsaZn4eEgwRdWt"
        "4Q08RWD8MpZRJ7xnw8outmvqRsfHIKCxH2XeSAi6pE6p8oNGN4Tr6MyBSENnTnIq"
        "m1y9TBsoilwie7SrmNnu4FGDwwlGTm0+mfqVF9p8M1dBPI1R7Qu2XK8sYxrfV8g/"
        "vOldxJuvRZnio1oktLqpVj3Pb6r/SVi+8Kj/9Lit6Tf7urj0Czr56ENCHonYhMsT"
        "8dm74YlguIwoVqwUHZwK53Hrzw7dPamWoUi9PPevtQ0iTMARgexWO/bTouJbt7IE"
        "IlKVgJNp6I5MZfGRAy1wdALqi2cVKWlSArvX31BqVUa/oKMoYX9w0MOiqiwhqkfO"
        "KJwGRXa/ghgntNWutMtQ5mv0TIZxMOmm3xaG4Nj/QN370EKIf6MzOi5cHkERgWPO"
        "GHFrK+ymircxXDpqR+DDeVnWIBqv8mqYqnK8V0rSS527EPywTEHl7R09XiidnMy/"
        "s1Hap0flhFMCAwEAAaOB9DCB8TAfBgNVHSMEGDAWgBStvZh6NLQm9/rEJlTvA73g"
        "JMtUGjAdBgNVHQ4EFgQUu69+Aj36pvE8hI6t7jiY7NkyMtQwDgYDVR0PAQH/BAQD"
        "AgGGMA8GA1UdEwEB/wQFMAMBAf8wEQYDVR0gBAowCDAGBgRVHSAAMEQGA1UdHwQ9"
        "MDswOaA3oDWGM2h0dHA6Ly9jcmwudXNlcnRydXN0LmNvbS9BZGRUcnVzdEV4dGVy"
        "bmFsQ0FSb290LmNybDA1BggrBgEFBQcBAQQpMCcwJQYIKwYBBQUHMAGGGWh0dHA6"
        "Ly9vY3NwLnVzZXJ0cnVzdC5jb20wDQYJKoZIhvcNAQEMBQADggEBAGS/g/FfmoXQ"
        "zbihKVcN6Fr30ek+8nYEbvFScLsePP9NDXRqzIGCJdPDoCpdTPW6i6FtxFQJdcfj"
        "Jw5dhHk3QBN39bSsHNA7qxcS1u80GH4r6XnTq1dFDK8o+tDb5VCViLvfhVdpfZLY"
        "Uspzgb8c8+a4bmYRBbMelC1/kZWSWfFMzqORcUx8Rww7Cxn2obFshj5cqsQugsv5"
        "B5a6SE2Q8pTIqXOi6wZ7I53eovNNVZ96YUWYGGjHXkBrI/V5eu+MtWuLt29G9Hvx"
        "PUsE2JOAWVrgQSQdso8VYFhH2+9uRv0V9dlfmrPb2LjkQLPNlzmuhbsdjrzch5vR"
        "pu/xO28QOG8="
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "no-common-name.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, NoSubject)
{
    Vector<String> certs {
        //  0 s:
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = UbiquiTLS\E2\84\A2 DV RSA Server CA
        "-----BEGIN CERTIFICATE-----"
        "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"
        "yd8BGrY2DQplAarq7e1IBIK+tBSRIYF3Ag=="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = UbiquiTLS\E2\84\A2 DV RSA Server CA
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Certification Authority
        "-----BEGIN CERTIFICATE-----"
        "MIIF+TCCA+GgAwIBAgIRALEh69i9ODvCDLdUvBhDX/swDQYJKoZIhvcNAQEMBQAw"
        "gYUxCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAO"
        "BgNVBAcTB1NhbGZvcmQxGjAYBgNVBAoTEUNPTU9ETyBDQSBMaW1pdGVkMSswKQYD"
        "VQQDEyJDT01PRE8gUlNBIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTE2MDMy"
        "OTAwMDAwMFoXDTMxMDMyOTIzNTk1OVowgYAxCzAJBgNVBAYTAkdCMRswGQYDVQQI"
        "ExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNVBAcTB1NhbGZvcmQxGjAYBgNVBAoT"
        "EUNPTU9ETyBDQSBMaW1pdGVkMSYwJAYDVQQDDB1VYmlxdWlUTFPihKIgRFYgUlNB"
        "IFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL23CxiN"
        "bAfZclMlGi9tHpl6dCSxT4yRM79i/h3u/HbErdS+3jiq9oPZfCqgbJ0PVIOBelGb"
        "EhvBBbkW/wPlDq3vEEt9nP/P3cRVGu+i0X/puXA51E5TgrEn5/jdorBANmhItrG4"
        "uZietPJmFx6awA3ANC2MVnvmT1s5qS0cnrdFh2rLRwfANm4S0UbJNpHSItFinh0J"
        "XtQFyc/QE+fGAH1sC8aW34Rt+qQ1+msY7hvNpXKZLR8Fy5AVi089iv5cK8bqzVaQ"
        "F8kJTiukJ9vQ/e9CC7x9/TxX/oYn5bIejxsaoGPi1BgeRkS7J42MeNpYmh7kUFN6"
        "ZZm5Yh8OB571L30CAwEAAaOCAWUwggFhMB8GA1UdIwQYMBaAFLuvfgI9+qbxPISO"
        "re44mOzZMjLUMB0GA1UdDgQWBBQ4EsZ5AmY4AuM3JOWLD/9FlWnmMTAOBgNVHQ8B"
        "Af8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNVHSUEFjAUBggrBgEFBQcD"
        "AQYIKwYBBQUHAwIwGwYDVR0gBBQwEjAGBgRVHSAAMAgGBmeBDAECATBMBgNVHR8E"
        "RTBDMEGgP6A9hjtodHRwOi8vY3JsLmNvbW9kb2NhLmNvbS9DT01PRE9SU0FDZXJ0"
        "aWZpY2F0aW9uQXV0aG9yaXR5LmNybDBxBggrBgEFBQcBAQRlMGMwOwYIKwYBBQUH"
        "MAKGL2h0dHA6Ly9jcnQuY29tb2RvY2EuY29tL0NPTU9ET1JTQUFkZFRydXN0Q0Eu"
        "Y3J0MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wDQYJKoZI"
        "hvcNAQEMBQADggIBAC3C7NUxneoMxtAT5QcdKTxVO2TmjbQtmYaZ6b06ySZC+RZm"
        "GrktbzTwGh0jb33K/Wx9X1GXhsZIACgZM9bbeopskqlg/x5UDtHzaDWs+XtWMhET"
        "aRtQRyRJ6bxTU64DSjnHbgLedToXmA2H8BF+DrRwEGG0twg9EjhIOJbvJ8Jcsdzh"
        "b2hE4w48VwpQzH+Sg78TnGr6MimbnkFpuE+DO5q/x2AmeuOdmcl/3ufs3EjKeJTr"
        "ybyjutGfI+LM3UTw1FhVzeMUPJBrebiy8tTP2/Rn8sJRdK3J+lmfY+sgfkcpXjMO"
        "PA8esFM43Wawn2fLaQALi0Vpm36oc9ECIqjAunxQ/OmLl2uSvERNln/hmSlKvPEY"
        "vA7lUXzRAxbpR0K7n+GBOMvkT9n1JGscHR9Oy3QGVksCT5bVvuGhH9m6BM3lnskp"
        "3eyOXy0PpT9eJVflrpA8HVcJXGNCFDBT3iWUtHXXfopJvBe6RcEyu7aLWPNJ0/wM"
        "n+EdLjUShlFqczNfw15eAdnTHkPCFW588pj7Hz0MaTeIqOBZWTHvc/BADJbRZuh7"
        "jVXvr24Pv15WVEt/GKqm+VkvusRTVMVQm9fDH8rBIxBmpzt0uBTcg8IWY8HtGfEt"
        "5R+qkfsm6EWtj4APtivR5qC2mzEDxbZSlduxifzG194rAGGbcn2KeOA0QwNE"
        "-----END CERTIFICATE-----"_s,
        //  2 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO RSA Certification Authority
        //    i:C = SE, O = AddTrust AB, OU = AddTrust External TTP Network, CN = AddTrust External CA Root
        "-----BEGIN CERTIFICATE-----"
        "MIIFdDCCBFygAwIBAgIQJ2buVutJ846r13Ci/ITeIjANBgkqhkiG9w0BAQwFADBv"
        "MQswCQYDVQQGEwJTRTEUMBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFk"
        "ZFRydXN0IEV4dGVybmFsIFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBF"
        "eHRlcm5hbCBDQSBSb290MB4XDTAwMDUzMDEwNDgzOFoXDTIwMDUzMDEwNDgzOFow"
        "gYUxCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAO"
        "BgNVBAcTB1NhbGZvcmQxGjAYBgNVBAoTEUNPTU9ETyBDQSBMaW1pdGVkMSswKQYD"
        "VQQDEyJDT01PRE8gUlNBIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIICIjANBgkq"
        "hkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAkehUktIKVrGsDSTdxc9EZ3SZKzejfSNw"
        "AHG8U9/E+ioSj0t/EFa9n3Byt2F/yUsPF6c947AEYe7/EZfH9IY+Cvo+XPmT5jR6"
        "2RRr55yzhaCCenavcZDX7P0N+pxs+t+wgvQUfvm+xKYvT3+Zf7X8Z0NyvQwA1onr"
        "ayzT7Y+YHBSrfuXjbvzYqOSSJNpDa2K4Vf3qwbxstovzDo2a5JtsaZn4eEgwRdWt"
        "4Q08RWD8MpZRJ7xnw8outmvqRsfHIKCxH2XeSAi6pE6p8oNGN4Tr6MyBSENnTnIq"
        "m1y9TBsoilwie7SrmNnu4FGDwwlGTm0+mfqVF9p8M1dBPI1R7Qu2XK8sYxrfV8g/"
        "vOldxJuvRZnio1oktLqpVj3Pb6r/SVi+8Kj/9Lit6Tf7urj0Czr56ENCHonYhMsT"
        "8dm74YlguIwoVqwUHZwK53Hrzw7dPamWoUi9PPevtQ0iTMARgexWO/bTouJbt7IE"
        "IlKVgJNp6I5MZfGRAy1wdALqi2cVKWlSArvX31BqVUa/oKMoYX9w0MOiqiwhqkfO"
        "KJwGRXa/ghgntNWutMtQ5mv0TIZxMOmm3xaG4Nj/QN370EKIf6MzOi5cHkERgWPO"
        "GHFrK+ymircxXDpqR+DDeVnWIBqv8mqYqnK8V0rSS527EPywTEHl7R09XiidnMy/"
        "s1Hap0flhFMCAwEAAaOB9DCB8TAfBgNVHSMEGDAWgBStvZh6NLQm9/rEJlTvA73g"
        "JMtUGjAdBgNVHQ4EFgQUu69+Aj36pvE8hI6t7jiY7NkyMtQwDgYDVR0PAQH/BAQD"
        "AgGGMA8GA1UdEwEB/wQFMAMBAf8wEQYDVR0gBAowCDAGBgRVHSAAMEQGA1UdHwQ9"
        "MDswOaA3oDWGM2h0dHA6Ly9jcmwudXNlcnRydXN0LmNvbS9BZGRUcnVzdEV4dGVy"
        "bmFsQ0FSb290LmNybDA1BggrBgEFBQcBAQQpMCcwJQYIKwYBBQUHMAGGGWh0dHA6"
        "Ly9vY3NwLnVzZXJ0cnVzdC5jb20wDQYJKoZIhvcNAQEMBQADggEBAGS/g/FfmoXQ"
        "zbihKVcN6Fr30ek+8nYEbvFScLsePP9NDXRqzIGCJdPDoCpdTPW6i6FtxFQJdcfj"
        "Jw5dhHk3QBN39bSsHNA7qxcS1u80GH4r6XnTq1dFDK8o+tDb5VCViLvfhVdpfZLY"
        "Uspzgb8c8+a4bmYRBbMelC1/kZWSWfFMzqORcUx8Rww7Cxn2obFshj5cqsQugsv5"
        "B5a6SE2Q8pTIqXOi6wZ7I53eovNNVZ96YUWYGGjHXkBrI/V5eu+MtWuLt29G9Hvx"
        "PUsE2JOAWVrgQSQdso8VYFhH2+9uRv0V9dlfmrPb2LjkQLPNlzmuhbsdjrzch5vR"
        "pu/xO28QOG8="
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "no-subject.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, IncompleteChain)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron Torres, CN = *.badssl.com
        //    i:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIGqDCCBZCgAwIBAgIQCvBs2jemC2QTQvCh6x1Z/TANBgkqhkiG9w0BAQsFADBN"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMScwJQYDVQQDEx5E"
        "aWdpQ2VydCBTSEEyIFNlY3VyZSBTZXJ2ZXIgQ0EwHhcNMjAwMzIzMDAwMDAwWhcN"
        "MjIwNTE3MTIwMDAwWjBuMQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5p"
        "YTEVMBMGA1UEBxMMV2FsbnV0IENyZWVrMRwwGgYDVQQKExNMdWNhcyBHYXJyb24g"
        "VG9ycmVzMRUwEwYDVQQDDAwqLmJhZHNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUA"
        "A4IBDwAwggEKAoIBAQDCBOz4jO4EwrPYUNVwWMyTGOtcqGhJsCK1+ZWesSssdj5s"
        "wEtgTEzqsrTAD4C2sPlyyYYC+VxBXRMrf3HES7zplC5QN6ZnHGGM9kFCxUbTFocn"
        "n3TrCp0RUiYhc2yETHlV5NFr6AY9SBVSrbMo26r/bv9glUp3aznxJNExtt1NwMT8"
        "U7ltQq21fP6u9RXSM0jnInHHwhR6bCjqN0rf6my1crR+WqIW3GmxV0TbChKr3sMP"
        "R3RcQSLhmvkbk+atIgYpLrG6SRwMJ56j+4v3QHIArJII2YxXhFOBBcvm/mtUmEAn"
        "hccQu3Nw72kYQQdFVXz5ZD89LMOpfOuTGkyG0cqFAgMBAAGjggNhMIIDXTAfBgNV"
        "HSMEGDAWgBQPgGEcgjFh1S8o541GOLQs4cbZ4jAdBgNVHQ4EFgQUne7Be4ELOkdp"
        "cRh9ETeTvKUbP/swIwYDVR0RBBwwGoIMKi5iYWRzc2wuY29tggpiYWRzc2wuY29t"
        "MA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw"
        "awYDVR0fBGQwYjAvoC2gK4YpaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL3NzY2Et"
        "c2hhMi1nNi5jcmwwL6AtoCuGKWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9zc2Nh"
        "LXNoYTItZzYuY3JsMEwGA1UdIARFMEMwNwYJYIZIAYb9bAEBMCowKAYIKwYBBQUH"
        "AgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwCAYGZ4EMAQIDMHwGCCsG"
        "AQUFBwEBBHAwbjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29t"
        "MEYGCCsGAQUFBzAChjpodHRwOi8vY2FjZXJ0cy5kaWdpY2VydC5jb20vRGlnaUNl"
        "cnRTSEEyU2VjdXJlU2VydmVyQ0EuY3J0MAwGA1UdEwEB/wQCMAAwggF+BgorBgEE"
        "AdZ5AgQCBIIBbgSCAWoBaAB2ALvZ37wfinG1k5Qjl6qSe0c4V5UKq1LoGpCWZDaO"
        "HtGFAAABcQhGXioAAAQDAEcwRQIgDfWVBXEuUZC2YP4Si3AQDidHC4U9e5XTGyG7"
        "SFNDlRkCIQCzikrA1nf7boAdhvaGu2Vkct3VaI+0y8p3gmonU5d9DwB2ACJFRQdZ"
        "VSRWlj+hL/H3bYbgIyZjrcBLf13Gg1xu4g8CAAABcQhGXlsAAAQDAEcwRQIhAMWi"
        "Vsi2vYdxRCRsu/DMmCyhY0iJPKHE2c6ejPycIbgqAiAs3kSSS0NiUFiHBw7QaQ/s"
        "GO+/lNYvjExlzVUWJbgNLwB2AFGjsPX9AXmcVm24N3iPDKR6zBsny/eeiEKaDf7U"
        "iwXlAAABcQhGXnoAAAQDAEcwRQIgKsntiBqt8Au8DAABFkxISELhP3U/wb5lb76p"
        "vfenWL0CIQDr2kLhCWP/QUNxXqGmvr1GaG9EuokTOLEnGPhGv1cMkDANBgkqhkiG"
        "9w0BAQsFAAOCAQEA0RGxlwy3Tl0lhrUAn2mIi8LcZ9nBUyfAcCXCtYyCdEbjIP64"
        "xgX6pzTt0WJoxzlT+MiK6fc0hECZXqpkTNVTARYtGkJoljlTK2vAdHZ0SOpm9OT4"
        "RLfjGnImY0hiFbZ/LtsvS2Zg7cVJecqnrZe/za/nbDdljnnrll7C8O5naQuKr4te"
        "uice3e8a4TtviFwS/wdDnJ3RrE83b1IljILbU5SV0X1NajyYkUWS7AnOmrFUUByz"
        "MwdGrM6kt0lfJy/gvGVsgIKZocHdedPeECqAtq7FAJYanOsjNN9RbBOGhbwq0/FP"
        "CC01zojqS10nGowxzOiqyB4m6wytmzf0QwjpMw=="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "incomplete-chain.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, SHA256)
{
    runTest({ "sha256.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, SHA384)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron Torres, CN = sha384.badssl.com
        //    i:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIGwDCCBaigAwIBAgIQDmUWRxWq6Q5sixyf9z28HDANBgkqhkiG9w0BAQwFADBN"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMScwJQYDVQQDEx5E"
        "aWdpQ2VydCBTSEEyIFNlY3VyZSBTZXJ2ZXIgQ0EwHhcNMjAwMzIzMDAwMDAwWhcN"
        "MjIwNDAxMTIwMDAwWjBzMQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5p"
        "YTEVMBMGA1UEBxMMV2FsbnV0IENyZWVrMRwwGgYDVQQKExNMdWNhcyBHYXJyb24g"
        "VG9ycmVzMRowGAYDVQQDExFzaGEzODQuYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcN"
        "AQEBBQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5lZ6x"
        "Kyx2PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2QULF"
        "RtMWhyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk0TG2"
        "3U3AxPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFXRNsK"
        "Eqveww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EFy+b+"
        "a1SYQCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaOCA3QwggNw"
        "MB8GA1UdIwQYMBaAFA+AYRyCMWHVLyjnjUY4tCzhxtniMB0GA1UdDgQWBBSd7sF7"
        "gQs6R2lxGH0RN5O8pRs/+zA2BgNVHREELzAtggpiYWRzc2wuY29tggwqLmJhZHNz"
        "bC5jb22CEXNoYTM4NC5iYWRzc2wuY29tMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUE"
        "FjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwawYDVR0fBGQwYjAvoC2gK4YpaHR0cDov"
        "L2NybDMuZGlnaWNlcnQuY29tL3NzY2Etc2hhMi1nNi5jcmwwL6AtoCuGKWh0dHA6"
        "Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9zc2NhLXNoYTItZzYuY3JsMEwGA1UdIARFMEMw"
        "NwYJYIZIAYb9bAEBMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0"
        "LmNvbS9DUFMwCAYGZ4EMAQIDMHwGCCsGAQUFBwEBBHAwbjAkBggrBgEFBQcwAYYY"
        "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEYGCCsGAQUFBzAChjpodHRwOi8vY2Fj"
        "ZXJ0cy5kaWdpY2VydC5jb20vRGlnaUNlcnRTSEEyU2VjdXJlU2VydmVyQ0EuY3J0"
        "MAwGA1UdEwEB/wQCMAAwggF+BgorBgEEAdZ5AgQCBIIBbgSCAWoBaAB3ACl5vvCe"
        "OTkh8FZzn2Old+W+V32cYAr4+U1dJlwlXceEAAABcQg8p/oAAAQDAEgwRgIhAP22"
        "rbGHboA6UnZbiIBN+aBfW3Hj3shYqHLHFxLSWIYOAiEA58wvrY+hjrqCGUz/xyRn"
        "wZ+FxrKROL6eTpHF2BrYqt8AdgBByMqx3yJGShDGoToJQodeTjGLGwPr60vHaPCQ"
        "YpYG9gAAAXEIPKf6AAAEAwBHMEUCIB2I3uY9P5UbeOcPkTMlJAHsY6Wn9KuWfH3K"
        "yABgvWR/AiEAsVIZ+Nu68hEutD8hEwVCfVroqzxGHst7SwwhjStNpH8AdQC72d+8"
        "H4pxtZOUI5eqkntHOFeVCqtS6BqQlmQ2jh7RhQAAAXEIPKgrAAAEAwBGMEQCIAEQ"
        "SLEZ1hh34u3fS9kUZ6a6MDufkmk4v2RnphI/XLmZAiBK3YMPBlkl8sJ+A/PyfB2j"
        "XOghmsvKSHkHasXYfOUE9zANBgkqhkiG9w0BAQwFAAOCAQEAxQ5hy3JxFIvpzM4L"
        "/rAINzavnCvmelvTECi0HVvZMxkVQox4LP7KQv7ckAi9OdE7RR3YrKSUPd98Tc+e"
        "ksHr2xa+sVbs2ZSnbFa2xCUpA2Ft/1Z6VHLaO2y44apgNET2uFeZwDSS8TyugmV7"
        "aaLE8whUZ2H/7P0gcOg6BYQDa6zCgYK+h+z742htfQ7AWKCtQX3Sp0Mnjo0s0X+t"
        "OBearDUV6WPxdxyRCQ9YLPRA4n+jsTcV4N991j7EK6BY7CxvvWcuXlgnsM+BWEx4"
        "1cFSaddGEX2CMy3ch2kuYy5+EPyzlvpKDktuNPrzk5NlC6ozrA4dze0ft7tHqcXG"
        "vj33ig=="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert Global Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
        "QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT"
        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg"
        "U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB"
        "ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83"
        "nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd"
        "KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f"
        "/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX"
        "kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0"
        "/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C"
        "AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY"
        "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6"
        "Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1"
        "oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD"
        "QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v"
        "d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh"
        "xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB"
        "CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl"
        "5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA"
        "8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC"
        "2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit"
        "c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0"
        "j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "sha384.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, SHA512)
{
    Vector<String> certs {
        // 0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron Torres, CN = sha512.badssl.com
        //   i:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIGvjCCBaagAwIBAgIQAoAY7JB8mex2mpP8gaPFcDANBgkqhkiG9w0BAQ0FADBN"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMScwJQYDVQQDEx5E"
        "aWdpQ2VydCBTSEEyIFNlY3VyZSBTZXJ2ZXIgQ0EwHhcNMjAwMzIzMDAwMDAwWhcN"
        "MjIwNDAxMTIwMDAwWjBzMQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5p"
        "YTEVMBMGA1UEBxMMV2FsbnV0IENyZWVrMRwwGgYDVQQKExNMdWNhcyBHYXJyb24g"
        "VG9ycmVzMRowGAYDVQQDExFzaGE1MTIuYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcN"
        "AQEBBQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5lZ6x"
        "Kyx2PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2QULF"
        "RtMWhyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk0TG2"
        "3U3AxPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFXRNsK"
        "Eqveww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EFy+b+"
        "a1SYQCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaOCA3IwggNu"
        "MB8GA1UdIwQYMBaAFA+AYRyCMWHVLyjnjUY4tCzhxtniMB0GA1UdDgQWBBSd7sF7"
        "gQs6R2lxGH0RN5O8pRs/+zA2BgNVHREELzAtggpiYWRzc2wuY29tggwqLmJhZHNz"
        "bC5jb22CEXNoYTUxMi5iYWRzc2wuY29tMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUE"
        "FjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwawYDVR0fBGQwYjAvoC2gK4YpaHR0cDov"
        "L2NybDMuZGlnaWNlcnQuY29tL3NzY2Etc2hhMi1nNi5jcmwwL6AtoCuGKWh0dHA6"
        "Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9zc2NhLXNoYTItZzYuY3JsMEwGA1UdIARFMEMw"
        "NwYJYIZIAYb9bAEBMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0"
        "LmNvbS9DUFMwCAYGZ4EMAQIDMHwGCCsGAQUFBwEBBHAwbjAkBggrBgEFBQcwAYYY"
        "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEYGCCsGAQUFBzAChjpodHRwOi8vY2Fj"
        "ZXJ0cy5kaWdpY2VydC5jb20vRGlnaUNlcnRTSEEyU2VjdXJlU2VydmVyQ0EuY3J0"
        "MAwGA1UdEwEB/wQCMAAwggF8BgorBgEEAdZ5AgQCBIIBbASCAWgBZgB1AKS5CZC0"
        "GFgUh7sTosxncAo8NZgE+RvfuON3zQ7IDdwQAAABcQg+QaMAAAQDAEYwRAIgMzeU"
        "9Tesw0Qf1eGy4AaYZc+k2g0t81tSvuo25Cl/hYMCIHLV0Sf1wgW+i9vMwBRQTnG3"
        "/jTBNgYnoWTkUr1L9fRLAHYAQcjKsd8iRkoQxqE6CUKHXk4xixsD6+tLx2jwkGKW"
        "BvYAAAFxCD5BjAAABAMARzBFAiBHmSNJ0EyXZlohOdU+2Nf3Ja+GsQZpra8t/01S"
        "kKYNRQIhANivCnWgABRA+pN0PODoln5WELpOR1ZK7Z3f6VBDONnAAHUAu9nfvB+K"
        "cbWTlCOXqpJ7RzhXlQqrUugakJZkNo4e0YUAAAFxCD5BtwAABAMARjBEAiBcKblK"
        "Qzl9wC6+fDDjLTbUzgA6i/SBAPIcMjeQykpgTAIgI9U4sdszlHj3vfVpCTEBs/eQ"
        "yiXWQUosnvctseYc6t0wDQYJKoZIhvcNAQENBQADggEBAJvXzThMX/gWXyri6fJB"
        "7UdQIhe6cFu4F/TFBTgD1WJ0obJ4R7uQL1krME3NYw10UKiBxKQeV6i9YvKGO11E"
        "kZYUpyT/Ev+CvqhJuJuv8l+tiu8kGzbVQVZ3cwoK+Y4im+0+HsSxzV6RLj8DwhA/"
        "j4lUmDn0dR/oYUV4sI8uimZyRq+uSafMtYUHv3Y3zWnT9JwABQWHWUwII2ZRUAKm"
        "iBu7zNq6T1I+fdF8KG4DoptBTujti9bmz9BPbipxh37GDmjtP5l5lRQidQoLHh5X"
        "JLJgRFsmlZ1WONkUFL7q5oj2hjUFIEEQbvvEUQ91SYWbD03bUxI1jqcDUqileIb1"
        "SS4="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert Global Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
        "QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT"
        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg"
        "U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB"
        "ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83"
        "nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd"
        "KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f"
        "/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX"
        "kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0"
        "/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C"
        "AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY"
        "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6"
        "Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1"
        "oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD"
        "QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v"
        "d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh"
        "xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB"
        "CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl"
        "5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA"
        "8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC"
        "2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit"
        "c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0"
        "j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "sha512.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, _1000SANs)
{
    Vector<String> certs {
        // 0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron Torres, CN = 1000-sans.badssl.com
        //   i:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIJvpTCCbo2gAwIBAgIQAkxUF2G06wzFEu7GfRhQ3zANBgkqhkiG9w0BAQsFADBN"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMScwJQYDVQQDEx5E"
        "aWdpQ2VydCBTSEEyIFNlY3VyZSBTZXJ2ZXIgQ0EwHhcNMTkxMDA1MDAwMDAwWhcN"
        "MjExMDEzMTIwMDAwWjB2MQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5p"
        "YTEVMBMGA1UEBxMMV2FsbnV0IENyZWVrMRwwGgYDVQQKExNMdWNhcyBHYXJyb24g"
        "VG9ycmVzMR0wGwYDVQQDExQxMDAwLXNhbnMuYmFkc3NsLmNvbTCCASIwDQYJKoZI"
        "hvcNAQEBBQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5"
        "lZ6xKyx2PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2"
        "QULFRtMWhyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk"
        "0TG23U3AxPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFX"
        "RNsKEqveww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EF"
        "y+b+a1SYQCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaOCbFYw"
        "gmxSMB8GA1UdIwQYMBaAFA+AYRyCMWHVLyjnjUY4tCzhxtniMB0GA1UdDgQWBBSd"
        "7sF7gQs6R2lxGH0RN5O8pRs/+zCCaRcGA1UdEQSCaQ4wgmkKghQxMDAwLXNhbnMu"
        "YmFkc3NsLmNvbYIXd293bW9hcnNhbnMyLmJhZHNzbC5jb22CF3dvd21vYXJzYW5z"
        "My5iYWRzc2wuY29tghd3b3dtb2Fyc2FuczQuYmFkc3NsLmNvbYIXd293bW9hcnNh"
        "bnM1LmJhZHNzbC5jb22CF3dvd21vYXJzYW5zNi5iYWRzc2wuY29tghd3b3dtb2Fy"
        "c2FuczcuYmFkc3NsLmNvbYIXd293bW9hcnNhbnM4LmJhZHNzbC5jb22CF3dvd21v"
        "YXJzYW5zOS5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczEwLmJhZHNzbC5jb22CGHdv"
        "d21vYXJzYW5zMTEuYmFkc3NsLmNvbYIYd293bW9hcnNhbnMxMi5iYWRzc2wuY29t"
        "ghh3b3dtb2Fyc2FuczEzLmJhZHNzbC5jb22CGHdvd21vYXJzYW5zMTQuYmFkc3Ns"
        "LmNvbYIYd293bW9hcnNhbnMxNS5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczE2LmJh"
        "ZHNzbC5jb22CGHdvd21vYXJzYW5zMTcuYmFkc3NsLmNvbYIYd293bW9hcnNhbnMx"
        "OC5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczE5LmJhZHNzbC5jb22CGHdvd21vYXJz"
        "YW5zMjAuYmFkc3NsLmNvbYIYd293bW9hcnNhbnMyMS5iYWRzc2wuY29tghh3b3dt"
        "b2Fyc2FuczIyLmJhZHNzbC5jb22CGHdvd21vYXJzYW5zMjMuYmFkc3NsLmNvbYIY"
        "d293bW9hcnNhbnMyNC5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczI1LmJhZHNzbC5j"
        "b22CGHdvd21vYXJzYW5zMjYuYmFkc3NsLmNvbYIYd293bW9hcnNhbnMyNy5iYWRz"
        "c2wuY29tghh3b3dtb2Fyc2FuczI4LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zMjku"
        "YmFkc3NsLmNvbYIYd293bW9hcnNhbnMzMC5iYWRzc2wuY29tghh3b3dtb2Fyc2Fu"
        "czMxLmJhZHNzbC5jb22CGHdvd21vYXJzYW5zMzIuYmFkc3NsLmNvbYIYd293bW9h"
        "cnNhbnMzMy5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczM0LmJhZHNzbC5jb22CGHdv"
        "d21vYXJzYW5zMzUuYmFkc3NsLmNvbYIYd293bW9hcnNhbnMzNi5iYWRzc2wuY29t"
        "ghh3b3dtb2Fyc2FuczM3LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zMzguYmFkc3Ns"
        "LmNvbYIYd293bW9hcnNhbnMzOS5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczQwLmJh"
        "ZHNzbC5jb22CGHdvd21vYXJzYW5zNDEuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM0"
        "Mi5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczQzLmJhZHNzbC5jb22CGHdvd21vYXJz"
        "YW5zNDQuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM0NS5iYWRzc2wuY29tghh3b3dt"
        "b2Fyc2FuczQ2LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zNDcuYmFkc3NsLmNvbYIY"
        "d293bW9hcnNhbnM0OC5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczQ5LmJhZHNzbC5j"
        "b22CGHdvd21vYXJzYW5zNTAuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM1MS5iYWRz"
        "c2wuY29tghh3b3dtb2Fyc2FuczUyLmJhZHNzbC5jb22CGHdvd21vYXJzYW5zNTMu"
        "YmFkc3NsLmNvbYIYd293bW9hcnNhbnM1NC5iYWRzc2wuY29tghh3b3dtb2Fyc2Fu"
        "czU1LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zNTYuYmFkc3NsLmNvbYIYd293bW9h"
        "cnNhbnM1Ny5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczU4LmJhZHNzbC5jb22CGHdv"
        "d21vYXJzYW5zNTkuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM2MC5iYWRzc2wuY29t"
        "ghh3b3dtb2Fyc2FuczYxLmJhZHNzbC5jb22CGHdvd21vYXJzYW5zNjIuYmFkc3Ns"
        "LmNvbYIYd293bW9hcnNhbnM2My5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczY0LmJh"
        "ZHNzbC5jb22CGHdvd21vYXJzYW5zNjUuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM2"
        "Ni5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczY3LmJhZHNzbC5jb22CGHdvd21vYXJz"
        "YW5zNjguYmFkc3NsLmNvbYIYd293bW9hcnNhbnM2OS5iYWRzc2wuY29tghh3b3dt"
        "b2Fyc2FuczcwLmJhZHNzbC5jb22CGHdvd21vYXJzYW5zNzEuYmFkc3NsLmNvbYIY"
        "d293bW9hcnNhbnM3Mi5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczczLmJhZHNzbC5j"
        "b22CGHdvd21vYXJzYW5zNzQuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM3NS5iYWRz"
        "c2wuY29tghh3b3dtb2Fyc2Fuczc2LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zNzcu"
        "YmFkc3NsLmNvbYIYd293bW9hcnNhbnM3OC5iYWRzc2wuY29tghh3b3dtb2Fyc2Fu"
        "czc5LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zODAuYmFkc3NsLmNvbYIYd293bW9h"
        "cnNhbnM4MS5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczgyLmJhZHNzbC5jb22CGHdv"
        "d21vYXJzYW5zODMuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM4NC5iYWRzc2wuY29t"
        "ghh3b3dtb2Fyc2Fuczg1LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zODYuYmFkc3Ns"
        "LmNvbYIYd293bW9hcnNhbnM4Ny5iYWRzc2wuY29tghh3b3dtb2Fyc2Fuczg4LmJh"
        "ZHNzbC5jb22CGHdvd21vYXJzYW5zODkuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM5"
        "MC5iYWRzc2wuY29tghh3b3dtb2Fyc2FuczkxLmJhZHNzbC5jb22CGHdvd21vYXJz"
        "YW5zOTIuYmFkc3NsLmNvbYIYd293bW9hcnNhbnM5My5iYWRzc2wuY29tghh3b3dt"
        "b2Fyc2Fuczk0LmJhZHNzbC5jb22CGHdvd21vYXJzYW5zOTUuYmFkc3NsLmNvbYIY"
        "d293bW9hcnNhbnM5Ni5iYWRzc2wuY29tghh3b3dtb2Fyc2Fuczk3LmJhZHNzbC5j"
        "b22CGHdvd21vYXJzYW5zOTguYmFkc3NsLmNvbYIYd293bW9hcnNhbnM5OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczEwMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEw"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEwMi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczEwMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEwNC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczEwNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEwNi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczEwNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEwOC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczEwOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czExMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczExMS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczExMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczExMy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczExNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczExNS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczExNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEx"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczExOC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczExOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEyMC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczEyMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEyMi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczEyMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEyNC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczEyNS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czEyNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEyNy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczEyOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEyOS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczEzMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEzMS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczEzMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEz"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEzNC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczEzNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEzNi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczEzNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczEzOC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczEzOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE0MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczE0MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czE0Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE0My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczE0NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE0NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczE0Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE0Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczE0OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE0"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE1MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczE1MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE1Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczE1My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE1NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczE1NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE1Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczE1Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czE1OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE1OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczE2MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE2MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczE2Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE2My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczE2NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE2"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE2Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczE2Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE2OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczE2OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE3MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczE3MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE3Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczE3My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czE3NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE3NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczE3Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE3Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczE3OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE3OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczE4MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE4"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE4Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczE4My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE4NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczE4NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE4Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczE4Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE4OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczE4OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czE5MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE5MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczE5Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE5My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczE5NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE5NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczE5Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE5"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczE5OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczE5OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIwMC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczIwMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIwMi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczIwMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIwNC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczIwNS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czIwNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIwNy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczIwOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIwOS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczIxMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIxMS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczIxMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIx"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIxNC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczIxNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIxNi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczIxNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIxOC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczIxOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIyMC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczIyMS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czIyMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIyMy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczIyNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIyNS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczIyNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIyNy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczIyOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIy"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIzMC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczIzMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIzMi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczIzMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIzNC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczIzNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIzNi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczIzNy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czIzOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczIzOS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczI0MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI0MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczI0Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI0My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczI0NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI0"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI0Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczI0Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI0OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczI0OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI1MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczI1MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI1Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczI1My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czI1NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI1NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczI1Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI1Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczI1OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI1OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczI2MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI2"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI2Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczI2My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI2NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczI2NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI2Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczI2Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI2OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczI2OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czI3MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI3MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczI3Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI3My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczI3NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI3NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczI3Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI3"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI3OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczI3OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI4MC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczI4MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI4Mi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczI4My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI4NC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczI4NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czI4Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI4Ny5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczI4OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI4OS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczI5MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI5MS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczI5Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI5"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI5NC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczI5NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI5Ni5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczI5Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczI5OC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczI5OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMwMC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczMwMS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czMwMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMwMy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczMwNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMwNS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczMwNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMwNy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczMwOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMw"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMxMC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczMxMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMxMi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczMxMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMxNC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczMxNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMxNi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczMxNy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czMxOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMxOS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczMyMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMyMS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczMyMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMyMy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczMyNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMy"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMyNi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczMyNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMyOC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczMyOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMzMC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczMzMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMzMi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczMzMy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czMzNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMzNS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczMzNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMzNy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczMzOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczMzOS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczM0MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM0"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM0Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczM0My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM0NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczM0NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM0Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczM0Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM0OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczM0OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czM1MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM1MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczM1Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM1My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczM1NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM1NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczM1Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM1"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM1OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczM1OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM2MC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczM2MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM2Mi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczM2My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM2NC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczM2NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czM2Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM2Ny5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczM2OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM2OS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczM3MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM3MS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczM3Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM3"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM3NC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczM3NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM3Ni5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczM3Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM3OC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczM3OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM4MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczM4MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czM4Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM4My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczM4NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM4NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczM4Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM4Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczM4OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM4"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM5MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczM5MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM5Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczM5My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM5NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczM5NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM5Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczM5Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czM5OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczM5OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQwMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQwMS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQwMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQwMy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczQwNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQw"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQwNi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczQwNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQwOC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczQwOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQxMC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczQxMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQxMi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczQxMy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czQxNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQxNS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQxNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQxNy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQxOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQxOS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczQyMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQy"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQyMi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczQyMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQyNC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczQyNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQyNi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczQyNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQyOC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczQyOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czQzMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQzMS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQzMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQzMy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQzNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQzNS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczQzNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQz"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQzOC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczQzOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ0MC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczQ0MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ0Mi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczQ0My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ0NC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczQ0NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czQ0Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ0Ny5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQ0OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ0OS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQ1MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ1MS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczQ1Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ1"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ1NC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczQ1NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ1Ni5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczQ1Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ1OC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczQ1OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ2MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczQ2MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czQ2Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ2My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQ2NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ2NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQ2Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ2Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczQ2OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ2"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ3MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczQ3MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ3Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczQ3My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ3NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczQ3NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ3Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczQ3Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czQ3OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ3OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQ4MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ4MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQ4Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ4My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczQ4NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ4"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ4Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczQ4Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ4OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczQ4OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ5MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczQ5MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ5Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczQ5My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czQ5NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ5NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczQ5Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ5Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczQ5OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczQ5OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczUwMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUw"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUwMi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczUwMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUwNC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczUwNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUwNi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczUwNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUwOC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczUwOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czUxMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUxMS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczUxMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUxMy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczUxNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUxNS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczUxNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUx"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUxOC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczUxOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUyMC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczUyMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUyMi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczUyMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUyNC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczUyNS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czUyNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUyNy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczUyOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUyOS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczUzMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUzMS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczUzMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUz"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUzNC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczUzNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUzNi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczUzNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczUzOC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczUzOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU0MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczU0MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czU0Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU0My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczU0NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU0NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczU0Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU0Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczU0OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU0"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU1MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczU1MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU1Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczU1My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU1NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczU1NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU1Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczU1Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czU1OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU1OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczU2MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU2MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczU2Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU2My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczU2NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU2"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU2Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczU2Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU2OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczU2OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU3MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczU3MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU3Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczU3My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czU3NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU3NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczU3Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU3Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczU3OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU3OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczU4MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU4"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU4Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczU4My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU4NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczU4NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU4Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczU4Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU4OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczU4OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czU5MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU5MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczU5Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU5My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczU5NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU5NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczU5Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU5"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczU5OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczU5OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYwMC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczYwMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYwMi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczYwMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYwNC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczYwNS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czYwNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYwNy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczYwOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYwOS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczYxMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYxMS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczYxMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYx"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYxNC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczYxNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYxNi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczYxNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYxOC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczYxOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYyMC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczYyMS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czYyMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYyMy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczYyNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYyNS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczYyNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYyNy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczYyOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYy"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYzMC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczYzMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYzMi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczYzMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYzNC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczYzNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYzNi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczYzNy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czYzOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczYzOS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczY0MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY0MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczY0Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY0My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczY0NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY0"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY0Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczY0Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY0OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczY0OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY1MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczY1MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY1Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczY1My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czY1NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY1NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczY1Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY1Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczY1OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY1OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczY2MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY2"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY2Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczY2My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY2NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczY2NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY2Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczY2Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY2OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczY2OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czY3MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY3MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczY3Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY3My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczY3NC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY3NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczY3Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY3"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY3OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczY3OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY4MC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczY4MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY4Mi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczY4My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY4NC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczY4NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czY4Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY4Ny5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczY4OC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY4OS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczY5MC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY5MS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczY5Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY5"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY5NC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczY5NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY5Ni5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczY5Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczY5OC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczY5OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcwMC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczcwMS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czcwMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcwMy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczcwNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcwNS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczcwNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcwNy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczcwOC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczcw"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcxMC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczcxMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcxMi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczcxMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcxNC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczcxNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcxNi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczcxNy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czcxOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcxOS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczcyMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcyMS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczcyMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcyMy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczcyNC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczcy"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcyNi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczcyNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczcyOC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczcyOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczczMC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczczMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczczMi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczczMy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czczNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczczNS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczczNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczczNy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczczOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczczOS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczc0MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc0"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc0Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczc0My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc0NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczc0NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc0Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczc0Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc0OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczc0OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czc1MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc1MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczc1Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc1My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczc1NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc1NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczc1Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc1"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc1OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczc1OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc2MC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczc2MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc2Mi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczc2My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc2NC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczc2NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czc2Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc2Ny5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczc2OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc2OS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczc3MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc3MS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczc3Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc3"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc3NC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczc3NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc3Ni5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczc3Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc3OC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczc3OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc4MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczc4MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czc4Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc4My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczc4NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc4NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczc4Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc4Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczc4OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc4"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc5MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczc5MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc5Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczc5My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc5NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczc5NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc5Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczc5Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czc5OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczc5OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczgwMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgwMS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczgwMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgwMy5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczgwNC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczgw"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgwNi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczgwNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgwOC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczgwOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgxMC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczgxMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgxMi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczgxMy5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czgxNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgxNS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczgxNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgxNy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczgxOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgxOS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczgyMC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczgy"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgyMi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczgyMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgyNC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczgyNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgyNi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczgyNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgyOC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczgyOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czgzMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgzMS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczgzMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgzMy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczgzNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgzNS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczgzNi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczgz"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczgzOC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczgzOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg0MC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczg0MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg0Mi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczg0My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg0NC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczg0NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czg0Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg0Ny5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczg0OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg0OS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczg1MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg1MS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczg1Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg1"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg1NC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczg1NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg1Ni5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczg1Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg1OC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczg1OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg2MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczg2MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czg2Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg2My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczg2NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg2NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczg2Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg2Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczg2OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg2"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg3MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczg3MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg3Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczg3My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg3NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczg3NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg3Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczg3Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czg3OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg3OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczg4MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg4MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczg4Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg4My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczg4NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg4"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg4Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczg4Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg4OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczg4OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg5MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczg5MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg5Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczg5My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czg5NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg5NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczg5Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg5Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczg5OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczg5OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczkwMC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczkw"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkwMi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczkwMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkwNC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczkwNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkwNi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczkwNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkwOC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczkwOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czkxMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkxMS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczkxMi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkxMy5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczkxNC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkxNS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczkxNi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczkx"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkxOC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczkxOS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkyMC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczkyMS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkyMi5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczkyMy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkyNC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2FuczkyNS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czkyNi5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkyNy5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2FuczkyOC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkyOS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2FuczkzMC5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkzMS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2FuczkzMi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczkz"
        "My5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkzNC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2FuczkzNS5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkzNi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2FuczkzNy5iYWRzc2wuY29tghl3b3dtb2Fyc2FuczkzOC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2FuczkzOS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk0MC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczk0MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czk0Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk0My5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczk0NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk0NS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczk0Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk0Ny5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczk0OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk0"
        "OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk1MC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczk1MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk1Mi5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczk1My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk1NC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczk1NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk1Ni5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczk1Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czk1OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk1OS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczk2MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk2MS5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczk2Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk2My5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczk2NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk2"
        "NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk2Ni5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczk2Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk2OC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczk2OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk3MC5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczk3MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk3Mi5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczk3My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czk3NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk3NS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczk3Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk3Ny5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczk3OC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk3OS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczk4MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk4"
        "MS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk4Mi5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczk4My5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk4NC5iYWRzc2wuY29tghl3"
        "b3dtb2Fyc2Fuczk4NS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk4Ni5iYWRzc2wu"
        "Y29tghl3b3dtb2Fyc2Fuczk4Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk4OC5i"
        "YWRzc2wuY29tghl3b3dtb2Fyc2Fuczk4OS5iYWRzc2wuY29tghl3b3dtb2Fyc2Fu"
        "czk5MC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk5MS5iYWRzc2wuY29tghl3b3dt"
        "b2Fyc2Fuczk5Mi5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk5My5iYWRzc2wuY29t"
        "ghl3b3dtb2Fyc2Fuczk5NC5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk5NS5iYWRz"
        "c2wuY29tghl3b3dtb2Fyc2Fuczk5Ni5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk5"
        "Ny5iYWRzc2wuY29tghl3b3dtb2Fyc2Fuczk5OC5iYWRzc2wuY29tghl3b3dtb2Fy"
        "c2Fuczk5OS5iYWRzc2wuY29tghp3b3dtb2Fyc2FuczEwMDAuYmFkc3NsLmNvbTAO"
        "BgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMGsG"
        "A1UdHwRkMGIwL6AtoCuGKWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9zc2NhLXNo"
        "YTItZzYuY3JsMC+gLaArhilodHRwOi8vY3JsNC5kaWdpY2VydC5jb20vc3NjYS1z"
        "aGEyLWc2LmNybDBMBgNVHSAERTBDMDcGCWCGSAGG/WwBATAqMCgGCCsGAQUFBwIB"
        "FhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMAgGBmeBDAECAzB8BggrBgEF"
        "BQcBAQRwMG4wJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBG"
        "BggrBgEFBQcwAoY6aHR0cDovL2NhY2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0"
        "U0hBMlNlY3VyZVNlcnZlckNBLmNydDAMBgNVHRMBAf8EAjAAMIIBfQYKKwYBBAHW"
        "eQIEAgSCAW0EggFpAWcAdQC72d+8H4pxtZOUI5eqkntHOFeVCqtS6BqQlmQ2jh7R"
        "hQAAAW2eIkHzAAAEAwBGMEQCIEpoTEi0RcnWKIdDnAYAkfLKHKGiJQhh2BlG7/Pn"
        "g2dKAiAXCXWKhkTf4jqhLUSGtdCvHIC6tOgzT6cXsj7J+so5LgB3AESUZS6w7s6v"
        "xEAH2Kj+KMDa5oK+2MsxtT/TM5a1toGoAAABbZ4iQUEAAAQDAEgwRgIhANsSrrHN"
        "JLBsvN+lnXhSixFDMHLsWwRPXgXBPa/z4R+QAiEAjqTllHw4v33bdAMKhii3HLeA"
        "YEcgLjmGfd4DDne5UWwAdQBWFAaaL9fC7NP14b1Esj7HRna5vJkRXMDvlJhV1onQ"
        "3QAAAW2eIkOsAAAEAwBGMEQCIFw2+me3hlB9WuBMTv1ZVHxxMTCjF0GGn67Gko6i"
        "rNzpAiBaAtLaBUfj17ETVlohrnzXPQDrMgN6jxMysIYxSUr63zANBgkqhkiG9w0B"
        "AQsFAAOCAQEAO6bVckVqsZ1oUeTW4mtmvgxcSknUZN5qMEB4dwQziqJqL5QhOQb6"
        "Q7/MEKUtb+VSj5Q3ST76X5rfTV2Wa5vSPzSwKnJm98le+Xxj/G47zKY+UHPm9LdP"
        "XjPOtsSIf3iPBXnOVtZ3062at2j8GohMlN0j+vxEePNla0KdSchVh7V/Pkccate/"
        "RfrkwcLYkvZ+iU0NyHdhICJefIFNFqdp8hDFF8aiCA1l+6j+sUgb7sTKfUGN6oR0"
        "UJmoDeG0W/71+faD2f1sqggCVVjKYWCLg/Bmen2UAqHWmN3G7vF3y+rKp+fFk8ac"
        "MqdvxTy5qmexezF83HTozIWQg1hwhfoaSg=="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert Global Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
        "QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT"
        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg"
        "U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB"
        "ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83"
        "nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd"
        "KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f"
        "/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX"
        "kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0"
        "/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C"
        "AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY"
        "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6"
        "Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1"
        "oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD"
        "QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v"
        "d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh"
        "xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB"
        "CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl"
        "5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA"
        "8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC"
        "2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit"
        "c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0"
        "j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "1000-sans.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, _10000SANs)
{
    runTest({ "10000-sans.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, ECC256)
{
    runTest({ "ecc256.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, ECC384)
{
    runTest({ "ecc384.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, RSA2048)
{
    runTest({ "rsa2048.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, RSA4096)
{
    runTest({ "rsa4096.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, RSA8192)
{
    runTest({ "rsa8192.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, ExtendedValidation)
{
    Vector<String> certs {
        // 0 s:businessCategory = Private Organization, jurisdictionC = US, jurisdictionST = California, serialNumber = C2543436, C = US, ST = California, L = Mountain View, O = Mozilla Foundation, CN = extended-validation.badssl.com
        //   i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert SHA2 Extended Validation Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIHZDCCBkygAwIBAgIQDtsxL6s4mGkViYnesbc/1zANBgkqhkiG9w0BAQsFADB1"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMTQwMgYDVQQDEytEaWdpQ2VydCBTSEEyIEV4dGVuZGVk"
        "IFZhbGlkYXRpb24gU2VydmVyIENBMB4XDTIwMDYyMzAwMDAwMFoXDTIyMDgxMDEy"
        "MDAwMFowgeQxHTAbBgNVBA8MFFByaXZhdGUgT3JnYW5pemF0aW9uMRMwEQYLKwYB"
        "BAGCNzwCAQMTAlVTMRswGQYLKwYBBAGCNzwCAQITCkNhbGlmb3JuaWExETAPBgNV"
        "BAUTCEMyNTQzNDM2MQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEW"
        "MBQGA1UEBxMNTW91bnRhaW4gVmlldzEbMBkGA1UEChMSTW96aWxsYSBGb3VuZGF0"
        "aW9uMScwJQYDVQQDEx5leHRlbmRlZC12YWxpZGF0aW9uLmJhZHNzbC5jb20wggEi"
        "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCBOz4jO4EwrPYUNVwWMyTGOtc"
        "qGhJsCK1+ZWesSssdj5swEtgTEzqsrTAD4C2sPlyyYYC+VxBXRMrf3HES7zplC5Q"
        "N6ZnHGGM9kFCxUbTFocnn3TrCp0RUiYhc2yETHlV5NFr6AY9SBVSrbMo26r/bv9g"
        "lUp3aznxJNExtt1NwMT8U7ltQq21fP6u9RXSM0jnInHHwhR6bCjqN0rf6my1crR+"
        "WqIW3GmxV0TbChKr3sMPR3RcQSLhmvkbk+atIgYpLrG6SRwMJ56j+4v3QHIArJII"
        "2YxXhFOBBcvm/mtUmEAnhccQu3Nw72kYQQdFVXz5ZD89LMOpfOuTGkyG0cqFAgMB"
        "AAGjggN+MIIDejAfBgNVHSMEGDAWgBQ901Cl1qCt7vNKYApl0yHU+PjWDzAdBgNV"
        "HQ4EFgQUne7Be4ELOkdpcRh9ETeTvKUbP/swKQYDVR0RBCIwIIIeZXh0ZW5kZWQt"
        "dmFsaWRhdGlvbi5iYWRzc2wuY29tMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUEFjAU"
        "BggrBgEFBQcDAQYIKwYBBQUHAwIwdQYDVR0fBG4wbDA0oDKgMIYuaHR0cDovL2Ny"
        "bDMuZGlnaWNlcnQuY29tL3NoYTItZXYtc2VydmVyLWcyLmNybDA0oDKgMIYuaHR0"
        "cDovL2NybDQuZGlnaWNlcnQuY29tL3NoYTItZXYtc2VydmVyLWcyLmNybDBLBgNV"
        "HSAERDBCMDcGCWCGSAGG/WwCATAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5k"
        "aWdpY2VydC5jb20vQ1BTMAcGBWeBDAEBMIGIBggrBgEFBQcBAQR8MHowJAYIKwYB"
        "BQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBSBggrBgEFBQcwAoZGaHR0"
        "cDovL2NhY2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0U0hBMkV4dGVuZGVkVmFs"
        "aWRhdGlvblNlcnZlckNBLmNydDAMBgNVHRMBAf8EAjAAMIIBfwYKKwYBBAHWeQIE"
        "AgSCAW8EggFrAWkAdgApeb7wnjk5IfBWc59jpXflvld9nGAK+PlNXSZcJV3HhAAA"
        "AXLhwe8uAAAEAwBHMEUCIQC5/b5wmGbMOkgH/GupRPFXZ29CaGG8JQMFkjzgBz8n"
        "owIgZQwjhH6rH8lbUX9y3+DLPyUJMA6JXy+18kKQ90JzanIAdwAiRUUHWVUkVpY/"
        "oS/x922G4CMmY63AS39dxoNcbuIPAgAAAXLhwe84AAAEAwBIMEYCIQCI7jirWHoe"
        "G5VW0FDM7MkB2pkUyi2RzM9JDFZ5HXfGJwIhAMWSFJKM57x+bFVfOJkqz3V0vDI/"
        "nywkI96DpHE7tIDdAHYAQcjKsd8iRkoQxqE6CUKHXk4xixsD6+tLx2jwkGKWBvYA"
        "AAFy4cHu+gAABAMARzBFAiASe/ZlNY2nqmcLX6hnjXu7exSER/BmhAVKHexAeGwU"
        "dgIhAJunm2S4Hyz/ofuz4Cs98PknztPlRY3gSxO+ay8lr7XkMA0GCSqGSIb3DQEB"
        "CwUAA4IBAQB0ZpWayltbvblCxkb/KI/UptbKSPex2C8HosV0cXZLdzkAa9UA9Vdg"
        "IYNfkqVUpZH6Z3b7jtyZIUE7Thtcmglmm/OcPeLYOmO6L27T3igni2+b5mlj7L00"
        "PjWsRforHnD7B+q8KnIpdLs4pJc/0hHK2yn11utAOgn+jnBXs3xoRxKYC+nXWM3C"
        "Syhq4B+z/4clh3Mq+Jgse9h50uRf9bmn+n/TxCcfeiDdgY5Z2KNy+nPrP78Jhpl9"
        "f8N6Kv+K8Mm398q8iHyM14V6o0VdrQUTr8ZmEa/KmRAL+eMRzbEZg+YlIyn9qQAy"
        "A5GhqEwE29Z5Knslx7CvNEO9xV3CByfS"
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        // 1 s:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert SHA2 Extended Validation Server CA
        // i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert High Assurance EV Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIEtjCCA56gAwIBAgIQDHmpRLCMEZUgkmFf4msdgzANBgkqhkiG9w0BAQsFADBs"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j"
        "ZSBFViBSb290IENBMB4XDTEzMTAyMjEyMDAwMFoXDTI4MTAyMjEyMDAwMFowdTEL"
        "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3"
        "LmRpZ2ljZXJ0LmNvbTE0MDIGA1UEAxMrRGlnaUNlcnQgU0hBMiBFeHRlbmRlZCBW"
        "YWxpZGF0aW9uIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC"
        "ggEBANdTpARR+JmmFkhLZyeqk0nQOe0MsLAAh/FnKIaFjI5j2ryxQDji0/XspQUY"
        "uD0+xZkXMuwYjPrxDKZkIYXLBxA0sFKIKx9om9KxjxKws9LniB8f7zh3VFNfgHk/"
        "LhqqqB5LKw2rt2O5Nbd9FLxZS99RStKh4gzikIKHaq7q12TWmFXo/a8aUGxUvBHy"
        "/Urynbt/DvTVvo4WiRJV2MBxNO723C3sxIclho3YIeSwTQyJ3DkmF93215SF2AQh"
        "cJ1vb/9cuhnhRctWVyh+HA1BV6q3uCe7seT6Ku8hI3UarS2bhjWMnHe1c63YlC3k"
        "8wyd7sFOYn4XwHGeLN7x+RAoGTMCAwEAAaOCAUkwggFFMBIGA1UdEwEB/wQIMAYB"
        "Af8CAQAwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEF"
        "BQcDAjA0BggrBgEFBQcBAQQoMCYwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRp"
        "Z2ljZXJ0LmNvbTBLBgNVHR8ERDBCMECgPqA8hjpodHRwOi8vY3JsNC5kaWdpY2Vy"
        "dC5jb20vRGlnaUNlcnRIaWdoQXNzdXJhbmNlRVZSb290Q0EuY3JsMD0GA1UdIAQ2"
        "MDQwMgYEVR0gADAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5j"
        "b20vQ1BTMB0GA1UdDgQWBBQ901Cl1qCt7vNKYApl0yHU+PjWDzAfBgNVHSMEGDAW"
        "gBSxPsNpA/i/RwHUmCYaCALvY2QrwzANBgkqhkiG9w0BAQsFAAOCAQEAnbbQkIbh"
        "hgLtxaDwNBx0wY12zIYKqPBKikLWP8ipTa18CK3mtlC4ohpNiAexKSHc59rGPCHg"
        "4xFJcKx6HQGkyhE6V6t9VypAdP3THYUYUN9XR3WhfVUgLkc3UHKMf4Ib0mKPLQNa"
        "2sPIoc4sUqIAY+tzunHISScjl2SFnjgOrWNoPLpSgVh5oywM395t6zHyuqB8bPEs"
        "1OG9d4Q3A84ytciagRpKkk47RpqF/oOi+Z6Mo8wNXrM9zwR4jxQUezKcxwCmXMS1"
        "oVWNWlZopCJwqjyBcdmdqEU79OX2olHdx3ti6G8MdOu42vi/hw15UJGQmxg7kVkn"
        "8TUoE6smftX3eg=="
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "extended-validation.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, ClientCertMissing)
{
    runTest({ "client-cert-missing.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, MixedScript)
{
    runTest({ "mixed-script.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, Very)
{
    runTest({ "very.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, Mixed)
{
    runTest({ "mixed.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, MixedFavicon)
{
    runTest({ "mixed-favicon.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, MixedForm)
{
    runTest({ "mixed-form.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, HTTP)
{
    runTest({ "http.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, HTTPTextarea)
{
    runTest({ "http-textarea.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, HTTPPassword)
{
    runTest({ "http-password.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, HTTPLogin)
{
    runTest({ "http-login.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, HTTPDynamicLogin)
{
    runTest({ "http-dynamic-login.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, HTTPCreditCard)
{
    runTest({ "http-credit-card.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, CBC)
{
    runTest({ "cbc.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, RC4MD5)
{
    runTest({ "rc4-md5.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, RC4)
{
    runTest({ "rc4.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, 3DES)
{
    runTest({ "3des.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, _NULL)
{
    runTest({ "null.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, MozillaOld)
{
    runTest({ "mozilla-old.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, MozillaIntermediate)
{
    runTest({ "mozilla-intermediate.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, MozillaModern)
{
    runTest({ "mozilla-modern.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, DH480)
{
    runTest({ "dh480.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, DH512)
{
    runTest({ "dh512.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, DH1024)
{
    runTest({ "dh1024.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, DH2048)
{
    runTest({ "dh2048.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, DHSmallSubgroup)
{
    runTest({ "dh-small-subgroup.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, DHComposite)
{
    runTest({ "dh-composite.badssl.com"_s, State::EndMode::Fail });
}

TEST(BadSsl, StaticRSA)
{
    runTest({ "static-rsa.badssl.com"_s, State::EndMode::Finish });
}

#if ENABLE_TLS_V1_TEST
TEST(BadSsl, TLSV10)
{
    runTest({ "tls-v1-0.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, TLSV11)
{
    runTest({ "tls-v1-1.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, TLSV12)
{
    runTest({ "tls-v1-2.badssl.com"_s, State::EndMode::Finish });
}
#endif

TEST(BadSsl, NoSct)
{
    Vector<String> certs {
        //  0 s:CN = no-sct.badssl.com
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = RapidSSL TLS RSA CA G1
        "-----BEGIN CERTIFICATE-----"
        "MIIEtzCCA5+gAwIBAgIQDdhV+QIZmPTi+KPGwUPtVDANBgkqhkiG9w0BAQsFADBg"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMR8wHQYDVQQDExZSYXBpZFNTTCBUTFMgUlNBIENBIEcx"
        "MB4XDTIzMTAyNzAwMDAwMFoXDTI0MTEyNjIzNTk1OVowHDEaMBgGA1UEAxMRbm8t"
        "c2N0LmJhZHNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDC"
        "BOz4jO4EwrPYUNVwWMyTGOtcqGhJsCK1+ZWesSssdj5swEtgTEzqsrTAD4C2sPly"
        "yYYC+VxBXRMrf3HES7zplC5QN6ZnHGGM9kFCxUbTFocnn3TrCp0RUiYhc2yETHlV"
        "5NFr6AY9SBVSrbMo26r/bv9glUp3aznxJNExtt1NwMT8U7ltQq21fP6u9RXSM0jn"
        "InHHwhR6bCjqN0rf6my1crR+WqIW3GmxV0TbChKr3sMPR3RcQSLhmvkbk+atIgYp"
        "LrG6SRwMJ56j+4v3QHIArJII2YxXhFOBBcvm/mtUmEAnhccQu3Nw72kYQQdFVXz5"
        "ZD89LMOpfOuTGkyG0cqFAgMBAAGjggGvMIIBqzAfBgNVHSMEGDAWgBQM22yCSQ9K"
        "Zwq4FO56xEhSiOtWODAdBgNVHQ4EFgQUne7Be4ELOkdpcRh9ETeTvKUbP/swMwYD"
        "VR0RBCwwKoIRbm8tc2N0LmJhZHNzbC5jb22CFXd3dy5uby1zY3QuYmFkc3NsLmNv"
        "bTA+BgNVHSAENzA1MDMGBmeBDAECATApMCcGCCsGAQUFBwIBFhtodHRwOi8vd3d3"
        "LmRpZ2ljZXJ0LmNvbS9DUFMwDgYDVR0PAQH/BAQDAgWgMB0GA1UdJQQWMBQGCCsG"
        "AQUFBwMBBggrBgEFBQcDAjA/BgNVHR8EODA2MDSgMqAwhi5odHRwOi8vY2RwLnJh"
        "cGlkc3NsLmNvbS9SYXBpZFNTTFRMU1JTQUNBRzEuY3JsMHYGCCsGAQUFBwEBBGow"
        "aDAmBggrBgEFBQcwAYYaaHR0cDovL3N0YXR1cy5yYXBpZHNzbC5jb20wPgYIKwYB"
        "BQUHMAKGMmh0dHA6Ly9jYWNlcnRzLnJhcGlkc3NsLmNvbS9SYXBpZFNTTFRMU1JT"
        "QUNBRzEuY3J0MAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQELBQADggEBAF37U+gm"
        "kVPuzCEPXJvb+vtsqPrZ5WUEaFgPgO+qAvCwmINWslnafZspOkOFr9ExXah3Rp6d"
        "fLIdBM9kEtLcqOjlRDlzNqfMtr9DE8UeYObrNfeQ321wnG2duejtT8Ctg9f3G39g"
        "V5twCaKNFYZ+Y45TGL9myzfIrkyGU170OYjhGQiyJhAl0amNbBE3lcEiJxVeTpjv"
        "1LSgrBm9G01sybwvnqRfJTbRV+pw+Cli3+elB5zO4S9SwhP+lJ/7LPsahyCWMlpE"
        "/GjoBHkdMrShNryDgPKVfCshvMJYlgKBdkvQwnYj7FfHKcmhNcS1ElM0olI+cQFf"
        "6R2ZngGyM7FrVmo="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "no-sct.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, WTFMove(certs) });
}

TEST(BadSsl, HSTS)
{
    runTest({ "hsts.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, Upgrade)
{
    runTest({ "upgrade.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, PreloadedHSTS)
{
    runTest({ "preloaded-hsts.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, SubdomainPreloadedHSTS)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL Fallback. Unknown subdomain or no SNI., CN = badssl-fallback-unknown-subdomain-or-no-sni
        //    i:C = US, ST = California, L = San Francisco, O = BadSSL, CN = BadSSL Intermediate Certificate Authority
        "-----BEGIN CERTIFICATE-----"
        "MIIE8DCCAtigAwIBAgIJAM28Wkrsl2exMA0GCSqGSIb3DQEBCwUAMH8xCzAJBgNV"
        "BAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNp"
        "c2NvMQ8wDQYDVQQKDAZCYWRTU0wxMjAwBgNVBAMMKUJhZFNTTCBJbnRlcm1lZGlh"
        "dGUgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MB4XDTE2MDgwODIxMTcwNVoXDTE4MDgw"
        "ODIxMTcwNVowgagxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYw"
        "FAYDVQQHDA1TYW4gRnJhbmNpc2NvMTYwNAYDVQQKDC1CYWRTU0wgRmFsbGJhY2su"
        "IFVua25vd24gc3ViZG9tYWluIG9yIG5vIFNOSS4xNDAyBgNVBAMMK2JhZHNzbC1m"
        "YWxsYmFjay11bmtub3duLXN1YmRvbWFpbi1vci1uby1zbmkwggEiMA0GCSqGSIb3"
        "DQEBAQUAA4IBDwAwggEKAoIBAQDCBOz4jO4EwrPYUNVwWMyTGOtcqGhJsCK1+ZWe"
        "sSssdj5swEtgTEzqsrTAD4C2sPlyyYYC+VxBXRMrf3HES7zplC5QN6ZnHGGM9kFC"
        "xUbTFocnn3TrCp0RUiYhc2yETHlV5NFr6AY9SBVSrbMo26r/bv9glUp3aznxJNEx"
        "tt1NwMT8U7ltQq21fP6u9RXSM0jnInHHwhR6bCjqN0rf6my1crR+WqIW3GmxV0Tb"
        "ChKr3sMPR3RcQSLhmvkbk+atIgYpLrG6SRwMJ56j+4v3QHIArJII2YxXhFOBBcvm"
        "/mtUmEAnhccQu3Nw72kYQQdFVXz5ZD89LMOpfOuTGkyG0cqFAgMBAAGjRTBDMAkG"
        "A1UdEwQCMAAwNgYDVR0RBC8wLYIrYmFkc3NsLWZhbGxiYWNrLXVua25vd24tc3Vi"
        "ZG9tYWluLW9yLW5vLXNuaTANBgkqhkiG9w0BAQsFAAOCAgEAsuFs0K86D2IB20nB"
        "QNb+4vs2Z6kECmVUuD0vEUBR/dovFE4PfzTr6uUwRoRdjToewx9VCwvTL7toq3dd"
        "oOwHakRjoxvq+lKvPq+0FMTlKYRjOL6Cq3wZNcsyiTYr7odyKbZs383rEBbcNu0N"
        "c666/ozs4y4W7ufeMFrKak9UenrrPlUe0nrEHV3IMSF32iV85nXm95f7aLFvM6Lm"
        "EzAGgWopuRqD+J0QEt3WNODWqBSZ9EYyx9l2l+KI1QcMalG20QXuxDNHmTEzMaCj"
        "4Zl8k0szexR8rbcQEgJ9J+izxsecLRVp70siGEYDkhq0DgIDOjmmu8ath4yznX6A"
        "pYEGtYTDUxIvsWxwkraBBJAfVxkp2OSg7DiZEVlMM8QxbSeLCz+63kE/d5iJfqde"
        "cGqX7rKEsVW4VLfHPF8sfCyXVi5sWrXrDvJm3zx2b3XToU7EbNONO1C85NsUOWy4"
        "JccoiguV8V6C723IgzkSgJMlpblJ6FVxC6ZX5XJ0ZsMI9TIjibM2L1Z9DkWRCT6D"
        "QjuKbYUeURhScofQBiIx73V7VXnFoc1qHAUd/pGhfkCUnUcuBV1SzCEhjiwjnVKx"
        "HJKvc9OYjJD0ZuvZw9gBrY7qKyBX8g+sglEGFNhruH8/OhqrV8pBXX/EWY0fUZTh"
        "iywmc6GTT7X94Ze2F7iB45jh7WQ="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "subdomain.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, HTTPSEverywhere)
{
    runTest({ "https-everywhere.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, SpoofedFavicon)
{
    runTest({ "spoofed-favicon.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, LockTitle)
{
    runTest({ "lock-title.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, LongExtendedSubdomainNameContainingManyLettersAndDashes)
{
    runTest({ "long-extended-subdomain-name-containing-many-letters-and-dashes.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, LongExtendedSubdomainNameWithoutDashesInorderToTestWordWrapping)
{
    runTest({ "longextendedsubdomainnamewithoutdashesinordertotestwordwrapping.badssl.com"_s, State::EndMode::Finish });
}

TEST(BadSsl, Superfish)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = superfish.badssl.com
        //    i:O = "Superfish, Inc.", L = SF, ST = CA, C = US, CN = "Superfish, Inc."
        "-----BEGIN CERTIFICATE-----"
        "MIIC9TCCAl6gAwIBAgIJAK5EmlK7Klu5MA0GCSqGSIb3DQEBCwUAMFsxGDAWBgNV"
        "BAoTD1N1cGVyZmlzaCwgSW5jLjELMAkGA1UEBxMCU0YxCzAJBgNVBAgTAkNBMQsw"
        "CQYDVQQGEwJVUzEYMBYGA1UEAxMPU3VwZXJmaXNoLCBJbmMuMB4XDTE4MDUxNjE3"
        "MTUyM1oXDTIwMDUxNTE3MTUyM1owajELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNh"
        "bGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDzANBgNVBAoMBkJhZFNT"
        "TDEdMBsGA1UEAwwUc3VwZXJmaXNoLmJhZHNzbC5jb20wggEiMA0GCSqGSIb3DQEB"
        "AQUAA4IBDwAwggEKAoIBAQDCBOz4jO4EwrPYUNVwWMyTGOtcqGhJsCK1+ZWesSss"
        "dj5swEtgTEzqsrTAD4C2sPlyyYYC+VxBXRMrf3HES7zplC5QN6ZnHGGM9kFCxUbT"
        "Focnn3TrCp0RUiYhc2yETHlV5NFr6AY9SBVSrbMo26r/bv9glUp3aznxJNExtt1N"
        "wMT8U7ltQq21fP6u9RXSM0jnInHHwhR6bCjqN0rf6my1crR+WqIW3GmxV0TbChKr"
        "3sMPR3RcQSLhmvkbk+atIgYpLrG6SRwMJ56j+4v3QHIArJII2YxXhFOBBcvm/mtU"
        "mEAnhccQu3Nw72kYQQdFVXz5ZD89LMOpfOuTGkyG0cqFAgMBAAGjLjAsMAkGA1Ud"
        "EwQCMAAwHwYDVR0RBBgwFoIUc3VwZXJmaXNoLmJhZHNzbC5jb20wDQYJKoZIhvcN"
        "AQELBQADgYEAKgHH4VD3jfwzxvtWTmIA1nwK+Fjqe9VFXyDwXiBnhqDwJp9J+/2y"
        "r7jbXfEKf7WBS6OmnU+HTjxUCFx2ZnA4r7dU5nIsNadKEDVHDOvYEJ6mXHPkrvlt"
        "k79iHC0DJiJX36BTXcU649wKEVjgX/kT2yy3YScPdBoN0vtzPN3yFsQ="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "superfish.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, eDellRoot)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = edellroot.badssl.com
        //    i:CN = eDellRoot
        "-----BEGIN CERTIFICATE-----"
        "MIIDLzCCAhegAwIBAgIJAI4bBlMfw/FKMA0GCSqGSIb3DQEBCwUAMBQxEjAQBgNV"
        "BAMTCWVEZWxsUm9vdDAeFw0xODA1MTYwMTU4MzhaFw0yMDA1MTUwMTU4MzhaMGox"
        "CzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4g"
        "RnJhbmNpc2NvMQ8wDQYDVQQKDAZCYWRTU0wxHTAbBgNVBAMMFGVkZWxscm9vdC5i"
        "YWRzc2wuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwgTs+Izu"
        "BMKz2FDVcFjMkxjrXKhoSbAitfmVnrErLHY+bMBLYExM6rK0wA+AtrD5csmGAvlc"
        "QV0TK39xxEu86ZQuUDemZxxhjPZBQsVG0xaHJ5906wqdEVImIXNshEx5VeTRa+gG"
        "PUgVUq2zKNuq/27/YJVKd2s58STRMbbdTcDE/FO5bUKttXz+rvUV0jNI5yJxx8IU"
        "emwo6jdK3+pstXK0flqiFtxpsVdE2woSq97DD0d0XEEi4Zr5G5PmrSIGKS6xukkc"
        "DCeeo/uL90ByAKySCNmMV4RTgQXL5v5rVJhAJ4XHELtzcO9pGEEHRVV8+WQ/PSzD"
        "qXzrkxpMhtHKhQIDAQABoy4wLDAJBgNVHRMEAjAAMB8GA1UdEQQYMBaCFGVkZWxs"
        "cm9vdC5iYWRzc2wuY29tMA0GCSqGSIb3DQEBCwUAA4IBAQA0A9MUBcCy/eb3PHR0"
        "4MEOIpM6kTBewtsaW9BIyEqo8XICSPZB2lYSNAriBvK9SAlZi9kXXJCJE2zgTUXL"
        "VC93BMdOXe3kQ9s7Fbxj3iVNr+Ztegd5JmHICMBaFUjzbJZr7GXWqZUkWxuWD2hF"
        "w3ze0ej0zxZ6deLVEcI8eKbZCACc2eytRKs/v8Wib7Z22BbRuQlliCkt8TDPnCWG"
        "fZyppETTonYvZwtKf1zZTwhZHHeB6XKy+m6l2BeGGa06NeYm34dwOrRPKXD8KWCi"
        "ElWfJOevG6KLwG4pYNV6x/8NgttGTUIDD3t+78hYeC6IIj7hxsPvR0UZeU9Bgcsw"
        "WbqA"
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "edellroot.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, DSDTestProvider)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = dsdtestprovider.badssl.com
        //    i:OU = DSDTestProvider, O = DSDTestProvider, CN = DSDTestProvider
        "-----BEGIN CERTIFICATE-----"
        "MIIEdTCCAl2gAwIBAgIJANbyMsdDstESMA0GCSqGSIb3DQEBCwUAME4xGDAWBgNV"
        "BAsTD0RTRFRlc3RQcm92aWRlcjEYMBYGA1UEChMPRFNEVGVzdFByb3ZpZGVyMRgw"
        "FgYDVQQDEw9EU0RUZXN0UHJvdmlkZXIwHhcNMTgwNTE2MTcxNTIzWhcNMjAwNTE1"
        "MTcxNTIzWjBwMQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQG"
        "A1UEBwwNU2FuIEZyYW5jaXNjbzEPMA0GA1UECgwGQmFkU1NMMSMwIQYDVQQDDBpk"
        "c2R0ZXN0cHJvdmlkZXIuYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEP"
        "ADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5lZ6xKyx2PmzAS2BM"
        "TOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2QULFRtMWhyefdOsK"
        "nRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk0TG23U3AxPxTuW1C"
        "rbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFXRNsKEqveww9HdFxB"
        "IuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EFy+b+a1SYQCeFxxC7"
        "c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaM0MDIwCQYDVR0TBAIwADAl"
        "BgNVHREEHjAcghpkc2R0ZXN0cHJvdmlkZXIuYmFkc3NsLmNvbTANBgkqhkiG9w0B"
        "AQsFAAOCAgEAfNfnT/CA6YUYU9Z1y14O+qbDwMvo2//70i8OvT+xit4ymv6iSnEx"
        "m8J01YsYI/sS4Jvw8dlRtKr1l0h73jiF3cDGpVP38WEdX3kqM0+iCk4kpqkqJgqg"
        "Glwvgx62Z3drXJnlSzr5wAbfzPgqV4g72cJVvD5ksvw1wq5mMc+ESg9m1sBXx5aY"
        "DxgV9+AIetX38k+rMKDHtFQai2BtGgifzowY9F8sM2Z8DaBAaTU0yjPwSZ4ujUYQ"
        "ahzB9E1k6n1JKeKkXCAzb9Q/96APeNVDcmzoOoADLHVz4PAFXEPuuf0E2yx+uC4f"
        "/IUGQ1orb1IP5sPLGy/y7VqzSiTe5R8oU8d20yszVk98HLwO+++nAXmAXs9RQner"
        "3lWBQlGTKwcg3tuQICVPJZMV+9CCYyXLhXOzTHaAG7exxWtnL0E1SHY36Qq3f6jb"
        "r0luAyTvwFYfWEivzXkzSi/r2fRQwUr3ydrJvXZ5sE7Gzx62oBGENiPjDXlDxCIC"
        "m/+nX/1uaBn1Z09V6tyire34FUS4yqLYx59VWODcFp55DzML8AJRylTSPWb5aVi0"
        "vVnuHb5UoRfL2JOaPPTUEINz5C9szD64AOBjN1fXkq3ZFAvbK6MVrrunOGEmiV8N"
        "IRwKmUHTa2wHbP6eyMblyaQdMCVwJE+0LFKlgR9fEe9lSMw5axceZ/4="
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "dsdtestprovider.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, PreactCli)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = preact-cli.badssl.com
        //    i:O = Acme Co
        "-----BEGIN CERTIFICATE-----"
        "MIIDLzCCAhegAwIBAgIJAPw3wdZsDXZOMA0GCSqGSIb3DQEBCwUAMBIxEDAOBgNV"
        "BAoTB0FjbWUgQ28wHhcNMjMxMDI3MjIwNjM5WhcNMjUxMDI2MjIwNjM5WjBrMQsw"
        "CQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZy"
        "YW5jaXNjbzEPMA0GA1UECgwGQmFkU1NMMR4wHAYDVQQDDBVwcmVhY3QtY2xpLmJh"
        "ZHNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCBOz4jO4E"
        "wrPYUNVwWMyTGOtcqGhJsCK1+ZWesSssdj5swEtgTEzqsrTAD4C2sPlyyYYC+VxB"
        "XRMrf3HES7zplC5QN6ZnHGGM9kFCxUbTFocnn3TrCp0RUiYhc2yETHlV5NFr6AY9"
        "SBVSrbMo26r/bv9glUp3aznxJNExtt1NwMT8U7ltQq21fP6u9RXSM0jnInHHwhR6"
        "bCjqN0rf6my1crR+WqIW3GmxV0TbChKr3sMPR3RcQSLhmvkbk+atIgYpLrG6SRwM"
        "J56j+4v3QHIArJII2YxXhFOBBcvm/mtUmEAnhccQu3Nw72kYQQdFVXz5ZD89LMOp"
        "fOuTGkyG0cqFAgMBAAGjLzAtMAkGA1UdEwQCMAAwIAYDVR0RBBkwF4IVcHJlYWN0"
        "LWNsaS5iYWRzc2wuY29tMA0GCSqGSIb3DQEBCwUAA4IBAQBGaZBrNUMztM6rbvwN"
        "qNGlvFuhkSuXAV73vWsIhq/RVFL/lnoNhYc2jRM+MPgzD/7RhKeK4kem4D2bh5cd"
        "o8k48wEfoRaZyh07Bi6spe0p2Xs2fkjQnaYfLy7feHkvWLbD3Mam171RD4Bxehfs"
        "XvF2uAOWFeh31yLTe7WGvrgUSLoYUcxDanR+r0xtm12iOJytiy+ZawyPkg+WzveC"
        "tknM8hF8bfhLI7U2dn29cNIXJybTBPOkOJEPm4KZTENPFJj3SLUj0gTizwt4jzTv"
        "PMYcFOgknWmQrrvtdQf5suvBIiLCF/bJRXzuk+WVbEuDBaCfbmqbxpGcT9txefgD"
        "/9uC"
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "preact-cli.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, WTFMove(certs) });
}

TEST(BadSsl, WebpackDevServer)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = webpack-dev-server.badssl.com
        //    i:CN = localhost
        "-----BEGIN CERTIFICATE-----"
        "MIIDQTCCAimgAwIBAgIJAN3tzhIJAeAYMA0GCSqGSIb3DQEBCwUAMBQxEjAQBgNV"
        "BAMTCWxvY2FsaG9zdDAeFw0yMzEwMjcyMjA2MzlaFw0yNTEwMjYyMjA2MzlaMHMx"
        "CzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4g"
        "RnJhbmNpc2NvMQ8wDQYDVQQKDAZCYWRTU0wxJjAkBgNVBAMMHXdlYnBhY2stZGV2"
        "LXNlcnZlci5iYWRzc2wuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC"
        "AQEAwgTs+IzuBMKz2FDVcFjMkxjrXKhoSbAitfmVnrErLHY+bMBLYExM6rK0wA+A"
        "trD5csmGAvlcQV0TK39xxEu86ZQuUDemZxxhjPZBQsVG0xaHJ5906wqdEVImIXNs"
        "hEx5VeTRa+gGPUgVUq2zKNuq/27/YJVKd2s58STRMbbdTcDE/FO5bUKttXz+rvUV"
        "0jNI5yJxx8IUemwo6jdK3+pstXK0flqiFtxpsVdE2woSq97DD0d0XEEi4Zr5G5Pm"
        "rSIGKS6xukkcDCeeo/uL90ByAKySCNmMV4RTgQXL5v5rVJhAJ4XHELtzcO9pGEEH"
        "RVV8+WQ/PSzDqXzrkxpMhtHKhQIDAQABozcwNTAJBgNVHRMEAjAAMCgGA1UdEQQh"
        "MB+CHXdlYnBhY2stZGV2LXNlcnZlci5iYWRzc2wuY29tMA0GCSqGSIb3DQEBCwUA"
        "A4IBAQDe40iQ/q2uwcz5kvJA8YymQS8wuuqv5LMMtFP4F0Aq+oYvV6OtwSRCmzIk"
        "zX3lnxmdd7oLnREfr0XIFY4BXk6QMS/3AZT2+ClSWfqlqPfibsw/NOpbV1VVN/bJ"
        "KeD2H08Olg6gb0ICu0YK4U1yJzOAkVm/UYvfeYAmLMi9cD0qFCUJ0iY3jGj+2bXO"
        "ps5xkFquf085MrHYZ3aMGmHG52PGmXsimSMO8SDe+Y5YYKHZeqkWuB1i4E9SEXiY"
        "tlwhLGKBZRnrLTytRbsmmOe/xXSroPJdhKU9Xaby3+j3g6XBH02vetcAz228L+d5"
        "2I3TCsULDB4mp6i9lP3dOY9sbCxr"
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "webpack-dev-server.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, WTFMove(certs) });
}

TEST(BadSsl, CaptivePortal)
{
    Vector<String> certs {
        // 0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron Torres, CN = login.captive-portal.badssl.com
        //   i:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIGxTCCBa2gAwIBAgIQBS2mVcnLx9Xd1XuEEKP0KDANBgkqhkiG9w0BAQsFADBN"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMScwJQYDVQQDEx5E"
        "aWdpQ2VydCBTSEEyIFNlY3VyZSBTZXJ2ZXIgQ0EwHhcNMjAwMTIxMDAwMDAwWhcN"
        "MjIwMTI3MTIwMDAwWjCBgTELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3Ju"
        "aWExFTATBgNVBAcTDFdhbG51dCBDcmVlazEcMBoGA1UEChMTTHVjYXMgR2Fycm9u"
        "IFRvcnJlczEoMCYGA1UEAxMfbG9naW4uY2FwdGl2ZS1wb3J0YWwuYmFkc3NsLmNv"
        "bTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALv3FFdxNV9rr08EJeE4"
        "MdxiA4O1u8u9x9y02l2KLq5DSAh42UOGsAEJfHkR6HEFhsqzz9t/dTTUTMKW+aLD"
        "MWyGFQGPKN2PWt1e70YvgT/WU2rU71FpaAKn4r/VEqEGBHWuHX7TLgNuVWZpSAa+"
        "3sramI5WBPXgtpybhG8GYiG2SGfBe5jL3hm7vz+CA0Uj4/YaIwvBvb+YNzS1LPST"
        "DM79b+yno9mUlcf7P10JwSwFpxSXU1+fvCnNENp+EsuEeTS0m46/PXy/cAUjN/RN"
        "wMB1mcBaGCm4O54+czf2qDUUXcNpmkq+33XCMPj3AAm06VB+SrgIlHNt9+lYo+vy"
        "UNkCAwEAAaOCA2owggNmMB8GA1UdIwQYMBaAFA+AYRyCMWHVLyjnjUY4tCzhxtni"
        "MB0GA1UdDgQWBBQQL+oKtbal3+cztFL0m4CgXYamyzAqBgNVHREEIzAhgh9sb2dp"
        "bi5jYXB0aXZlLXBvcnRhbC5iYWRzc2wuY29tMA4GA1UdDwEB/wQEAwIFoDAdBgNV"
        "HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwawYDVR0fBGQwYjAvoC2gK4YpaHR0"
        "cDovL2NybDMuZGlnaWNlcnQuY29tL3NzY2Etc2hhMi1nNi5jcmwwL6AtoCuGKWh0"
        "dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9zc2NhLXNoYTItZzYuY3JsMEwGA1UdIARF"
        "MEMwNwYJYIZIAYb9bAEBMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2lj"
        "ZXJ0LmNvbS9DUFMwCAYGZ4EMAQIDMHwGCCsGAQUFBwEBBHAwbjAkBggrBgEFBQcw"
        "AYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEYGCCsGAQUFBzAChjpodHRwOi8v"
        "Y2FjZXJ0cy5kaWdpY2VydC5jb20vRGlnaUNlcnRTSEEyU2VjdXJlU2VydmVyQ0Eu"
        "Y3J0MAwGA1UdEwEB/wQCMAAwggGABgorBgEEAdZ5AgQCBIIBcASCAWwBagB3AKS5"
        "CZC0GFgUh7sTosxncAo8NZgE+RvfuON3zQ7IDdwQAAABb8qB/9gAAAQDAEgwRgIh"
        "AKoj6bicVvH9H9LCLU1ym/l2EuyXFxESaFJKsTdmaN7kAiEA+pCrXBXB7IFwbZDw"
        "3EouAoZQnsB+4wGM/xG59kn323sAdwCHdb/nWXz4jEOZX73zbv9WjUdWNv9KtWDB"
        "tOr/XqCDDwAAAW/KggDIAAAEAwBIMEYCIQDyWifbVd34HcYPBCREkCBm4PctqR3g"
        "TWP4CVebm9C64AIhALSP2X6v4zI1ZhJcZJLbmK4RtsXsVAx3uZiW9FKeGjhZAHYA"
        "7ku9t3XOYLrhQmkfq+GeZqMPfl+wctiDAMR7iXqo/csAAAFvyoIAIwAABAMARzBF"
        "AiEAgcEvg5UdGnw5WOAVVGMg1cTeSs5NhQFoOgzHYmCY9JICIDJrg8NvLLqPzB92"
        "NuRtUZ6NTbPyhmIpikymlFgK6JVXMA0GCSqGSIb3DQEBCwUAA4IBAQAjdghSoa/h"
        "0Q2yGvVykOrRP2LANobgQWAnDrCaxV3Iv6azOynb97RRJ8Ha7gK9+Dw5n0iWPhB/"
        "dhcwh8eHfU1Y2rb01U2Wl3oBdDEBINM84XHaOKfzW8L4Hlc+ideTXivBfvhdoD2H"
        "CckkRdTLaGI5zoXt/jSBbwtce31MxJmY+j2Sug5Jdu0lWgBeRyuFgUV7uxX4Me0v"
        "5x9yXSJOWFEZ9vSIQz4XFGMNurDAKgI9T9/JHYxW42sAHfktmRZUARAPkm80L6JB"
        "ABz5eStVKpgvjA7BQwlhG7NxlJi4IrxNrufUvNG6gkH3XH8DVW3aClk0GID2NzsV"
        "6/NGEyttyCAT"
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = DigiCert Inc, CN = DigiCert SHA2 Secure Server CA
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert Global Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
        "QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT"
        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg"
        "U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB"
        "ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83"
        "nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd"
        "KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f"
        "/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX"
        "kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0"
        "/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C"
        "AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY"
        "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6"
        "Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1"
        "oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD"
        "QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v"
        "d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh"
        "xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB"
        "CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl"
        "5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA"
        "8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC"
        "2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit"
        "c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0"
        "j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "captive-portal.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, MitmSoftware)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = San Francisco, O = BadSSL, CN = mitm-software.badssl.com
        //    i:C = US, ST = California, L = San Francisco, O = BadSSL, CN = BadSSL MITM Software Test
        "-----BEGIN CERTIFICATE-----"
        "MIIDkjCCAnqgAwIBAgIJANw2YFzKOBcMMA0GCSqGSIb3DQEBCwUAMG8xCzAJBgNV"
        "BAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRYwFAYDVQQHEw1TYW4gRnJhbmNp"
        "c2NvMQ8wDQYDVQQKEwZCYWRTU0wxIjAgBgNVBAMTGUJhZFNTTCBNSVRNIFNvZnR3"
        "YXJlIFRlc3QwHhcNMjMxMDI3MjIwNjM4WhcNMjUxMDI2MjIwNjM4WjBuMQswCQYD"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j"
        "aXNjbzEPMA0GA1UECgwGQmFkU1NMMSEwHwYDVQQDDBhtaXRtLXNvZnR3YXJlLmJh"
        "ZHNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCBOz4jO4E"
        "wrPYUNVwWMyTGOtcqGhJsCK1+ZWesSssdj5swEtgTEzqsrTAD4C2sPlyyYYC+VxB"
        "XRMrf3HES7zplC5QN6ZnHGGM9kFCxUbTFocnn3TrCp0RUiYhc2yETHlV5NFr6AY9"
        "SBVSrbMo26r/bv9glUp3aznxJNExtt1NwMT8U7ltQq21fP6u9RXSM0jnInHHwhR6"
        "bCjqN0rf6my1crR+WqIW3GmxV0TbChKr3sMPR3RcQSLhmvkbk+atIgYpLrG6SRwM"
        "J56j+4v3QHIArJII2YxXhFOBBcvm/mtUmEAnhccQu3Nw72kYQQdFVXz5ZD89LMOp"
        "fOuTGkyG0cqFAgMBAAGjMjAwMAkGA1UdEwQCMAAwIwYDVR0RBBwwGoIYbWl0bS1z"
        "b2Z0d2FyZS5iYWRzc2wuY29tMA0GCSqGSIb3DQEBCwUAA4IBAQCKqiHG42WcPZ0v"
        "UPvOuT2t8B6Cw0s8PZ6ywsSbvRyXCfDIeHQByOCuKBmnQNowCyRr4ZhqXwImEmeX"
        "ypc8UeYFBnYdNpNEntq58YBcsH/rETXAoFF4dLOrJFcI6Xv57cpbxq2Vf4bdSeb8"
        "g7sgOMiXkSJK1Y5pqLKni62YBzSP3Nv+eWngAz8poYcZRHqqj2aTSsNMDwBNXaNx"
        "vK+/xG6yUFjNjaC/Bgp3ShauutV1ItEyfkd82c8saafhzbPkPb0mlJFoZikNtwrX"
        "yMD+VpHGOuF6KT5PSBirt5ic/ACBxgQFmuO6Wo6MlDNZPs7kyTi6l2d3yyDA/sNE"
        "qpgnTUwq"
        "-----END CERTIFICATE-----"_s
    };

    runTest({ "mitm-software.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, WTFMove(certs) });
}

TEST(BadSsl, SHA1_2016)
{
    Vector<String> certs {
        //  0 s:OU = Domain Control Validated, OU = PositiveSSL Wildcard, CN = *.badssl.com
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = PositiveSSL CA 2
        "-----BEGIN CERTIFICATE-----"
        "MIIE/DCCA+SgAwIBAgIRANtfeAW7mHWw3X4DQI+JuJwwDQYJKoZIhvcNAQEFBQAw"
        "czELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxGTAXBgNV"
        "BAMTEFBvc2l0aXZlU1NMIENBIDIwHhcNMTUwNDA5MDAwMDAwWhcNMTYxMjI5MjM1"
        "OTU5WjBZMSEwHwYDVQQLExhEb21haW4gQ29udHJvbCBWYWxpZGF0ZWQxHTAbBgNV"
        "BAsTFFBvc2l0aXZlU1NMIFdpbGRjYXJkMRUwEwYDVQQDFAwqLmJhZHNzbC5jb20w"
        "ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCBOz4jO4EwrPYUNVwWMyT"
        "GOtcqGhJsCK1+ZWesSssdj5swEtgTEzqsrTAD4C2sPlyyYYC+VxBXRMrf3HES7zp"
        "lC5QN6ZnHGGM9kFCxUbTFocnn3TrCp0RUiYhc2yETHlV5NFr6AY9SBVSrbMo26r/"
        "bv9glUp3aznxJNExtt1NwMT8U7ltQq21fP6u9RXSM0jnInHHwhR6bCjqN0rf6my1"
        "crR+WqIW3GmxV0TbChKr3sMPR3RcQSLhmvkbk+atIgYpLrG6SRwMJ56j+4v3QHIA"
        "rJII2YxXhFOBBcvm/mtUmEAnhccQu3Nw72kYQQdFVXz5ZD89LMOpfOuTGkyG0cqF"
        "AgMBAAGjggGjMIIBnzAfBgNVHSMEGDAWgBSZ5EBfaxRePgXZ3dNjVPxiuPcArDAd"
        "BgNVHQ4EFgQUne7Be4ELOkdpcRh9ETeTvKUbP/swDgYDVR0PAQH/BAQDAgWgMAwG"
        "A1UdEwEB/wQCMAAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMFAGA1Ud"
        "IARJMEcwOwYLKwYBBAGyMQECAgcwLDAqBggrBgEFBQcCARYeaHR0cDovL3d3dy5w"
        "b3NpdGl2ZXNzbC5jb20vQ1BTMAgGBmeBDAECATA7BgNVHR8ENDAyMDCgLqAshipo"
        "dHRwOi8vY3JsLmNvbW9kb2NhLmNvbS9Qb3NpdGl2ZVNTTENBMi5jcmwwbAYIKwYB"
        "BQUHAQEEYDBeMDYGCCsGAQUFBzAChipodHRwOi8vY3J0LmNvbW9kb2NhLmNvbS9Q"
        "b3NpdGl2ZVNTTENBMi5jcnQwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmNvbW9k"
        "b2NhLmNvbTAjBgNVHREEHDAaggwqLmJhZHNzbC5jb22CCmJhZHNzbC5jb20wDQYJ"
        "KoZIhvcNAQEFBQADggEBAKmb3yOAS9AxyXj/mkrb36QpLBYbCx148DeytlBWioks"
        "kE362rgsVenBCgtqXO4/xJOBG3B4Ll5stVRSViSBTinRE8NxwURY5apDiK6hjsEN"
        "gtqn8aEkdKB0Q6ArAWEjOHfjUDujlPUrksnJeqSDxLMisv8Yq5Zgen+gsUxr+/sJ"
        "raK0BUkCgbSQy9T8X/bZb+bLr05iv1jzn1N8SRYOelhnq9nE28onri2Nd3u9BV/N"
        "1NbKpD5w/A8IxY5L6VqaZLIlw8ucijGejEqq3n4hg5r/XzW9vKqnBZq9gn+YXTpc"
        "Vpww1KVuHmbn6G5Uz61CeH5gU4jLFWtbbtdmiOGTwq4="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = PositiveSSL CA 2
        //    i:C = SE, O = AddTrust AB, OU = AddTrust External TTP Network, CN = AddTrust External CA Root
        "-----BEGIN CERTIFICATE-----"
        "MIIE5TCCA82gAwIBAgIQB28SRoFFnCjVSNaXxA4AGzANBgkqhkiG9w0BAQUFADBv"
        "MQswCQYDVQQGEwJTRTEUMBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFk"
        "ZFRydXN0IEV4dGVybmFsIFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBF"
        "eHRlcm5hbCBDQSBSb290MB4XDTEyMDIxNjAwMDAwMFoXDTIwMDUzMDEwNDgzOFow"
        "czELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxGTAXBgNV"
        "BAMTEFBvc2l0aXZlU1NMIENBIDIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK"
        "AoIBAQDo6jnjIqaqucQA0OeqZztDB71Pkuu8vgGjQK3g70QotdA6voBUF4V6a4Rs"
        "NjbloyTi/igBkLzX3Q+5K05IdwVpr95XMLHo+xoD9jxbUx6hAUlocnPWMytDqTcy"
        "Ug+uJ1YxMGCtyb1zLDnukNh1sCUhYHsqfwL9goUfdE+SNHNcHQCgsMDqmOK+ARRY"
        "FygiinddUCXNmmym5QzlqyjDsiCJ8AckHpXCLsDl6ez2PRIHSD3SwyNWQezT3zVL"
        "yOf2hgVSEEOajBd8i6q8eODwRTusgFX+KJPhChFo9FJXb/5IC1tdGmpnc5mCtJ5D"
        "YD7HWyoSbhruyzmuwzWdqLxdsC/DAgMBAAGjggF3MIIBczAfBgNVHSMEGDAWgBSt"
        "vZh6NLQm9/rEJlTvA73gJMtUGjAdBgNVHQ4EFgQUmeRAX2sUXj4F2d3TY1T8Yrj3"
        "AKwwDgYDVR0PAQH/BAQDAgEGMBIGA1UdEwEB/wQIMAYBAf8CAQAwEQYDVR0gBAow"
        "CDAGBgRVHSAAMEQGA1UdHwQ9MDswOaA3oDWGM2h0dHA6Ly9jcmwudXNlcnRydXN0"
        "LmNvbS9BZGRUcnVzdEV4dGVybmFsQ0FSb290LmNybDCBswYIKwYBBQUHAQEEgaYw"
        "gaMwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNvbS9BZGRUcnVz"
        "dEV4dGVybmFsQ0FSb290LnA3YzA5BggrBgEFBQcwAoYtaHR0cDovL2NydC51c2Vy"
        "dHJ1c3QuY29tL0FkZFRydXN0VVROU0dDQ0EuY3J0MCUGCCsGAQUFBzABhhlodHRw"
        "Oi8vb2NzcC51c2VydHJ1c3QuY29tMA0GCSqGSIb3DQEBBQUAA4IBAQCcNuNOrvGK"
        "u2yXjI9LZ9Cf2ISqnyFfNaFbxCtjDei8d12nxDf9Sy2e6B1pocCEzNFti/OBy59L"
        "dLBJKjHoN0DrH9mXoxoR1Sanbg+61b4s/bSRZNy+OxlQDXqV8wQTqbtHD4tc0azC"
        "e3chUN1bq+70ptjUSlNrTa24yOfmUlhNQ0zCoiNPDsAgOa/fT0JbHtMJ9BgJWSrZ"
        "6EoYvzL7+i1ki4fKWyvouAt+vhcSxwOCKa9Yr4WEXT0K3yNRw82vEL+AaXeRCk/l"
        "uuGtm87fM04wO+mPZn+C+mv626PAcwDj1hKvTfIPWhRRH224hoFiB85ccsJP81cq"
        "cdnUl4XmGFO3"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "sha1-2016.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, SHA1_2017)
{
    Vector<String> certs {
        //  0 s:C = US, ST = California, L = Walnut Creek, O = Lucas Garron, CN = *.badssl.com
        //    i:C = US, O = DigiCert Inc, CN = DigiCert Secure Server CA
        "-----BEGIN CERTIFICATE-----"
        "MIIFAjCCA+qgAwIBAgIQC6EoMauKfYZOYVncNOdeDTANBgkqhkiG9w0BAQUFADBI"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMSIwIAYDVQQDExlE"
        "aWdpQ2VydCBTZWN1cmUgU2VydmVyIENBMB4XDTE1MDYwOTAwMDAwMFoXDTE3MDEw"
        "NTEyMDAwMFowZzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExFTAT"
        "BgNVBAcTDFdhbG51dCBDcmVlazEVMBMGA1UEChMMTHVjYXMgR2Fycm9uMRUwEwYD"
        "VQQDDAwqLmJhZHNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB"
        "AQDCBOz4jO4EwrPYUNVwWMyTGOtcqGhJsCK1+ZWesSssdj5swEtgTEzqsrTAD4C2"
        "sPlyyYYC+VxBXRMrf3HES7zplC5QN6ZnHGGM9kFCxUbTFocnn3TrCp0RUiYhc2yE"
        "THlV5NFr6AY9SBVSrbMo26r/bv9glUp3aznxJNExtt1NwMT8U7ltQq21fP6u9RXS"
        "M0jnInHHwhR6bCjqN0rf6my1crR+WqIW3GmxV0TbChKr3sMPR3RcQSLhmvkbk+at"
        "IgYpLrG6SRwMJ56j+4v3QHIArJII2YxXhFOBBcvm/mtUmEAnhccQu3Nw72kYQQdF"
        "VXz5ZD89LMOpfOuTGkyG0cqFAgMBAAGjggHHMIIBwzAfBgNVHSMEGDAWgBSQcds3"
        "63PI79zVHhK2NLorWqCmkjAdBgNVHQ4EFgQUne7Be4ELOkdpcRh9ETeTvKUbP/sw"
        "IwYDVR0RBBwwGoIKYmFkc3NsLmNvbYIMKi5iYWRzc2wuY29tMA4GA1UdDwEB/wQE"
        "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwYQYDVR0fBFowWDAq"
        "oCigJoYkaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL3NzY2EtZzcuY3JsMCqgKKAm"
        "hiRodHRwOi8vY3JsNC5kaWdpY2VydC5jb20vc3NjYS1nNy5jcmwwQgYDVR0gBDsw"
        "OTA3BglghkgBhv1sAQEwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNl"
        "cnQuY29tL0NQUzB4BggrBgEFBQcBAQRsMGowJAYIKwYBBQUHMAGGGGh0dHA6Ly9v"
        "Y3NwLmRpZ2ljZXJ0LmNvbTBCBggrBgEFBQcwAoY2aHR0cDovL2NhY2VydHMuZGln"
        "aWNlcnQuY29tL0RpZ2lDZXJ0U2VjdXJlU2VydmVyQ0EuY3J0MAwGA1UdEwEB/wQC"
        "MAAwDQYJKoZIhvcNAQEFBQADggEBAGNcYNDbTZP5rkHwTr//KhFqXhVclWGVBGrY"
        "DqGigBJKnhU7gHpPqknLhtrYr/Hihzaj5tvKmN9aPFzflRyn6cuy2fXiIFxN9NNt"
        "zkrp4iAyKQwbYAGb6tuE7xu9p8dLIF/ZnRvb64J5Pt/92s/9jFAb3W9xgpIjA2Tw"
        "AFco+osDP7nMkECTu3DIVlmoTWp3lKoV7l4TEMgHxM3H+iQl+9uKHLIuoVK5KD/Y"
        "UCKyBAy0gYO4w0oM+bKsiVzdBOYm+vUJXjTNTOCrAruYY34HV26qYYpT/D6Vd4SM"
        "TUtfbqM4Bg6CY1ehIoeVEnGhhGG68QsHtAEyaBQpyVg6V+ZcIXg="
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = DigiCert Inc, CN = DigiCert Secure Server CA
        //    i:C = US, O = DigiCert Inc, OU = www.digicert.com, CN = DigiCert Global Root CA
        "-----BEGIN CERTIFICATE-----"
        "MIIEjzCCA3egAwIBAgIQBp4dt3/PHfupevXlyaJANzANBgkqhkiG9w0BAQUFADBh"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD"
        "QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaMEgxCzAJBgNVBAYTAlVT"
        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxIjAgBgNVBAMTGURpZ2lDZXJ0IFNlY3Vy"
        "ZSBTZXJ2ZXIgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC7V+Qh"
        "qdWbYDd+jqFhf4HiGsJ1ZNmRUAvkNkQkbjDSm3on+sJqrmpwCTi5IArIZRBKiKwx"
        "8tyS8mOhXYBjWYCSIxzm73ZKUDXJ2HE4ue3w5kKu0zgmeTD5IpTG26Y/QXiQ2N5c"
        "fml9+JAVOtChoL76srIZodgr0c6/a91Jq6OS/rWryME+7gEA2KlEuEJziMNh9atK"
        "gygK0tRJ+mqxzd9XLJTl4sqDX7e6YlwvaKXwwLn9K9HpH9gaYhW9/z2m98vv5ttl"
        "LyU47PvmIGZYljQZ0hXOIdMkzNkUb9j+Vcfnb7YPGoxJvinyulqagSY3JG/XSBJs"
        "Lln1nBi72fZo4t9FAgMBAAGjggFaMIIBVjASBgNVHRMBAf8ECDAGAQH/AgEAMA4G"
        "A1UdDwEB/wQEAwIBhjA0BggrBgEFBQcBAQQoMCYwJAYIKwYBBQUHMAGGGGh0dHA6"
        "Ly9vY3NwLmRpZ2ljZXJ0LmNvbTB7BgNVHR8EdDByMDegNaAzhjFodHRwOi8vY3Js"
        "My5kaWdpY2VydC5jb20vRGlnaUNlcnRHbG9iYWxSb290Q0EuY3JsMDegNaAzhjFo"
        "dHRwOi8vY3JsNC5kaWdpY2VydC5jb20vRGlnaUNlcnRHbG9iYWxSb290Q0EuY3Js"
        "MD0GA1UdIAQ2MDQwMgYEVR0gADAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5k"
        "aWdpY2VydC5jb20vQ1BTMB0GA1UdDgQWBBSQcds363PI79zVHhK2NLorWqCmkjAf"
        "BgNVHSMEGDAWgBQD3lA1VtFMu2bwo+IbG8OXsj3RVTANBgkqhkiG9w0BAQUFAAOC"
        "AQEAMM7RlVEArgYLoQ4CwBestn+PIPZAdXQczHixpE/q9NDEnaLegQcmH0CIUfAf"
        "z7dMQJnQ9DxxmHOIlywZ126Ej6QfnFog41FcsMWemWpPyGn3EP9OrRnZyVizM64M"
        "2ZYpnnGycGOjtpkWQh1l8/egHn3F1GUUsmKE1GxcCAzYbJMrtHZZitF//wPYwl24"
        "LyLWOPD2nGt9RuuZdPfrSg6ppgTre87wXGuYMVqYQOtpxAX0IKjKCDplbDgV9Vws"
        "slXkLGtB8L5cRspKKaBIXiDSRf8F3jSvcEuBOeLKB1d8tjHcISnivpcOd5AUUUDh"
        "v+PMGxmcJcqnBrJT3yOyzxIZow=="
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "sha1-2017.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, SHA1Intermediate)
{
    Vector<String> certs {
        //  0 s:OU = Domain Control Validated, OU = COMODO SSL Wildcard, CN = *.badssl.com
        //    i:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO SSL CA
        "-----BEGIN CERTIFICATE-----"
        "MIIE8TCCA9mgAwIBAgIRAL4AQmnXWHlXEDwE56pO2LIwDQYJKoZIhvcNAQELBQAw"
        "cDELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxFjAUBgNV"
        "BAMTDUNPTU9ETyBTU0wgQ0EwHhcNMTcwNDEzMDAwMDAwWhcNMjAwNTMwMjM1OTU5"
        "WjBYMSEwHwYDVQQLExhEb21haW4gQ29udHJvbCBWYWxpZGF0ZWQxHDAaBgNVBAsT"
        "E0NPTU9ETyBTU0wgV2lsZGNhcmQxFTATBgNVBAMMDCouYmFkc3NsLmNvbTCCASIw"
        "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yo"
        "aEmwIrX5lZ6xKyx2PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3"
        "pmccYYz2QULFRtMWhyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CV"
        "SndrOfEk0TG23U3AxPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5a"
        "ohbcabFXRNsKEqveww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZ"
        "jFeEU4EFy+b+a1SYQCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEA"
        "AaOCAZwwggGYMB8GA1UdIwQYMBaAFBtrvR+KSRiUVDdVtCAX7Te5dxh9MB0GA1Ud"
        "DgQWBBSd7sF7gQs6R2lxGH0RN5O8pRs/+zAOBgNVHQ8BAf8EBAMCBaAwDAYDVR0T"
        "AQH/BAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwTwYDVR0gBEgw"
        "RjA6BgsrBgEEAbIxAQICBzArMCkGCCsGAQUFBwIBFh1odHRwczovL3NlY3VyZS5j"
        "b21vZG8uY29tL0NQUzAIBgZngQwBAgEwOAYDVR0fBDEwLzAtoCugKYYnaHR0cDov"
        "L2NybC5jb21vZG9jYS5jb20vQ09NT0RPU1NMQ0EuY3JsMGkGCCsGAQUFBwEBBF0w"
        "WzAzBggrBgEFBQcwAoYnaHR0cDovL2NydC5jb21vZG9jYS5jb20vQ09NT0RPU1NM"
        "Q0EuY3J0MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wIwYD"
        "VR0RBBwwGoIMKi5iYWRzc2wuY29tggpiYWRzc2wuY29tMA0GCSqGSIb3DQEBCwUA"
        "A4IBAQCjAoXzYKLon9rpcYVKD1Y3zvIZyojAiUgibAi/v3trIBDA92bOCxBNgCyw"
        "yU3yFR8eSriE1lROeZghScU/qMKqJQhNv8jSRKiCaVjX/6XGJeGjJ4vDZgkoFOAt"
        "3BUpzUSqCNZPuHim6YSIWRgcoCgvqzvh9wVh/eRTMGt2naTfy2ieUkYSKleGbE91"
        "DeCKiiAJlimR0MJ5xOznTvCMxvs0ZppG41F+ain6rmsKQaVZfw4IxJW+9KmtNO4g"
        "EJO5rT+lOyz3t3Ij2yblHAwtcdxxwyA9BdvnIxfDcXVtNcqPNfBZRkhct/APO/yS"
        "Ix4MYaiI3P48eZeMnLgiw/MOh2Vi"
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = GB, ST = Greater Manchester, L = Salford, O = COMODO CA Limited, CN = COMODO SSL CA
        //    i:C = SE, O = AddTrust AB, OU = AddTrust External TTP Network, CN = AddTrust External CA Root
        "-----BEGIN CERTIFICATE-----"
        "MIIE4jCCA8qgAwIBAgIQbrrwj3mD+p3hsm+W/G6YvzANBgkqhkiG9w0BAQUFADBv"
        "MQswCQYDVQQGEwJTRTEUMBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFk"
        "ZFRydXN0IEV4dGVybmFsIFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBF"
        "eHRlcm5hbCBDQSBSb290MB4XDTExMDgyMzAwMDAwMFoXDTIwMDUzMDEwNDgzOFow"
        "cDELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G"
        "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxFjAUBgNV"
        "BAMTDUNPTU9ETyBTU0wgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB"
        "AQDUKy4c0qP4f1UUQN73RN2EVfeFe1VmaaflWetlg/TzdrFmw09OmJMJt0Cz0Reg"
        "EgmogOEpY5cCjDGdCgLgWVu77TC1735drwhOjYvCOVYWmHOUeArJpk8ot6g0N9sl"
        "IbE8mfbgEj5z6mQyn0IGPBnYCgR6TFdJK9J3etAAvF76ju7MwuQTbiVf3DykiKPc"
        "Sce8xw/dGcCxcu147ziDCkUXG8l9ne3fqywso3WuW4IdiIONzghlDGYmVwWhDN/m"
        "B4QLhKPIq9WVR7/c3P4d/AKTRAHK5rW3axYwAV3piQmVnvheKVzdx1WM8o4gTkB6"
        "5PVFA7SYK8SAflOHb8LSV7DpAgMBAAGjggF3MIIBczAfBgNVHSMEGDAWgBStvZh6"
        "NLQm9/rEJlTvA73gJMtUGjAdBgNVHQ4EFgQUG2u9H4pJGJRUN1W0IBftN7l3GH0w"
        "DgYDVR0PAQH/BAQDAgEGMBIGA1UdEwEB/wQIMAYBAf8CAQAwEQYDVR0gBAowCDAG"
        "BgRVHSAAMEQGA1UdHwQ9MDswOaA3oDWGM2h0dHA6Ly9jcmwudXNlcnRydXN0LmNv"
        "bS9BZGRUcnVzdEV4dGVybmFsQ0FSb290LmNybDCBswYIKwYBBQUHAQEEgaYwgaMw"
        "PwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNvbS9BZGRUcnVzdEV4"
        "dGVybmFsQ0FSb290LnA3YzA5BggrBgEFBQcwAoYtaHR0cDovL2NydC51c2VydHJ1"
        "c3QuY29tL0FkZFRydXN0VVROU0dDQ0EuY3J0MCUGCCsGAQUFBzABhhlodHRwOi8v"
        "b2NzcC51c2VydHJ1c3QuY29tMA0GCSqGSIb3DQEBBQUAA4IBAQBDJTkjBwSsmV1Z"
        "Zz3mL2F9WlZ7/AaNs0ud+tUFTA1mtb08x6Iqa7XP5rqDPmCQNgzVwu2KldmSQiMc"
        "A3Y+wkjxdXKds4zPs1g0VkkdoS4rPbLoWhBG3mS1Ta5LbvwBtyEQ1ZW36yy+FAbM"
        "QS7kbOJGkP/GKH5z/uUXuoLDEAWBZsKLKDigRD7p5M4zsHz44VOduLTL2sku2ZNw"
        "jnwL43M+mZmP6+ERRDXYYIFiRdTeRVuQLkkbG9ukD4BiIXNp8ePebdhIfFYSJiIR"
        "RwHGXhnCtJWX7mEAVfEEOPyE5ni0DUO+QzPdaNMiWwD7FILoS2J5MM/TlZ+zuYQB"
        "1N3PIxL4"
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "sha1-intermediate.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

TEST(BadSsl, InvalidExpectedSct)
{
    Vector<String> certs {
        //  0 s:CN = invalid-expected-sct.badssl.com
        //    i:C = US, O = GeoTrust Inc., CN = RapidSSL SHA256 CA
        "-----BEGIN CERTIFICATE-----"
        "MIIFGDCCBACgAwIBAgIQGkTSlT8/2bAURo/J1C2/STANBgkqhkiG9w0BAQsFADBC"
        "MQswCQYDVQQGEwJVUzEWMBQGA1UEChMNR2VvVHJ1c3QgSW5jLjEbMBkGA1UEAxMS"
        "UmFwaWRTU0wgU0hBMjU2IENBMB4XDTE2MTExNzAwMDAwMFoXDTE4MTExNzIzNTk1"
        "OVowKjEoMCYGA1UEAwwfaW52YWxpZC1leHBlY3RlZC1zY3QuYmFkc3NsLmNvbTCC"
        "ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY"
        "61yoaEmwIrX5lZ6xKyx2PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmU"
        "LlA3pmccYYz2QULFRtMWhyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u"
        "/2CVSndrOfEk0TG23U3AxPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVy"
        "tH5aohbcabFXRNsKEqveww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCs"
        "kgjZjFeEU4EFy+b+a1SYQCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUC"
        "AwEAAaOCAiAwggIcMCoGA1UdEQQjMCGCH2ludmFsaWQtZXhwZWN0ZWQtc2N0LmJh"
        "ZHNzbC5jb20wCQYDVR0TBAIwADArBgNVHR8EJDAiMCCgHqAchhpodHRwOi8vZ3Au"
        "c3ltY2IuY29tL2dwLmNybDBvBgNVHSAEaDBmMGQGBmeBDAECATBaMCoGCCsGAQUF"
        "BwIBFh5odHRwczovL3d3dy5yYXBpZHNzbC5jb20vbGVnYWwwLAYIKwYBBQUHAgIw"
        "IAweaHR0cHM6Ly93d3cucmFwaWRzc2wuY29tL2xlZ2FsMB8GA1UdIwQYMBaAFJfC"
        "J1CewsnsDIgyyHyt4qYBT9pvMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUEFjAUBggr"
        "BgEFBQcDAQYIKwYBBQUHAwIwVwYIKwYBBQUHAQEESzBJMB8GCCsGAQUFBzABhhNo"
        "dHRwOi8vZ3Auc3ltY2QuY29tMCYGCCsGAQUFBzAChhpodHRwOi8vZ3Auc3ltY2Iu"
        "Y29tL2dwLmNydDAPBgMrZU0ECDAGAgEBAgEBMIGKBgorBgEEAdZ5AgQCBHwEegB4"
        "AHYAp85KTmIH4K3e5f2qSx+GdodntdACpV1HMQ5+ZwqV6rIAAAFYb//OtAAABAMA"
        "RzBFAiEAuAOtNPb8Dyz/hKCG5dfPWvAKB2Jqf7OmRGTxlaRIRRECIC9hjVMbb0q4"
        "CmeyB+GPba3RBEpes4nvfGDCaFP5PR9tMA0GCSqGSIb3DQEBCwUAA4IBAQBIMHww"
        "vuW8XOBdxw0Mq0z3NFsAW2XvkcXtTGmB5savCpRJ6LN0ub6NJX+KiFmDo9voMDro"
        "YL7o9i+BrSAFo3hr8QNxLLJaXD54h8lJ0oCkzyZtzezM8p2PWsEjouePtdE0AIHn"
        "RK0SnKBk9w0b3CMSbzObc1Cu8ATqZnE51d7xurim7qTZMhJM2hi9HeLPLVdEBiuC"
        "zmYtNpRMMQqbQvvrXftAohq/W90rK42Ss8kYIf8FsVTa5VaqXW7lIh/3JmBNLZ1D"
        "Aw5rmaztWlYO64YS7z4am5d9h2rrF1rfgv9Mc3caxAUO3sJZDRyhYaj+7BUgv8HR"
        "otJHkjr2ASPp31Yf"
        "-----END CERTIFICATE-----"_s,
#if LIMIT_TO_ONE_CERTIFICATE
        //  1 s:C = US, O = GeoTrust Inc., CN = RapidSSL SHA256 CA
        //    i:C = US, O = GeoTrust Inc., CN = GeoTrust Global CA
        "-----BEGIN CERTIFICATE-----"
        "MIIETTCCAzWgAwIBAgIDAjpxMA0GCSqGSIb3DQEBCwUAMEIxCzAJBgNVBAYTAlVT"
        "MRYwFAYDVQQKEw1HZW9UcnVzdCBJbmMuMRswGQYDVQQDExJHZW9UcnVzdCBHbG9i"
        "YWwgQ0EwHhcNMTMxMjExMjM0NTUxWhcNMjIwNTIwMjM0NTUxWjBCMQswCQYDVQQG"
        "EwJVUzEWMBQGA1UEChMNR2VvVHJ1c3QgSW5jLjEbMBkGA1UEAxMSUmFwaWRTU0wg"
        "U0hBMjU2IENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu1jBEgEu"
        "l9h9GKrIwuWF4hdsYC7JjTEFORoGmFbdVNcRjFlbPbFUrkshhTIWX1SG5tmx2GCJ"
        "a1i+ctqgAEJ2sSdZTM3jutRc2aZ/uyt11UZEvexAXFm33Vmf8Wr3BvzWLxmKlRK6"
        "msrVMNI4/Bk7WxU7NtBDTdFlodSLwWBBs9ZwF8w5wJwMoD23ESJOztmpetIqYpyg"
        "C04q18NhWoXdXBC5VD0tA/hJ8LySt7ecMcfpuKqCCwW5Mc0IW7siC/acjopVHHZD"
        "dvDibvDfqCl158ikh4tq8bsIyTYYZe5QQ7hdctUoOeFTPiUs2itP3YqeUFDgb5rE"
        "1RkmiQF1cwmbOwIDAQABo4IBSjCCAUYwHwYDVR0jBBgwFoAUwHqYaI2J+6sFZAwR"
        "fap9ZbjKzE4wHQYDVR0OBBYEFJfCJ1CewsnsDIgyyHyt4qYBT9pvMBIGA1UdEwEB"
        "/wQIMAYBAf8CAQAwDgYDVR0PAQH/BAQDAgEGMDYGA1UdHwQvMC0wK6ApoCeGJWh0"
        "dHA6Ly9nMS5zeW1jYi5jb20vY3Jscy9ndGdsb2JhbC5jcmwwLwYIKwYBBQUHAQEE"
        "IzAhMB8GCCsGAQUFBzABhhNodHRwOi8vZzIuc3ltY2IuY29tMEwGA1UdIARFMEMw"
        "QQYKYIZIAYb4RQEHNjAzMDEGCCsGAQUFBwIBFiVodHRwOi8vd3d3Lmdlb3RydXN0"
        "LmNvbS9yZXNvdXJjZXMvY3BzMCkGA1UdEQQiMCCkHjAcMRowGAYDVQQDExFTeW1h"
        "bnRlY1BLSS0xLTU2OTANBgkqhkiG9w0BAQsFAAOCAQEANevhiyBWlLp6vXmp9uP+"
        "bji0MsGj21hWID59xzqxZ2nVeRQb9vrsYPJ5zQoMYIp0TKOTKqDwUX/N6fmS/Zar"
        "RfViPT9gRlATPSATGC6URq7VIf5Dockj/lPEvxrYrDrK3maXI67T30pNcx9vMaJR"
        "BBZqAOv5jUOB8FChH6bKOvMoPF9RrNcKRXdLDlJiG9g4UaCSLT+Qbsh+QJ8gRhVd"
        "4FB84XavXu0R0y8TubglpK9YCa81tGJUheNI3rzSkHp6pIQNo0LyUcDUrVNlXWz4"
        "Px8G8k/Ll6BKWcZ40egDuYVtLLrhX7atKz4lecWLVtXjCYDqwSfC2Q7sRwrp0Mr8"
        "2A=="
        "-----END CERTIFICATE-----"_s
#endif
    };

    runTest({ "invalid-expected-sct.badssl.com"_s, State::EndMode::Fail, true, X509_V_ERR_CERT_HAS_EXPIRED, WTFMove(certs) });
}

} // namespace TestWebKitAPI

#endif
