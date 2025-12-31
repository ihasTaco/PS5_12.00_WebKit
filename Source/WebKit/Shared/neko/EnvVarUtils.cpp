/*
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
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
#include "EnvVarUtils.h"

#include <wtf/text/StringBuilder.h>

namespace WebKit {

static constexpr ASCIILiteral envVarsAllowlist[] = {
    "WebKitModuleID"_s,
    "WebInspectorServerPort"_s,
    "DefaultThreadPriority"_s,
    "FMEMSizeLimit"_s,
    "MediaPlayerDebug"_s,
    "ConfigureJscForTesting"_s,
    "ALLOW_LOADING_ROOT_CA_FROM_FSROOT"_s,
    "TweaksSetting"_s,
};

static bool isOnAllowlist(const StringView& envVar)
{
    for (auto name : envVarsAllowlist) {
        if (envVar == name)
            return true;
    }
    return false;
}

String stringifyEnvVars()
{
    StringBuilder output;
    for (auto name : envVarsAllowlist) {
        char buf[256];
        if (getenv_np(name, buf, sizeof buf))
            continue;

        if (!output.isEmpty())
            output.append('&');
        output.append(name);
        output.append('=');
        output.append(buf);
    }
    return output.toString();
}

void parseAndSetEnvVars(const char* envVarsString)
{
    for (auto bytes : StringView::fromLatin1(envVarsString).split('&')) {
        auto equalIndex = bytes.find('=');
        if (equalIndex == notFound)
            continue;

        auto name = bytes.substring(0, equalIndex);
        auto value = bytes.substring(equalIndex + 1);
        if (name && value && isOnAllowlist(name))
            setenv_np(name.utf8().data(), value.utf8().data(), 1);
    }
}

} // namespace WebKit
