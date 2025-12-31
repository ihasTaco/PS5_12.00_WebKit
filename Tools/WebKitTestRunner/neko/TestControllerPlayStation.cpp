/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestController.h"

#include "PlatformWebView.h"
#include <WebKit/WKContextConfigurationPlayStation.h>
#include <wtf/FileSystem.h>
#include <wtf/Platform.h>
#include <wtf/RunLoop.h>
#include <wtf/text/WTFString.h>

namespace WTR {

void TestController::notifyDone()
{
    RunLoop::main().stop();
}

void TestController::platformInitialize(const Options&)
{
    auto pathForTesting = String::fromUTF8(platformLibraryPathForTesting());
    FileSystem::deleteNonEmptyDirectory(pathForTesting);
}

WKPreferencesRef TestController::platformPreferences()
{
    return WKPageGroupGetPreferences(m_pageGroup.get());
}

void TestController::platformDestroy()
{
}

void TestController::platformRunUntil(bool& done, WTF::Seconds timeout)
{
    struct TimeoutTimer {
        TimeoutTimer()
            : timer(RunLoop::main(), this, &TimeoutTimer::fired)
        { }

        void fired()
        {
            timedOut = true;
            RunLoop::main().stop();
        }

        RunLoop::Timer timer;
        bool timedOut { false };
    } timeoutTimer;

    if (timeout >= 0_s)
        timeoutTimer.timer.startOneShot(timeout);

    while (!done && !timeoutTimer.timedOut)
        RunLoop::main().run();

    timeoutTimer.timer.stop();
}

void TestController::initializeInjectedBundlePath()
{
    m_injectedBundlePath.adopt(WKStringCreateWithUTF8CString("/app0/libTestRunnerInjectedBundle.sprx"));
}

void TestController::initializeTestPluginDirectory()
{
    m_testPluginDirectory.adopt(WKStringCreateWithUTF8CString(""));
}

void TestController::platformInitializeContext()
{
    WKContextSetUsesSingleWebProcess(m_context.get(), true);
}

void TestController::runModal(PlatformWebView*)
{
}

void TestController::abortModal()
{
}

const char* TestController::platformLibraryPathForTesting()
{
    return "/data/nekotests/wtr";
}

void TestController::setHidden(bool hidden)
{
    if (!m_mainWebView)
        return;
    WKViewSetVisible(m_mainWebView->platformView(), !hidden);
}

WKContextRef TestController::platformContext()
{
    return m_context.get();
}

bool TestController::platformResetStateToConsistentValues(const TestOptions&)
{
    // [TBD]
    return true;
}

TestFeatures TestController::platformSpecificFeatureDefaultsForTest(const TestCommand&) const
{
    TestFeatures features;
    features.boolTestRunnerFeatures.insert({ "FullScreenEnabled", true });
    features.boolTestRunnerFeatures.insert({ "NeedsSiteSpecificQuirks", true });
    features.boolTestRunnerFeatures.insert({ "DNSPrefetchingEnabled", true });
    return features;
}

void TestController::platformConfigureViewForTest(const TestInvocation&)
{
    WKPageSetApplicationNameForUserAgent(mainWebView()->page(), WKStringCreateWithUTF8CString("WebKitTestRunnerPlayStation"));
}

#define MAKE_PROCESS_PATH(x) "/app0/" #x ".self"
void TestController::platformContextConfiguration(WKRetainPtr<WKContextConfigurationRef> configuration) const
{
    WKRetainPtr<WKStringRef> webProcessPath = adoptWK(WKStringCreateWithUTF8CString(MAKE_PROCESS_PATH(WebProcess)));
    WKRetainPtr<WKStringRef> networkProcessPath = adoptWK(WKStringCreateWithUTF8CString(MAKE_PROCESS_PATH(NetworkProcess)));
    WKContextConfigurationSetWebProcessPath(configuration.get(), webProcessPath.get());
    WKContextConfigurationSetNetworkProcessPath(configuration.get(), networkProcessPath.get());
    WKContextConfigurationSetUserId(configuration.get(), -1);
}

} // namespace WTR
