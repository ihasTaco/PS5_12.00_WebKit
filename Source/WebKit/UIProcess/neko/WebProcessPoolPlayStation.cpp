/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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
#include "WebProcessPool.h"

#include "AssertReportProxy.h"
#include "AssertReportProxyMessages.h"
#include "WebProcessCreationParameters.h"

#if ENABLE(REMOTE_INSPECTOR)
#include "AutomationClientPlayStation.h"
#include "WebPageProxy.h"
#include <JavaScriptCore/RemoteInspectorServer.h>
#include <wtf/text/CString.h>
#endif

#if ENABLE(ACCESSIBILITY)
#include "WebProcessMessages.h"
#endif

#define MAKE_PROCESS_PATH(x) "/app0/" #x ".self"

namespace WebKit {

#if ENABLE(REMOTE_INSPECTOR)
static unsigned getWebInspectorServerPort()
{
    static unsigned port = 0;
    static std::once_flag flag;
    std::call_once(flag, [] {
        char buf[64];
        if (!getenv_np("WebInspectorServerPort", buf, sizeof buf))
            port = atoi(buf);
    });

    return port;
}

static bool initializeRemoteInspectorServer()
{
    unsigned port = getWebInspectorServerPort();
    if (!port)
        return false;

    if (Inspector::RemoteInspectorServer::singleton().isRunning())
        return true;

    return Inspector::RemoteInspectorServer::singleton().start(nullptr, port);
}
#endif

void WebProcessPool::platformInitialize()
{
    m_userId = m_configuration->userId();

#if ENABLE(REMOTE_INSPECTOR)
    initializeRemoteInspectorServer();
    setAutomationClient(WTF::makeUnique<AutomationClient>(*this));
#endif
}

void WebProcessPool::platformInitializeNetworkProcess(NetworkProcessCreationParameters&)
{
    notImplemented();
}

void WebProcessPool::platformInitializeWebProcess(const WebProcessProxy&, WebProcessCreationParameters& parameters)
{
#if ENABLE(ACCESSIBILITY)
    parameters.accessibilityEnabled = m_accessibilityEnabled;
#endif
}

void WebProcessPool::platformInvalidateContext()
{
    notImplemented();
}

String WebProcessPool::legacyPlatformDefaultWebProcessPath()
{
    return String::fromLatin1(MAKE_PROCESS_PATH(WebProcess));
}

String WebProcessPool::legacyPlatformDefaultNetworkProcessPath()
{
    return String::fromLatin1(MAKE_PROCESS_PATH(NetworkProcess));
}

void WebProcessPool::platformResolvePathsForSandboxExtensions()
{
    m_resolvedPaths.webProcessPath = resolvePathForSandboxExtension(m_configuration->webProcessPath().isEmpty() ? legacyPlatformDefaultWebProcessPath() : m_configuration->webProcessPath());
    m_resolvedPaths.networkProcessPath = resolvePathForSandboxExtension(m_configuration->networkProcessPath().isEmpty() ? legacyPlatformDefaultNetworkProcessPath() : m_configuration->networkProcessPath());
}

#if ENABLE(ACCESSIBILITY)
void WebProcessPool::setAccessibilityEnabled(bool enable)
{
    if (m_accessibilityEnabled == enable)
        return;

    m_accessibilityEnabled = enable;
    sendToAllProcesses(Messages::WebProcess::SetAccessibilityEnabled(enable));
}
#endif 

#if ENABLE(REMOTE_INSPECTOR)
void WebProcessPool::setPagesControlledByAutomation(bool controlled)
{
    for (auto& process : m_processes) {
        for (auto& page : process->pages())
            if (page)
                page->setControlledByAutomation(controlled);
    }
}
#endif

void WebProcessPool::setAssertReportCallback(WTF::AssertReportFunction callback)
{
    if (AssertReportProxy::singleton())
        AssertReportProxy::singleton()->setReportCallback(callback);
}

} // namespace WebKit
