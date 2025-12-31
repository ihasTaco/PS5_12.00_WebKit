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
#include "PlatformWebView.h"

#include <WebKit/WKPagePrivatePlayStation.h>

namespace WTR {

PlatformWebView::PlatformWebView(WKPageConfigurationRef config, const TestOptions&opts)
    : m_window(0)
    , m_windowIsKey(true)
    , m_options(opts)
{
    m_view = WKViewCreate(config);
    // Chosen a view size of 800 x 600 for LayoutTests, to remain consistent
    // with LayoutTest's requirement and with expected results
    WKSize size { 800, 600 };
    WKViewSetSize(m_view, size);
    WKViewSetVisible(m_view, true);
}

PlatformWebView::~PlatformWebView()
{
}

void PlatformWebView::resizeTo(unsigned width, unsigned height, WebViewSizingMode)
{
    WKViewSetSize(m_view, WKSizeMake(width, height));
}

WKPageRef PlatformWebView::page()
{
    return WKViewGetPage(m_view);
}

void PlatformWebView::focus()
{
}

WKRect PlatformWebView::windowFrame()
{
    // This method returns a default frame size of 800 x 600
    // because m_window is currently not used.
    WKRect frame;
    frame.origin.x = 0;
    frame.origin.y = 0;
    frame.size.width = 800;
    frame.size.height = 600;
    return frame;
}

void PlatformWebView::setWindowFrame(WKRect frame, WebViewSizingMode)
{
    resizeTo(frame.size.width, frame.size.height);
}

void PlatformWebView::addChromeInputField()
{
}

void PlatformWebView::removeChromeInputField()
{
}

void PlatformWebView::makeWebViewFirstResponder()
{
}

PlatformImage PlatformWebView::windowSnapshotImage()
{
    int width = 800;
    int height = 600;

    auto surface = std::make_unique<unsigned char[]>(cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width) * height);
    WKRect rect;
    rect.origin.x = 0;
    rect.origin.y = 0;
    rect.size.height = height;
    rect.size.width = width;
    WKPagePaint(WKViewGetPage(m_view), surface.get(), WKSizeMake(width, height), rect);
    cairo_surface_t* tmpSurface = cairo_image_surface_create_for_data(surface.get(), CAIRO_FORMAT_ARGB32, width, height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width));
    cairo_surface_t* imageSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* context = cairo_create(imageSurface);
    cairo_set_source_surface(context, tmpSurface, 0, 0);
    cairo_rectangle(context, 0, 0, width, height);
    cairo_fill(context);
    cairo_destroy(context);
    cairo_surface_destroy(tmpSurface);
    return imageSurface;
}

void PlatformWebView::didInitializeClients()
{
}

void PlatformWebView::changeWindowScaleIfNeeded(float)
{
}


void PlatformWebView::setWindowIsKey(bool isKey)
{
    m_windowIsKey = isKey;
}

void PlatformWebView::setTextInChromeInputField(const String&)
{
}

void PlatformWebView::selectChromeInputField()
{
}

String PlatformWebView::getSelectedTextInChromeInputField()
{
    return { };
}

void PlatformWebView::addToWindow()
{
}

void PlatformWebView::setNavigationGesturesEnabled(bool)
{
}

bool PlatformWebView::isSecureEventInputEnabled() const
{
    return false;
}

} // namespace WTR
