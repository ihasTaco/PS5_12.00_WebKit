/*
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
#include "UserAgent.h"

#include "GetLine.h"

using namespace std;

// A simple text file, where title and UserAgent lines are alternately written.
static const char* s_userAgentFilePath = "/hostapp/MiniBrowserUserAgents.txt";

void listUserAgent(const std::string& current, std::function<bool(UserAgent&&)>&& callback)
{
    const UserAgent defaults[] = {
        { string("WebKit Neko"), string("Mozilla/5.0 (PlayStation 4 6.00) AppleWebKit/605.1.15 (KHTML, like Gecko)") },
        { string("WebKit Manx"), string("Mozilla/5.0 (PlayStation 4 5.50) AppleWebKit/601.2 (KHTML, like Gecko)") },
        { string("Mac Safari 605.1.x"), string("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/11.1.2 Safari/605.1.15")},
        { string("iPhone Safari 605.1.x"), string("Mozilla/5.0 (iPhone; CPU iPhone OS 12_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) FxiOS/12.1b10941 Mobile/16A5308e Safari/605.1.15")},
        { string("WebKitGTK"), string("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/11.0 Safari/605.1.15")},
        { string("Windows Chrome67"), string("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.2526.73 Safari/537.36")},
        { string("Windows FireFox61"), string("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0")}
    };
    const int uaNum = sizeof(defaults) / sizeof(struct UserAgent);

    for (int i = 0; i < uaNum; i++) {
        if (!defaults[i].uaString.compare(current))
            callback({ std::string("* ") + defaults[i].title, defaults[i].uaString });
        else
            callback({ std::string("  ") + defaults[i].title, defaults[i].uaString });
    }

    FILE* fin = fopen(s_userAgentFilePath, "r");
    if (!fin) {
        fprintf(stderr, "Cannot open %s\n", s_userAgentFilePath);
        return;
    }

    string title;
    string uaString;
    while (getLine(fin, title) && getLine(fin, uaString)) {
        if (!uaString.compare(current))
            callback({ std::string("* ") + title, uaString });
        else
            callback({ std::string("  ") + title, uaString });
    }
    fclose(fin);
}
