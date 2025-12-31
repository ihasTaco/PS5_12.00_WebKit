/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
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
#include "WebPage.h"

#include "EditorState.h"
#include "MessageSenderInlines.h"
#include "WKImeEvent.h"
#include "WebCoreArgumentCoders.h"
#include "WebEventConversion.h"
#include "WebFrame.h"
#include "WebKeyboardEvent.h"
#include "WebKitVersion.h"
#include "WebPageInlines.h"
#include "WebPageProxyMessages.h"
#include "WebPreferencesKeys.h"
#include "WebPreferencesStore.h"
#include <KeyboardCodes.h>
#include <WebCore/AXObjectCache.h>
#include <WebCore/CompositionHighlight.h>
#include <WebCore/CursorNavigation.h>
#include <WebCore/Editor.h>
#include <WebCore/Element.h>
#include <WebCore/EventNames.h>
#include <WebCore/FocusController.h>
#include <WebCore/FrameSelection.h>
#include <WebCore/GeometryUtilities.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/LocalFrame.h>
#include <WebCore/LocalFrameView.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/PointerCharacteristics.h>
#include <WebCore/Range.h>
#include <WebCore/RenderElement.h>
#include <WebCore/RenderObject.h>
#include <WebCore/ScriptDisallowedScope.h>
#include <WebCore/Settings.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/TextIterator.h>
#include <WebCore/UserAgent.h>

using namespace WebCore;

namespace WebKit {

void WebPage::platformInitialize(const WebPageCreationParameters&)
{
#if ENABLE(CURSOR_NAVIGATION)
    m_page->settings().setCursorNavigationEnabled(true);
#endif
#if ENABLE(VIDEO)
    m_page->settings().setMediaControlsScaleWithPageZoom(false);
    m_page->settings().setMediaEnabled(true);
#if ENABLE(MEDIA_SOURCE)
    {
        // const int platformMediaPlayerBufferSizeMiBFor2K = 48;
        // const int oneAppendBufferSizeFor2K5SecMiB = 6; // 1080p30fps5sec=5MiB + margin20%
        const int platformMediaPlayerBufferSizeMiBFor4K = 150; // not determined yet
        // const int oneAppendBufferSizeFor4K5SecMiB  = 30; // 2160p30fpx5sec=25MiB + margin20%
        m_page->settings().setMaximumSourceBufferSize(platformMediaPlayerBufferSizeMiBFor4K*1024*1024);
    }
#endif
#endif
    m_page->settings().setMediaCapabilitiesEnabled(false);
}

void WebPage::platformReinitialize()
{
}

void WebPage::platformDetach()
{
}

void WebPage::getPlatformEditorState(LocalFrame& frame, EditorState& state) const
{
    state.fieldType = InputFieldTypeUndefined;
    state.fieldLang = InputLanguageUndefined;

    Element* element = frame.document()->focusedElement();
    if (!element || !element->renderer())
        state.isContentEditable = false;

    if (!state.isContentEditable)
        return;

    state.fieldRect = frame.view()->contentsToWindow(static_cast<Node*>(element)->renderer()->absoluteBoundingBoxRect());

    auto str = element->effectiveLang();
    if (!str.isEmpty())
        state.fieldLang = WKImeEventGetInputLanguage(str.string().utf8().data());
    if (element->hasAttribute("x-ps-osk"_s))
        state.psOSKAttr = WTF::String(element->getAttribute("x-ps-osk"_s).string());

    state.hasPreviousNode = m_page->focusController().hasPreviousEditableElement(element);
    state.hasNextNode = m_page->focusController().hasNextEditableElement(element);
    Element* selectionRoot = frame.selection().selection().rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : frame.document()->documentElement();

    if (element->hasTagName(HTMLNames::inputTag)) {
        state.fieldText = scope->textContent();
        HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
        if (input->isTextField()) {
            if (input->isPasswordField())
                state.fieldType = INPUT_FIELD_PASSWORD;
            else if (input->isEmailField())
                state.fieldType = INPUT_FIELD_EMAIL;
            else if (input->isURLField())
                state.fieldType = INPUT_FIELD_URL;
            else if (input->isSearchField())
                state.fieldType = INPUT_FIELD_SEARCH;
            else if (input->isNumberField())
                state.fieldType = INPUT_FIELD_NUMBER;
            else if (input->isTelephoneField())
                state.fieldType = INPUT_FIELD_TELEPHONE;
            else
                state.fieldType = INPUT_FIELD_TEXT;
            HTMLFormElement* form = input->form();
            if (form) {
                if (form->action().contains("login"_s))
                    state.isInLoginForm = true;
                // the following code should be moved more appropriate place.
                {
                    bool canSubmitImplicitly = false;
                    int submissionTriggerCount = 0;
                    ScriptDisallowedScope::InMainThread scriptDisallowedScope;
                    auto& associatedElements = form->unsafeListedElements();

                    for (unsigned i = 0; i < associatedElements.size(); ++i) {
                        auto* associatedElement = associatedElements[i]->asFormListedElement();
                        ASSERT(associatedElement);
                        if (!associatedElement->isFormControlElement())
                            continue;
                        HTMLFormControlElement* formElement = static_cast<HTMLFormControlElement*>(associatedElement);
                        if (!formElement->isDisabledFormControl() && formElement->isSuccessfulSubmitButton()) {
                            if (formElement->renderer()) {
                                canSubmitImplicitly = true;
                                break;
                            }
                        }
                        if (formElement->canTriggerImplicitSubmission())
                            ++submissionTriggerCount;
                    }
                    if (submissionTriggerCount == 1)
                        canSubmitImplicitly = true;

                    state.canSubmitImplicitly = canSubmitImplicitly;
                }

                // workaround for WEBBROWSER-968
                if (mainWebFrame().url().string().startsWith("https://discord.com/register"_s)) {
                    // If the next editable element is input with type="password", set "basic-latin" to x-ps-osk forcibly as workaround.
                    Element* nextElement = m_page->focusController().nextEditableElement(element);
                    if (nextElement && is<HTMLInputElement>(nextElement)) {
                        auto nextInput = downcast<HTMLInputElement>(nextElement);
                        if (nextInput->isPasswordField())
                            state.psOSKAttr = "basic-latin"_s;
                    }
                }

                // workaround for WEBBROWSER-976
                if (mainWebFrame().url().string().startsWith("https://discord.com/login"_s)) {
                    unsigned numOfCodeInputs = 0;
                    auto& elements = form->unsafeListedElements();
                    for (unsigned i = 0; i < elements.size(); i++) {
                        RefPtr element = elements[i].get();
                        ASSERT(element && element->asFormListedElement());

                        if (!is<HTMLInputElement>(element))
                            continue;
                        auto& input = downcast<HTMLInputElement>(*element);
                        // If there is even one input field with maxlength is not 1, this is not Discord's code form.
                        if (input.effectiveMaxLength() != 1) {
                            numOfCodeInputs = 0;
                            break;
                        }
                        numOfCodeInputs++;
                    }
                    // If 6 inputs with maxlength is 1 are found in the form, set "basic-latin" to x-ps-osk forcibly as workaround.
                    if (numOfCodeInputs == 6)
                        state.psOSKAttr = "basic-latin"_s;
                }
            }
        } else
            state.fieldType = InputFieldTypeUndefined;
    } else {
        state.fieldText = makeStringByReplacingAll(scope->innerText(), (UChar)0xA0, (UChar)0x20);
        if (element->hasTagName(HTMLNames::textareaTag))
            state.fieldType = INPUT_FIELD_TEXTAREA;
        else
            state.fieldType = INPUT_FIELD_OTHER;
    }

    std::optional<SimpleRange> range = std::nullopt;
    if (state.selectionIsRange && !state.shouldIgnoreSelectionChanges && (range = frame.selection().selection().toNormalizedRange())) {
        state.selectedRect = frame.view()->contentsToWindow(unionRect(RenderObject::absoluteTextRects(*range)));
        range->start.document().updateLayout();
        state.selectedText = plainText(*range);
    }

    range = std::nullopt;
    if (!state.selectionIsRange && state.hasComposition && (range = frame.editor().compositionRange()))
        state.compositionRect = frame.view()->contentsToWindow(unionRect(RenderObject::absoluteTextRects(*range)));

    range = std::nullopt;
    if (!state.shouldIgnoreSelectionChanges && !state.selectionIsNone && (range = frame.selection().selection().firstRange())) {
        auto relativeRange = characterRange(makeBoundaryPointBeforeNodeContents(*scope), *range);
        bool baseIsFirst = frame.selection().selection().isBaseFirst();
        state.caretOffset = (baseIsFirst) ? relativeRange.location + relativeRange.length : relativeRange.location;
    } else
        state.caretOffset = -1;
}

static const unsigned CtrlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;

struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft" },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection" },
    { VK_RIGHT,  0,                  "MoveRight" },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection" },
    { VK_UP,     0,                  "MoveUp" },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection" },
    { VK_DOWN,   0,                  "MoveDown" },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection" },
    { VK_PRIOR,  0,                  "MovePageUp" },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection" },
    { VK_NEXT,   0,                  "MovePageDown" },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection" },
    { VK_HOME,   0,                  "MoveToBeginningOfLine" },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection" },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument" },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },
    { VK_END,    0,                  "MoveToEndOfLine" },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection" },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument" },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection" },
    { VK_BACK,   0,                  "DeleteBackward" },
    { VK_DELETE, 0,                  "DeleteForward" },
    { VK_ESCAPE, 0,                  "Cancel" },
    { VK_TAB,    0,                  "InsertTab" },
    { VK_RETURN, 0,                  "InsertNewline" },
    { 'A',             CtrlKey,      "SelectAll" },

    { VKX_PS_CARET_LEFT,  0,        "MoveLeft" },
    { VKX_PS_CARET_LEFT,  ShiftKey, "MoveLeftAndModifySelection" },
    { VKX_PS_CARET_RIGHT, 0,        "MoveRight" },
    { VKX_PS_CARET_RIGHT, ShiftKey, "MoveRightAndModifySelection" },
    { VKX_PS_CARET_UP,    0,        "MoveUp" },
    { VKX_PS_CARET_UP,    ShiftKey, "MoveUpAndModifySelection" },
    { VKX_PS_CARET_DOWN,  0,        "MoveDown" },
    { VKX_PS_CARET_DOWN,  ShiftKey, "MoveDownAndModifySelection" },

    { VKX_PS_TAB_JUMP,    0,        "TabJumpForward" },
    { VKX_PS_TAB_JUMP,    ShiftKey, "TabJumpBackword" },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',            0,           "InsertTab" },
    { '\r',            0,           "InsertNewline" },
};

static bool isValidAXObject(AXCoreObject* obj)
{
    if (obj->isLandmark())
        return false;

    switch (obj->roleValue()) {
    case AccessibilityRole::Generic:
    case AccessibilityRole::Paragraph:
        return false;
    default:
        return true;
    }
}

static AXCoreObject* searchValidAXObject(AXCoreObject* root, const IntPoint& point)
{
    if (!root)
        return nullptr;

    Vector<AXCoreObject*> childrenList;
    childrenList.append(root);

    // If there is a hit and the role is not valid, search for child elements.
    while (!childrenList.isEmpty()) {
        AXCoreObject* search = childrenList.first();
        childrenList.remove(0);

        if (isValidAXObject(search))
            return search;

        // Search for an element by searching its children in reverse order.
        // To reduce processing time, select child elements at the cursor position.
        for (auto it = search->children().rbegin(); it != search->children().rend(); it++)
            if ((*it)->elementRect().contains(point))
                childrenList.append((*it).get());
    }

    return nullptr;
}

const char* WebPage::interpretKeyEvent(const KeyboardEvent* evt)
{
    ASSERT(evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);

    static NeverDestroyed<HashMap<int, const char*>> keyDownCommandsMap;
    static NeverDestroyed<HashMap<int, const char*>> keyPressCommandsMap;

    if (keyDownCommandsMap.get().isEmpty()) {
        for (unsigned i = 0; i < std::size(keyDownEntries); i++)
            keyDownCommandsMap.get().set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);
        for (unsigned i = 0; i < std::size(keyPressEntries); i++)
            keyPressCommandsMap.get().set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
    }

    unsigned modifiers = 0;
    if (evt->shiftKey())
        modifiers |= ShiftKey;
    if (evt->altKey())
        modifiers |= AltKey;
    if (evt->ctrlKey())
        modifiers |= CtrlKey;

    if (evt->type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | evt->keyCode();
        return mapKey ? keyDownCommandsMap.get().get(mapKey) : nullptr;
    }

    int mapKey = modifiers << 16 | evt->charCode();
    return mapKey ? keyPressCommandsMap.get().get(mapKey) : nullptr;
}

bool WebPage::platformCanHandleRequest(const ResourceRequest&)
{
    notImplemented();
    return false;
}

String WebPage::platformUserAgent(const URL& url) const
{
    if (url.isNull() || !m_page->settings().needsSiteSpecificQuirks())
        return emptyString();

    return WebCore::standardUserAgentForURL(url);
}

bool WebPage::hoverSupportedByPrimaryPointingDevice() const
{
    return true;
}

bool WebPage::hoverSupportedByAnyAvailablePointingDevice() const
{
    return true;
}

std::optional<PointerCharacteristics> WebPage::pointerCharacteristicsOfPrimaryPointingDevice() const
{
    return PointerCharacteristics::Fine;
}

OptionSet<PointerCharacteristics> WebPage::pointerCharacteristicsOfAllAvailablePointingDevices() const
{
    return PointerCharacteristics::Fine;
}

void WebPage::keyEventPlayStation(const WebKeyboardEvent& event, int caretOffset)
{
    bool handled = false;

    if (caretOffset == -1)
        caretOffset = event.text().length();

    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();
    if (frame.editor().canEdit()) {
        Element* element = frame.document()->focusedElement();
        if (element) {
            if (event.keyIdentifier() == "TabJump"_s) {
                if (frame.editor().hasComposition())
                    frame.editor().cancelComposition();
                moveFocusInTabOrder(!event.shiftKey());
                handled = true;
            } else if (event.keyIdentifier() == "Accept"_s) {
                frame.editor().confirmComposition(event.text(), caretOffset);
                handled = true;
            } else if (event.keyIdentifier() == "Convert"_s) {
                ASSERT(caretOffset >= 0 && caretOffset < std::numeric_limits<int>::max());
                auto caretPosition = static_cast<unsigned>(caretOffset);

                Vector<CompositionUnderline> underlines;
                underlines.append(CompositionUnderline(0, caretPosition, CompositionUnderlineColor::TextColor, Color(Color::black), true));
                if (event.text().length() > caretPosition)
                    underlines.append(CompositionUnderline(caretPosition, (unsigned)event.text().length(), CompositionUnderlineColor::TextColor, Color(Color::black), false));
                frame.editor().setComposition(event.text(), underlines, { }, { }, caretOffset, caretOffset);
                handled = true;
            } else if (event.keyIdentifier() == "Cancel"_s) {
                frame.editor().cancelComposition();
                handled = true;
            }
        }
    }

    if (handled)
        send(Messages::WebPageProxy::DidReceiveEvent(event.type(), true));
    else
        keyEvent(event);
}

void WebPage::setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t, uint64_t)
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();
    if (!frame.editor().canEdit())
        return;

    frame.editor().setComposition(text, underlines, { }, { }, selectionStart, selectionEnd);
}

void WebPage::confirmComposition(const String& text, int64_t selectionStart, int64_t)
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();
    if (!frame.editor().canEdit())
        return;

    frame.editor().confirmComposition(text, selectionStart);
}

void WebPage::exitComposition()
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();

    if (frame.editor().hasComposition())
        frame.editor().cancelComposition();

    Node* node = frame.document()->focusedElement();
    if (!node || !node->isElementNode())
        return;

    static_cast<Element*>(node)->blur();
}

void WebPage::setValueToFocusedNode(const String& value)
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();
    if (!frame.editor().canEdit())
        return;

    if (frame.editor().hasComposition())
        frame.editor().cancelComposition();

    Element* element = frame.document()->focusedElement();
    if (!element)
        return;
    if (is<HTMLInputElement>(element)) {
        downcast<HTMLInputElement>(element)->setValue(value, WebCore::DispatchInputAndChangeEvent);
    } else if (is<HTMLTextAreaElement>(element)) {
        downcast<HTMLTextAreaElement>(element)->setValue(value);
        element->dispatchInputEvent();
    } else {
        element->setTextContent(String { value });
        element->dispatchInputEvent();
    }
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
}

void WebPage::moveFocusInTabOrder(bool forward)
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();

    Element* node = frame.document()->focusedElement();
    if (!node)
        return;

    Element* nodeNext;
    if (forward)
        nodeNext = m_page->focusController().nextEditableElement(node);
    else
        nodeNext = m_page->focusController().previousEditableElement(node);

    if (nodeNext) {
        nodeNext->dispatchEvent(Event::create(eventNames().selectstartEvent, Event::CanBubble::Yes, Event::IsCancelable::Yes));
        LocalFrameView* view = frame.document()->view();
        IntPoint oldPosition = view->scrollPosition();
        view->scrollElementToRect(*nodeNext, bounds());
        IntPoint newPosition = view->scrollPosition();
        view->scrollOffsetChangedViaPlatformWidget(oldPosition, newPosition);
        frame.document()->setFocusedElement(nodeNext);

        if (is<HTMLInputElement>(nodeNext)) {
            HTMLInputElement* element = downcast<HTMLInputElement>(nodeNext);
            element->setSelectionRange(0, element->value().length());
        } else if (is<HTMLTextAreaElement>(nodeNext)) {
            HTMLTextAreaElement* element = downcast<HTMLTextAreaElement>(nodeNext);
            element->setSelectionRange(0, 0);
        } else {
            VisibleSelection newSelection(firstPositionInNode(nodeNext), firstPositionInNode(nodeNext));
            if (frame.selection().shouldChangeSelection(newSelection))
                frame.selection().setSelection(newSelection);
        }
    }
}

void WebPage::clearSelectionWithoutBlur()
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();
    frame.selection().setBase(frame.selection().selection().extent(), frame.selection().selection().affinity());
}

void WebPage::setCaretVisible(bool visible)
{
    LocalFrame& frame = m_page->focusController().focusedOrMainFrame();
    frame.selection().setCaretVisible(visible);
}

bool WebPage::handleEditingKeyboardEvent(KeyboardEvent& evt)
{
    LocalFrame* frame = downcast<Node>(*evt.target()).document().frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* keyEvent = evt.underlyingPlatformEvent();
    if (!keyEvent)
        return false;

    Editor::Command command = frame->editor().command(String::fromUTF8(interpretKeyEvent(&evt)));

    if (keyEvent->type() == PlatformEvent::Type::RawKeyDown) {
        // WebKit doesn't have enough information about mode to decide how commands that just insert text if executed via Editor should be treated,
        // so we leave it upon WebCore to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        return !command.isTextInsertion() && command.execute(&evt);
    }

    if (command.execute(&evt))
        return true;

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor().canEdit())
        return false;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (evt.charCode() < ' ')
        return false;

    return frame->editor().insertText(evt.underlyingPlatformEvent()->text(), &evt);
}

#if ENABLE(ACCESSIBILITY)
void WebPage::accessibilityRootObject()
{
    if (!WebCore::AXObjectCache::accessibilityEnabled())
        return;

    WebCore::Page* page = corePage();
    if (!page)
        return;

    auto* core = dynamicDowncast<WebCore::LocalFrame>(page->mainFrame());
    if (!core)
        return;

    auto* document = core->document();
    if (!document)
        return;

    WebCore::AXCoreObject* root = document->axObjectCache()->rootObject();
    if (!root || root->isDetached())
        return;

    // FIXME: It's better to write AX implementation in WebCore/accessbility, instead of here.
    //        (e.g. WebCore::AXCoreObject or WebCore::AXObjectCache)
    document->updateStyleIfNeeded();
    send(Messages::WebPageProxy::DidReceiveAccessibilityRootObject(WebAccessibilityObjectData(root)));
}

void WebPage::accessibilityFocusedObject()
{
    if (!WebCore::AXObjectCache::accessibilityEnabled())
        return;

    WebCore::Page* page = corePage();
    if (!page)
        return;

    auto* core = dynamicDowncast<WebCore::LocalFrame>(page->mainFrame());
    if (!core)
        return;

    auto* document = core->document();
    if (!document)
        return;

    WebCore::AXCoreObject* focused = document->axObjectCache()->focusedObjectForPage(page);
    if (!focused || focused->isDetached())
        return;

    // FIXME: It's better to write AX implementation in WebCore/accessbility, instead of here.
    //        (e.g. WebCore::AXCoreObject or WebCore::AXObjectCache)
    document->updateStyleIfNeeded();
    send(Messages::WebPageProxy::DidReceiveAccessibilityFocusedObject(WebAccessibilityObjectData(focused)));
}

void WebPage::accessibilityHitTest(const IntPoint& point)
{
    if (!WebCore::AXObjectCache::accessibilityEnabled())
        return;

    WebCore::Page* page = corePage();
    if (!page)
        return;

    auto* core = dynamicDowncast<WebCore::LocalFrame>(page->mainFrame());
    if (!core)
        return;

    auto* document = core->document();
    if (!document)
        return;

    WebCore::AXCoreObject* root = document->axObjectCache()->rootObject();
    if (!root)
        return;

    IntPoint contentsPoint = root->documentFrameView()->windowToContents(point);
    AXCoreObject* axObjectInterface = root->accessibilityHitTest(contentsPoint);
    if (!axObjectInterface || !axObjectInterface->isAccessibilityObject())
        return;

    AXCoreObject* axObject = static_cast<AXCoreObject*>(axObjectInterface);
    if (!axObject || axObject->isDetached())
        return;

    // It searches through the child elements for a valid AX object.
    // It does not do this if the AX object is already valid.
    if (!isValidAXObject(axObject)) {
        axObject = searchValidAXObject(axObject, contentsPoint);
        // If not found, restore
        if (!axObject)
            axObject = static_cast<AXCoreObject*>(axObjectInterface);
    }

    // FIXME: It's better to write AX implementation in WebCore/accessbility, instead of here.
    //        (e.g. WebCore::AXCoreObject or WebCore::AXObjectCache)
    document->updateStyleIfNeeded();
    send(Messages::WebPageProxy::DidReceiveAccessibilityHitTest(WebAccessibilityObjectData(axObject)));
}
#endif

} // namespace WebKit
