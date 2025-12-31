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
#include "GetLine.h"
#include "MainWindow.h"

#include <CertificateStore/CertificateStore.h>
#include <WebKit/WKRunLoop.h>
#include <dlfcn.h>
#include <toolkitten/Application.h>

using namespace std;
using toolkitten::Widget;
using toolkitten::Application;

class ApplicationClient : public Application::Client {
public:
    void requestNextFrame() override
    {
        WKRunLoopCallOnMainThread(updateApplication, this);
    }

private:
    static void updateApplication(void*)
    {
        Application::singleton().update();
    }
};

int main(int argc, char *argv[])
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
    dlopen(ToolKitten_LOAD_AT, RTLD_NOW);
    dlopen(WebKitRequirements_LOAD_AT, RTLD_NOW);

#if HAVE(OPENGL_SERVER_SUPPORT)
    dlopen("libSceGLSlimClientVSH", RTLD_NOW);
#endif

    ApplicationClient applicationClient;
    auto& app = Application::singleton();
    app.init(&applicationClient);

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

    if (CertificateStore::isAllowedLoadingRootCertificateFromFsroot())
        setenv_np("ALLOW_LOADING_ROOT_CA_FROM_FSROOT", "1", 1);

#if defined(ENABLE_STATIC_JSC) && !ENABLE_STATIC_JSC
    dlopen("libJavaScriptCore", RTLD_NOW);
#endif

    dlopen("libWebKit", RTLD_NOW);

    auto mainWindow = make_unique<MainWindow>(argc > 1 ? argv[1] : nullptr);
    mainWindow->setFocused();
    app.setRootWidget(std::move(mainWindow));

    // Request the first frame to start the application loop.
    applicationClient.requestNextFrame();

    WKRunLoopRunMain();

    return 0;
}
