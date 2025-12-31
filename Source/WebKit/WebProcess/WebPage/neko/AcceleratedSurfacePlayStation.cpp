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
#include "AcceleratedSurfacePlayStation.h"

#include <EGL/egl.h>
#include <WebCore/GLContext.h>
#include <WebCore/PlatformDisplay.h>
#include <canvas/canvas.h>
#include <mediaplayer/PSMediaPlayerBackend.h>
#include <thread>

namespace {
const int AcceleratedWindowInitialWidth = 1920;
const int AcceleratedWindowInitialHeight = 1080;
}

namespace WebKit {

std::unique_ptr<AcceleratedSurfacePlayStation> AcceleratedSurfacePlayStation::create(WebPage& webPage, Client& client)
{
    return std::unique_ptr<AcceleratedSurfacePlayStation>(new AcceleratedSurfacePlayStation(webPage, client));
}

AcceleratedSurfacePlayStation::AcceleratedSurfacePlayStation(WebPage& webPage, Client& client)
    : AcceleratedSurface(webPage, client)
    , m_vBlankMonitor(VBlankMonitor::create())
{
}

void AcceleratedSurfacePlayStation::initialize()
{
    m_window.uHeight = AcceleratedWindowInitialHeight;
    m_window.uWidth = AcceleratedWindowInitialWidth;
    m_window.uID = canvas::Canvas::kInvalidIdValue;
    // We don't create actual Framebuffer here because we don't have GLContext yet.
#if ENABLE(PUNCH_HOLE)
    m_canvas = canvas::IndirectCanvas::create();
#else
    m_canvas = canvas::Canvas::create();
#endif
}

void AcceleratedSurfacePlayStation::finalize()
{
    // We don't have GLContext here, m_canvas requires it to delete framebuffer.
    WebCore::PlatformDisplay::sharedDisplayForCompositing().sharingGLContext()->makeContextCurrent();
    m_canvas = nullptr;

    // FIXME: We should unbind the context, but there is not API to do that.
}

bool AcceleratedSurfacePlayStation::hostResize(const WebCore::IntSize& size)
{
    if (!AcceleratedSurface::hostResize(size))
        return false;

    // Resizing m_canvas is occured in willRenderFrame.
    // Returning true means that surfaceID() needs updating.
    // In PlayStation, we don't need to update the surfaceID even when the size has been changed.
    return false;
}

void AcceleratedSurfacePlayStation::clientResize(const WebCore::IntSize& size)
{
    // FIXME: What should we do here?
    AcceleratedSurface::clientResize(size);
}

// Need to return EGLWindowType
uint64_t AcceleratedSurfacePlayStation::window() const
{
    return (uint64_t)&m_window;
}

// AcceleratedSurfacePlayStation::surfaceID is used as LayerTreeContext.contextID.
// LayerTreeContext is transfered to UIProcess in enterAcceleratedMode event.
// That's why we return canvas handle instead of canvas id.
uint64_t AcceleratedSurfacePlayStation::surfaceID() const
{
    ASSERT(m_canvas);
    // AcceleratedSurfacePlayStation::surfaceID is const member function, so m_canvas->handle also must be const.
    return m_canvas->handle();
}

bool AcceleratedSurfacePlayStation::shouldPaintMirrored() const
{
    return true;
}

// Bind Framebuffer which is backed by canvas.
void AcceleratedSurfacePlayStation::willRenderFrame()
{
    ASSERT(m_canvas);
    m_canvas->ensureBufferInitialized(m_size.width(), m_size.height());
#if ENABLE(PUNCH_HOLE)
    canvas::CanvasUtil::memorizeForRepeat(false);
#endif
    m_canvas->activateAsFrameBuffer();
}

void AcceleratedSurfacePlayStation::didRenderFrame()
{
    ASSERT(m_canvas);
    glFlush();
    m_canvas->swapBuffers();

#if ENABLE(PUNCH_HOLE)
    // FIXME: If there are 2 or more AcceleratedSurfacePlayStation instances in one process,
    // then calling memorizeForRepeat(true) accidentally overwrites the other's repeat addresses.
    // To avoid it, we should call IndirectCanvas::activateAsCanvas for all the AcceleratedSurfacePlayStation instances between
    // one memorizeForRepeat and one flushWithRepeat like following code.

    // uint8_t postEventId = canvas::CanvasUtil::setPostEvent();
    // canvas::CanvasUtil::memorizeForRepeat(true);
    // for (auto& canvas : canvases) {
    //     canvas->activateAsCanvas();
    // }
    // canvas::CanvasUtil::flushWithRepeat();
    // canvas::CanvasUtil::waitPostEvent(postEventId);

    uint8_t postEventId = canvas::CanvasUtil::setPostEvent();
    canvas::CanvasUtil::memorizeForRepeat(true);
#endif

    m_canvas->activateAsCanvas();

    // FIXME: Don't make threads everytime.
    // Note that these code must be executed in the other thread because
    // didRenderFrame is called from rendering thread which is shared by several webpages.
    std::thread t([this, postEventId]() {
        // FIXME: Don't call canvas::CanvasUtil::finish everytime after rendering one webpage.
        // It throttles the FPS of every webpages.
#if ENABLE(PUNCH_HOLE)
        canvas::CanvasUtil::flushWithRepeat();
        canvas::CanvasUtil::waitPostEvent(postEventId);
#else
        canvas::CanvasUtil::finish();
#endif
        m_client.frameComplete();
    });
    t.detach();
}

#if ENABLE(PUNCH_HOLE)
void AcceleratedSurfacePlayStation::didUpdatePunchHoles(const Vector<WebCore::PunchHole>& holes)
{
    std::vector<canvas::IndirectCanvas::IndirectCompositingSource> sources;
    std::vector<mediaplayer::PSMediaCompositionControl> compositionControls;

    for (const auto& hole : holes) {
        if (shouldPaintMirrored()) {
            if (canvas::CanvasUtil::isTransparentCanvas(hole.canvasHandle)) {
                // OpenGL's screen space is the follwoing
                // (-1, 1)          (1, 1)
                //     +---------------+
                //     |               |
                //     |               |
                //     +---------------+
                // (-1, -1)          (1, -1)
                // and we need to convert to
                // (0, 0)            (1, 0)
                //     +---------------+
                //     |               |
                //     |               |
                //     +---------------+
                // (0, 1)            (1, 1)
                const auto leftAndRight = std::minmax({
                    (hole.quad.p1().x() + 1) / 2,
                    (hole.quad.p2().x() + 1) / 2,
                    (hole.quad.p3().x() + 1) / 2,
                    (hole.quad.p4().x() + 1) / 2,
                });
                const auto topAndBottom = std::minmax({
                    1 - ((-hole.quad.p1().y() + 1) / 2),
                    1 - ((-hole.quad.p2().y() + 1) / 2),
                    1 - ((-hole.quad.p3().y() + 1) / 2),
                    1 - ((-hole.quad.p4().y() + 1) / 2),
                });

                // v0(left, top)       v2(right, top)
                //     +---------------+
                //     |               |
                //     |               |
                //     +---------------+
                // v1(left, bottom)    v3(right, bottom)
                const float left = leftAndRight.first;
                const float right = leftAndRight.second;
                const float top = topAndBottom.first;
                const float bottom = topAndBottom.second;

                mediaplayer::PSMediaCompositionControl composition = { };
                // v0 (left, top)
                composition.vert[0].f[0] = left;
                composition.vert[0].f[1] = top;
                // v1 (left, bottom)
                composition.vert[1].f[0] = left;
                composition.vert[1].f[1] = bottom;
                // v2 (right, top)
                composition.vert[2].f[0] = right;
                composition.vert[2].f[1] = top;
                // v3 (right, bottom)
                composition.vert[3].f[0] = right;
                composition.vert[3].f[1] = bottom;

                composition.canvasHandle = hole.canvasHandle;

                compositionControls.push_back(composition);
            } else {
                canvas::IndirectCanvas::IndirectCompositingSource source;
                source.canvasHandle = hole.canvasHandle;
                // if shouldPaintMirrored, then vertices is inverted y-axis.
                // vertices is ordered by v0 - v1 - v2 - v3, GL_TRIANGLE_FAN,
                // but canvas::IndirectCanvas::IndirectCompositingSource requires v0 - v1 - v2 - v3, GL_TRIANGLE_STRIP.
                source.vertices[0][0] = hole.quad.p1().x();
                source.vertices[0][1] = -hole.quad.p1().y();
                source.vertices[1][0] = hole.quad.p4().x();
                source.vertices[1][1] = -hole.quad.p4().y();
                source.vertices[2][0] = hole.quad.p2().x();
                source.vertices[2][1] = -hole.quad.p2().y();
                source.vertices[3][0] = hole.quad.p3().x();
                source.vertices[3][1] = -hole.quad.p3().y();
                sources.push_back(source);
            }
        } else {
            // not supported yet.
            ASSERT_NOT_REACHED();
        }
    }

    if (!compositionControls.empty())
        mediaplayer::PSMediaPlayerBackend::setTransparentCanvasCompositionCommand(compositionControls.data(), compositionControls.size());

    m_canvas->setIndirectCompositingSource(WTFMove(sources));
}
#endif
}
