/*
 * (C) 2018 Sony Interactive Entertainment Inc.
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
#include "WebViewAccessibilityClient.h"

#if ENABLE(ACCESSIBILITY)

#include "WKAPICast.h"
#include "PlayStationWebView.h"

using namespace WebCore;

namespace WebKit {

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic error "-Wswitch"
#endif
WKAXNotification toAPI(AXObjectCache::AXNotification notification)
{
    switch (notification) {
    case AXObjectCache::AXNotification::AXAccessKeyChanged:
        return kWKAXAccessKeyChanged;
    case AXObjectCache::AXNotification::AXActiveDescendantChanged:
        return kWKAXActiveDescendantChanged;
    case AXObjectCache::AXNotification::AXAutocorrectionOccured:
        return kWKAXAutocorrectionOccured;
    case AXObjectCache::AXNotification::AXAutofillTypeChanged:
        return kWKAXAutofillTypeChanged;
    case AXObjectCache::AXNotification::AXCellSlotsChanged:
        return kWKAXCellSlotsChanged;
    case AXObjectCache::AXNotification::AXCheckedStateChanged:
        return kWKAXCheckedStateChanged;
    case AXObjectCache::AXNotification::AXChildrenChanged:
        return kWKAXChildrenChanged;
    case AXObjectCache::AXNotification::AXColumnCountChanged:
        return kWKAXColumnCountChanged;
    case AXObjectCache::AXNotification::AXColumnIndexChanged:
        return kWKAXColumnIndexChanged;
    case AXObjectCache::AXNotification::AXColumnSpanChanged:
        return kWKAXColumnSpanChanged;
    case AXObjectCache::AXNotification::AXControlledObjectsChanged:
        return kWKAXControlledObjectsChanged;
    case AXObjectCache::AXNotification::AXCurrentStateChanged:
        return kWKAXCurrentStateChanged;
    case AXObjectCache::AXNotification::AXDescribedByChanged:
        return kWKAXDescribedByChanged;
    case AXObjectCache::AXNotification::AXDisabledStateChanged:
        return kWKAXDisabledStateChanged;
    case AXObjectCache::AXNotification::AXDropEffectChanged:
        return kWKAXDropEffectChanged;
    case AXObjectCache::AXNotification::AXFlowToChanged:
        return kWKAXFlowToChanged;
    case AXObjectCache::AXNotification::AXFocusedUIElementChanged:
        return kWKAXFocusedUIElementChanged;
    case AXObjectCache::AXNotification::AXFrameLoadComplete:
        return kWKAXFrameLoadComplete;
    case AXObjectCache::AXNotification::AXGrabbedStateChanged:
        return kWKAXGrabbedStateChanged;
    case AXObjectCache::AXNotification::AXHasPopupChanged:
        return kWKAXHasPopupChanged;
    case AXObjectCache::AXNotification::AXIdAttributeChanged:
        return kWKAXIdAttributeChanged;
    case AXObjectCache::AXNotification::AXImageOverlayChanged:
        return kWKAXImageOverlayChanged;
    case AXObjectCache::AXNotification::AXIsAtomicChanged:
        return kWKAXIsAtomicChanged;
    case AXObjectCache::AXNotification::AXKeyShortcutsChanged:
        return kWAXKeyShortcutsChanged;
    case AXObjectCache::AXNotification::AXLanguageChanged:
        return kWKAXLanguageChanged;
    case AXObjectCache::AXNotification::AXLayoutComplete:
        return kWKAXLayoutComplete;
    case AXObjectCache::AXNotification::AXLevelChanged:
        return kWKAXLevelChanged;
    case AXObjectCache::AXNotification::AXLoadComplete:
        return kWKAXLoadComplete;
    case AXObjectCache::AXNotification::AXNewDocumentLoadComplete:
        return kWKAXNewDocumentLoadComplete;
    case AXObjectCache::AXNotification::AXPageScrolled:
        return kWKAXPageScrolled;
    case AXObjectCache::AXNotification::AXPlaceholderChanged:
        return kWKAXPlaceholderChanged;
    case AXObjectCache::AXNotification::AXPositionInSetChanged:
        return kWKAXPositionInSetChanged;
    case AXObjectCache::AXNotification::AXRoleChanged:
        return kWKAXRoleChanged;
    case AXObjectCache::AXNotification::AXRoleDescriptionChanged:
        return kWKAXRoleDescriptionChanged;
    case AXObjectCache::AXNotification::AXRowIndexChanged:
        return kWKAXRowIndexChanged;
    case AXObjectCache::AXNotification::AXRowSpanChanged:
        return kWKAXRowSpanChanged;
    case AXObjectCache::AXNotification::AXSelectedCellsChanged:
        return kWKAXSelectedCellsChanged;
    case AXObjectCache::AXNotification::AXSelectedChildrenChanged:
        return kWKAXSelectedChildrenChanged;
    case AXObjectCache::AXNotification::AXSelectedStateChanged:
        return kWKAXSelectedStateChanged;
    case AXObjectCache::AXNotification::AXSelectedTextChanged:
        return kWKAXSelectedTextChanged;
    case AXObjectCache::AXNotification::AXSetSizeChanged:
        return kWKAXSetSizeChanged;
    case AXObjectCache::AXNotification::AXTableHeadersChanged:
        return kWKAXTableHeadersChanged;
    case AXObjectCache::AXNotification::AXTextCompositionBegan:
        return kWKAXTextCompositionBegan;
    case AXObjectCache::AXNotification::AXTextCompositionEnded:
        return kWKAXTextCompositionEnded;
    case AXObjectCache::AXNotification::AXURLChanged:
        return kWKAXURLChanged;
    case AXObjectCache::AXNotification::AXValueChanged:
        return kWKAXValueChanged;
    case AXObjectCache::AXNotification::AXScrolledToAnchor:
        return kWKAXScrolledToAnchor;
    case AXObjectCache::AXNotification::AXLiveRegionCreated:
        return kWKAXLiveRegionCreated;
    case AXObjectCache::AXNotification::AXLiveRegionChanged:
        return kWKAXLiveRegionChanged;
    case AXObjectCache::AXNotification::AXLiveRegionRelevantChanged:
        return kWKAXLiveRegionRelevantChanged;
    case AXObjectCache::AXNotification::AXLiveRegionStatusChanged:
        return kWKAXLiveRegionStatusChanged;
    case AXObjectCache::AXNotification::AXMaximumValueChanged:
        return kWKAXMaximumValueChanged;
    case AXObjectCache::AXNotification::AXMenuListItemSelected:
        return kWKAXMenuListItemSelected;
    case AXObjectCache::AXNotification::AXMenuListValueChanged:
        return kWKAXMenuListValueChanged;
    case AXObjectCache::AXNotification::AXMenuClosed:
        return kWKAXMenuClosed;
    case AXObjectCache::AXNotification::AXMenuOpened:
        return kWKAXMenuOpened;
    case AXObjectCache::AXNotification::AXMinimumValueChanged:
        return kWKAXMinimumValueChanged;
    case AXObjectCache::AXNotification::AXMultiSelectableStateChanged:
        return kWKAXMultiSelectableStateChanged;
    case AXObjectCache::AXNotification::AXOrientationChanged:
        return kWKAXOrientationChanged;
    case AXObjectCache::AXNotification::AXRowCountChanged:
        return kWKAXRowCountChanged;
    case AXObjectCache::AXNotification::AXRowCollapsed:
        return kWKAXRowCollapsed;
    case AXObjectCache::AXNotification::AXRowExpanded:
        return kWKAXRowExpanded;
    case AXObjectCache::AXNotification::AXExpandedChanged:
        return kWKAXExpandedChanged;
    case AXObjectCache::AXNotification::AXInvalidStatusChanged:
        return kWKAXInvalidStatusChanged;
    case AXObjectCache::AXNotification::AXPressDidSucceed:
        return kWKAXPressDidSucceed;
    case AXObjectCache::AXNotification::AXPressDidFail:
        return kWKAXPressDidFail;
    case AXObjectCache::AXNotification::AXPressedStateChanged:
        return kWKAXPressedStateChanged;
    case AXObjectCache::AXNotification::AXReadOnlyStatusChanged:
        return kWKAXReadOnlyStatusChanged;
    case AXObjectCache::AXNotification::AXRequiredStatusChanged:
        return kWKAXRequiredStatusChanged;
    case AXObjectCache::AXNotification::AXSortDirectionChanged:
        return kWKAXSortDirectionChanged;
    case AXObjectCache::AXNotification::AXTextChanged:
        return kWKAXTextChanged;
    case AXObjectCache::AXNotification::AXTextCompositionChanged:
        return kWKAXTextCompositionChanged;
    case AXObjectCache::AXNotification::AXTextSecurityChanged:
        return kWKAXTextSecurityChanged;
    case AXObjectCache::AXNotification::AXElementBusyChanged:
        return kWKAXElementBusyChanged;
    case AXObjectCache::AXNotification::AXDraggingStarted:
        return kWKAXDraggingStarted;
    case AXObjectCache::AXNotification::AXDraggingEnded:
        return kWKAXDraggingEnded;
    case AXObjectCache::AXNotification::AXDraggingEnteredDropZone:
        return kWKAXDraggingEnteredDropZone;
    case AXObjectCache::AXNotification::AXDraggingDropped:
        return kWKAXDraggingDropped;
    case AXObjectCache::AXNotification::AXDraggingExitedDropZone:
        return kWKAXDraggingExitedDropZone;
    }
    ASSERT_NOT_REACHED();
    return kWKAXActiveDescendantChanged;
}

WKAXTextChange toAPI(AXTextChange textChange)
{
    switch (textChange) {
    case AXTextChange::AXTextInserted:
        return kWKAXTextInserted;
    case AXTextChange::AXTextDeleted:
        return kWKAXTextDeleted;
    case AXTextChange::AXTextAttributesChanged:
        return kWKAXTextAttributesChanged;
    }
    ASSERT_NOT_REACHED();
    return kWKAXTextInserted;
}

WKAXLoadingEvent toAPI(AXObjectCache::AXLoadingEvent event)
{
    switch (event) {
    case AXObjectCache::AXLoadingEvent::AXLoadingStarted:
        return kWKAXLoadingStarted;
    case AXObjectCache::AXLoadingEvent::AXLoadingReloaded:
        return kWKAXLoadingReloaded;
    case AXObjectCache::AXLoadingEvent::AXLoadingFailed:
        return kWKAXLoadingFailed;
    case AXObjectCache::AXLoadingEvent::AXLoadingFinished:
        return kWKAXLoadingFinished;
    }
    ASSERT_NOT_REACHED();
    return kWKAXLoadingStarted;
}
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

void WebViewAccessibilityClient::accessibilityNotification(PlayStationWebView* view, WebAccessibilityObject* axObject, AXObjectCache::AXNotification notification)
{
    if (!m_client.notification)
        return;

    m_client.notification(toAPI(view), toAPI(axObject), toAPI(notification), m_client.base.clientInfo);
}

void WebViewAccessibilityClient::accessibilityTextChanged(PlayStationWebView* view, WebAccessibilityObject* axObject, AXTextChange textChange, uint32_t offset, const String& text)
{
    if (!m_client.textChanged)
        return;

    m_client.textChanged(toAPI(view), toAPI(axObject), toAPI(textChange), offset, toAPI(text.impl()), m_client.base.clientInfo);
}

void WebViewAccessibilityClient::accessibilityLoadingEvent(PlayStationWebView* view, WebAccessibilityObject* axObject, AXObjectCache::AXLoadingEvent loadingEvent)
{
    if (!m_client.loadingEvent)
        return;

    m_client.loadingEvent(toAPI(view), toAPI(axObject), toAPI(loadingEvent), m_client.base.clientInfo);
}

void WebViewAccessibilityClient::handleAccessibilityRootObject(PlayStationWebView* view, WebAccessibilityObject* axObject)
{
    if (!m_client.rootObject)
        return;

    m_client.rootObject(toAPI(view), toAPI(axObject), m_client.base.clientInfo);
}

void WebViewAccessibilityClient::handleAccessibilityFocusedObject(PlayStationWebView* view, WebAccessibilityObject* axObject)
{
    if (!m_client.focusedObject)
        return;

    m_client.focusedObject(toAPI(view), toAPI(axObject), m_client.base.clientInfo);
}

void WebViewAccessibilityClient::handleAccessibilityHitTest(PlayStationWebView* view, WebAccessibilityObject* axObject)
{
    if (!m_client.hitTest)
        return;

    m_client.hitTest(toAPI(view), toAPI(axObject), m_client.base.clientInfo);
}

} // namespace WebKit

#endif // ENABLE(ACCESSIBILITY)
