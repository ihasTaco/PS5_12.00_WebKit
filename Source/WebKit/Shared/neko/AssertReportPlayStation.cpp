/*
 * Copyright (C) 2022 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AssertReportPlayStation.h"

#include "AssertReportProxyMessages.h"
#include "MessageSenderInlines.h"

namespace WebKit {
WTF::RecursiveLockAdapter<Lock> AssertReportPlayStation::singletonLock;

AssertReportPlayStation* AssertReportPlayStation::singleton(AuxiliaryProcess* process)
{
    static AssertReportPlayStation* assertReporter = nullptr;

    Locker locker(singletonLock);
    if (!assertReporter && process) {
        assertReporter = new AssertReportPlayStation(process);
        WTF::setAssertReportFunction(AssertReportPlayStation::reportFunction);
    }

    return assertReporter;
}

void AssertReportPlayStation::reportFunction(const char* file, uint32_t line)
{
    Locker locker(singletonLock);
    if (auto reporter = AssertReportPlayStation::singleton())
        reporter->reportSync(file, line);
}

AssertReportPlayStation::AssertReportPlayStation(AuxiliaryProcess* process)
{
    m_process = process;
}

AssertReportPlayStation::~AssertReportPlayStation()
{
}

void AssertReportPlayStation::reportSync(const char* file, uint32_t line)
{
    Locker locker(singletonLock);
    if (m_process && m_process->parentProcessConnection())
        m_process->sendSync(Messages::AssertReportProxy::ReportSync(String::fromUTF8(file), line));
}

}
