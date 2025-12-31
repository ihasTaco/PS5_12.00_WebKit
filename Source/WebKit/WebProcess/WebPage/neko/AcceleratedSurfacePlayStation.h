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

#pragma once

#if PLATFORM(PLAYSTATION)

#include "AcceleratedSurface.h"

#include "WebPage.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <VBlankMonitor/VBlankMonitor.h>
#include <canvas/canvas.h>
#include <utility>

namespace WebKit {

class AcceleratedSurfacePlayStation final : public AcceleratedSurface {
    WTF_MAKE_NONCOPYABLE(AcceleratedSurfacePlayStation);
    WTF_MAKE_FAST_ALLOCATED;

public:
    static std::unique_ptr<AcceleratedSurfacePlayStation> create(WebPage&, Client&);
    ~AcceleratedSurfacePlayStation() = default;
    uint64_t window() const override;
    uint64_t surfaceID() const override;
    bool hostResize(const WebCore::IntSize&) override;
    void clientResize(const WebCore::IntSize&) override;
    bool shouldPaintMirrored() const override;
    void initialize() override;
    void finalize() override;
    void willRenderFrame() override;
    void didRenderFrame() override;

#if ENABLE(PUNCH_HOLE)
    void didUpdatePunchHoles(const Vector<WebCore::PunchHole>&) override;
#endif

private:
    AcceleratedSurfacePlayStation(WebPage&, Client&);
#if ENABLE(PUNCH_HOLE)
    std::unique_ptr<canvas::IndirectCanvas> m_canvas;
#else
    std::unique_ptr<canvas::Canvas> m_canvas;
#endif

    std::unique_ptr<VBlankMonitor> m_vBlankMonitor;
    _EGLNativeWindowType m_window { };
};

} // namespace WebKit

#endif // PLATFORM(PLAYSTATION)
