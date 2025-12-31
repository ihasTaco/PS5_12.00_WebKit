/*
 * Copyright (C) 2011 Igalia S.L.
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

#include "TestController.h"
#include "process-initialization/nk-webkittestrunner.h"
#include <dlfcn.h>
#include <string>

static bool getLine(FILE*, std::string&);

int main(int argc, char** argv)
{
#if ENABLE(MAT)
    dlopen("MemoryAnalyzer", RTLD_NOW);
#endif

    dlopen(PNG_LOAD_AT, RTLD_NOW);
    dlopen(ICU_LOAD_AT, RTLD_NOW);
    dlopen(Fontconfig_LOAD_AT, RTLD_NOW);
    dlopen(Freetype_LOAD_AT, RTLD_NOW);
    dlopen(HarfBuzz_LOAD_AT, RTLD_NOW);
    dlopen(Cairo_LOAD_AT, RTLD_NOW);
    dlopen(WebKitRequirements_LOAD_AT, RTLD_NOW);

#if defined(ENABLE_STATIC_JSC) && !ENABLE_STATIC_JSC
    dlopen("libJavaScriptCore", RTLD_NOW);
#endif

    dlopen("libWebKit", RTLD_NOW);

    setenv_np("WebInspectorServerPort", "868", 1);

    // An environment variable of debug log setting for libmediaplayer.a.
    //   Only first line of MediaPlayerDebug.txt is avaiable.
    //   format: "Media=debug,MediaSource=debug,EncryptedMedia=debug,VCS=debug,MCS=debug"
    FILE* fin = fopen("/host/MediaPlayerDebug.txt", "r");
    if (fin) {
        std::string setting;
        if (getLine(fin, setting))
            setenv_np("MediaPlayerDebug", setting.c_str(), 1);
        fclose(fin);
    }

    WTR::TestController controller(argc, const_cast<const char**>(argv));

    return 0;
}

static bool getLine(FILE* fin, std::string& out)
{
    constexpr int BufferSize = 256;
    char buf[BufferSize];

    out.clear();

    while (fgets(buf, BufferSize, fin)) {
        char* eol = strpbrk(buf, "\r\n");
        if (eol) {
            *eol = '\0';
            out.append(buf);
            break;
        }
        out.append(buf);
    }

    return !out.empty();
}
