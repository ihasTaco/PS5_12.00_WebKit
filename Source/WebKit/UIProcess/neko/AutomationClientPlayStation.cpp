/*
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AutomationClientPlayStation.h"

#if ENABLE(REMOTE_INSPECTOR)
#include "WebAutomationSession.h"
#include "WebPageProxy.h"
#include <wtf/RunLoop.h>
#endif

namespace WebKit {

#if ENABLE(REMOTE_INSPECTOR)

// AutomationSessionClient
AutomationSessionClient::AutomationSessionClient(const String& sessionIdentifier)
{
    m_sessionIdentifier = sessionIdentifier;
}

void AutomationSessionClient::requestNewPageWithOptions(WebKit::WebAutomationSession& session, API::AutomationSessionBrowsingContextOptions, CompletionHandler<void(WebKit::WebPageProxy*)>&& completionHandler)
{
    RefPtr<WebPageProxy> pageProxy;

    auto* processPool = session.processPool();
    if (processPool && processPool->processes().size()) {
        // Set controlledByAutomation flag to all pages in all processes
        processPool->setPagesControlledByAutomation(true);
        // The default page for automation target is the first page of the first process
        auto& processProxy = processPool->processes()[0];
        if (processProxy->pageCount())
            pageProxy = *(processProxy->pages().begin());
    }

    completionHandler(pageProxy.get());
}

void AutomationSessionClient::didDisconnectFromRemote(WebKit::WebAutomationSession& session)
{
    session.setClient(nullptr);

    auto* processPool = session.processPool();
    if (processPool) {
        processPool->setAutomationSession(nullptr);
        processPool->setPagesControlledByAutomation(false);
    }
}

// AutomationClient
AutomationClient::AutomationClient(WebProcessPool& processPool)
    : m_processPool(processPool)
{
    Inspector::RemoteInspector::singleton().setClient(this);
}

AutomationClient::~AutomationClient()
{
    Inspector::RemoteInspector::singleton().setClient(nullptr);
}

void AutomationClient::requestAutomationSession(const String& sessionIdentifier, const Inspector::RemoteInspector::Client::SessionCapabilities&)
{
    auto session = adoptRef(new WebAutomationSession());
    session->setSessionIdentifier(sessionIdentifier);
    session->setClient(WTF::makeUnique<AutomationSessionClient>(sessionIdentifier));
    m_processPool.setAutomationSession(WTFMove(session));
}

void AutomationClient::closeAutomationSession()
{
    if (!m_processPool.automationSession())
        return;

    m_processPool.automationSession()->setClient(nullptr);
    RunLoop::current().dispatch([this] {
        m_processPool.setAutomationSession(nullptr);
    });
}

#endif

} // namespace WebKit
