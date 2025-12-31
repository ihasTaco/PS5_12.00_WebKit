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
#include "WKAXObject.h"

#if ENABLE(ACCESSIBILITY)

#include "WKAPICast.h"
#include "WKNumber.h"
#include "WebAccessibilityObject.h"
#include "WebFrameProxy.h"
#include "WebPageProxy.h"

using namespace WebKit;

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic error "-Wswitch"
#endif
WKAXRole toAPI(WebCore::AccessibilityRole role)
{
    switch (role) {
    case WebCore::AccessibilityRole::Annotation:
        return kWKAnnotation;
    case WebCore::AccessibilityRole::Application:
        return kWKApplication;
    case WebCore::AccessibilityRole::ApplicationAlert:
        return kWKApplicationAlert;
    case WebCore::AccessibilityRole::ApplicationAlertDialog:
        return kWKApplicationAlertDialog;
    case WebCore::AccessibilityRole::ApplicationDialog:
        return kWKApplicationDialog;
    case WebCore::AccessibilityRole::ApplicationGroup:
        return kWKApplicationGroup;
    case WebCore::AccessibilityRole::ApplicationLog:
        return kWKApplicationLog;
    case WebCore::AccessibilityRole::ApplicationMarquee:
        return kWKApplicationMarquee;
    case WebCore::AccessibilityRole::ApplicationStatus:
        return kWKApplicationStatus;
    case WebCore::AccessibilityRole::ApplicationTextGroup:
        return kWKApplicationTextGroup;
    case WebCore::AccessibilityRole::ApplicationTimer:
        return kWKApplicationTimer;
    case WebCore::AccessibilityRole::Audio:
        return kWKAudio;
    case WebCore::AccessibilityRole::Blockquote:
        return kWKBlockquote;
    case WebCore::AccessibilityRole::Browser:
        return kWKBrowser;
    case WebCore::AccessibilityRole::BusyIndicator:
        return kWKBusyIndicator;
    case WebCore::AccessibilityRole::Button:
        return kWKButton;
    case WebCore::AccessibilityRole::Canvas:
        return kWKCanvas;
    case WebCore::AccessibilityRole::Caption:
        return kWKCaption;
    case WebCore::AccessibilityRole::Cell:
        return kWKCell;
    case WebCore::AccessibilityRole::CheckBox:
        return kWKCheckBox;
    case WebCore::AccessibilityRole::Code:
        return kWKCode;
    case WebCore::AccessibilityRole::ColorWell:
        return kWKColorWell;
    case WebCore::AccessibilityRole::Column:
        return kWKColumn;
    case WebCore::AccessibilityRole::ColumnHeader:
        return kWKColumnHeader;
    case WebCore::AccessibilityRole::ComboBox:
        return kWKComboBox;
    case WebCore::AccessibilityRole::Definition:
        return kWKDefinition;
    case WebCore::AccessibilityRole::Deletion:
        return kWKDeletion;
    case WebCore::AccessibilityRole::DescriptionList:
        return kWKDescriptionList;
    case WebCore::AccessibilityRole::DescriptionListDetail:
        return kWKDescriptionListDetail;
    case WebCore::AccessibilityRole::DescriptionListTerm:
        return kWKDescriptionListTerm;
    case WebCore::AccessibilityRole::Details:
        return kWKDetails;
    case WebCore::AccessibilityRole::Directory:
        return kWKDirectory;
    case WebCore::AccessibilityRole::DisclosureTriangle:
        return kWKDisclosureTriangle;
    case WebCore::AccessibilityRole::Document:
        return kWKDocument;
    case WebCore::AccessibilityRole::DocumentArticle:
        return kWKDocumentArticle;
    case WebCore::AccessibilityRole::DocumentMath:
        return kWKDocumentMath;
    case WebCore::AccessibilityRole::DocumentNote:
        return kWKDocumentNote;
    case WebCore::AccessibilityRole::Drawer:
        return kWKDrawer;
    case WebCore::AccessibilityRole::EditableText:
        return kWKEditableText;
    case WebCore::AccessibilityRole::Feed:
        return kWKFeed;
    case WebCore::AccessibilityRole::Figure:
        return kWKFigure;
    case WebCore::AccessibilityRole::Footer:
        return kWKFooter;
    case WebCore::AccessibilityRole::Footnote:
        return kWKFootnote;
    case WebCore::AccessibilityRole::Form:
        return kWKForm;
    case WebCore::AccessibilityRole::Generic:
        return kWKDiv; // TODO: fix as part of SUPER-5927
    case WebCore::AccessibilityRole::GraphicsDocument:
        return kWKGraphicsDocument;
    case WebCore::AccessibilityRole::GraphicsObject:
        return kWKGraphicsObject;
    case WebCore::AccessibilityRole::GraphicsSymbol:
        return kWKGraphicsSymbol;
    case WebCore::AccessibilityRole::Grid:
        return kWKGrid;
    case WebCore::AccessibilityRole::GridCell:
        return kWKGridCell;
    case WebCore::AccessibilityRole::Group:
        return kWKGroup;
    case WebCore::AccessibilityRole::GrowArea:
        return kWKGrowArea;
    case WebCore::AccessibilityRole::Heading:
        return kWKHeading;
    case WebCore::AccessibilityRole::HelpTag:
        return kWKHelpTag;
    case WebCore::AccessibilityRole::HorizontalRule:
        return kWKHorizontalRule;
    case WebCore::AccessibilityRole::Ignored:
        return kWKIgnored;
    case WebCore::AccessibilityRole::Inline:
        return kWKInline;
    case WebCore::AccessibilityRole::Image:
        return kWKImage;
    case WebCore::AccessibilityRole::ImageMap:
        return kWKImageMap;
    case WebCore::AccessibilityRole::ImageMapLink:
        return kWKImageMapLink;
    case WebCore::AccessibilityRole::Incrementor:
        return kWKIncrementor;
    case WebCore::AccessibilityRole::Insertion:
        return kWKInsertion;
    case WebCore::AccessibilityRole::Label:
        return kWKLabel;
    case WebCore::AccessibilityRole::LandmarkBanner:
        return kWKLandmarkBanner;
    case WebCore::AccessibilityRole::LandmarkComplementary:
        return kWKLandmarkComplementary;
    case WebCore::AccessibilityRole::LandmarkContentInfo:
        return kWKLandmarkContentInfo;
    case WebCore::AccessibilityRole::LandmarkDocRegion:
        return kWKLandmarkDocRegion;
    case WebCore::AccessibilityRole::LandmarkMain:
        return kWKLandmarkMain;
    case WebCore::AccessibilityRole::LandmarkNavigation:
        return kWKLandmarkNavigation;
    case WebCore::AccessibilityRole::LandmarkRegion:
        return kWKLandmarkRegion;
    case WebCore::AccessibilityRole::LandmarkSearch:
        return kWKLandmarkSearch;
    case WebCore::AccessibilityRole::Legend:
        return kWKLegend;
    case WebCore::AccessibilityRole::Link:
        return kWKLink;
    case WebCore::AccessibilityRole::List:
        return kWKList;
    case WebCore::AccessibilityRole::ListBox:
        return kWKListBox;
    case WebCore::AccessibilityRole::ListBoxOption:
        return kWKListBoxOption;
    case WebCore::AccessibilityRole::ListItem:
        return kWKListItem;
    case WebCore::AccessibilityRole::ListMarker:
        return kWKListMarker;
    case WebCore::AccessibilityRole::Mark:
        return kWKMark;
    case WebCore::AccessibilityRole::MathElement:
        return kWKMathElement;
    case WebCore::AccessibilityRole::Matte:
        return kWKMatte;
    case WebCore::AccessibilityRole::Menu:
        return kWKMenu;
    case WebCore::AccessibilityRole::MenuBar:
        return kWKMenuBar;
    case WebCore::AccessibilityRole::MenuButton:
        return kWKMenuButton;
    case WebCore::AccessibilityRole::MenuItem:
        return kWKMenuItem;
    case WebCore::AccessibilityRole::MenuItemCheckbox:
        return kWKMenuItemCheckbox;
    case WebCore::AccessibilityRole::MenuItemRadio:
        return kWKMenuItemRadio;
    case WebCore::AccessibilityRole::MenuListPopup:
        return kWKMenuListPopup;
    case WebCore::AccessibilityRole::MenuListOption:
        return kWKMenuListOption;
    case WebCore::AccessibilityRole::Meter:
        return kWKMeter;
    case WebCore::AccessibilityRole::Model:
        return kWKModel;
    case WebCore::AccessibilityRole::Outline:
        return kWKOutline;
    case WebCore::AccessibilityRole::Paragraph:
        return kWKParagraph;
    case WebCore::AccessibilityRole::PopUpButton:
        return kWKPopUpButton;
    case WebCore::AccessibilityRole::Pre:
        return kWKPre;
    case WebCore::AccessibilityRole::Presentational:
        return kWKPresentational;
    case WebCore::AccessibilityRole::ProgressIndicator:
        return kWKProgressIndicator;
    case WebCore::AccessibilityRole::RadioButton:
        return kWKRadioButton;
    case WebCore::AccessibilityRole::RadioGroup:
        return kWKRadioGroup;
    case WebCore::AccessibilityRole::RowHeader:
        return kWKRowHeader;
    case WebCore::AccessibilityRole::Row:
        return kWKRow;
    case WebCore::AccessibilityRole::RowGroup:
        return kWKRowGroup;
    case WebCore::AccessibilityRole::RubyBase:
        return kWKRubyBase;
    case WebCore::AccessibilityRole::RubyBlock:
        return kWKRubyBlock;
    case WebCore::AccessibilityRole::RubyInline:
        return kWKRubyInline;
    case WebCore::AccessibilityRole::RubyRun:
        return kWKRubyRun;
    case WebCore::AccessibilityRole::RubyText:
        return kWKRubyText;
    case WebCore::AccessibilityRole::Ruler:
        return kWKRuler;
    case WebCore::AccessibilityRole::RulerMarker:
        return kWKRulerMarker;
    case WebCore::AccessibilityRole::ScrollArea:
        return kWKScrollArea;
    case WebCore::AccessibilityRole::ScrollBar:
        return kWKScrollBar;
    case WebCore::AccessibilityRole::SearchField:
        return kWKSearchField;
    case WebCore::AccessibilityRole::Sheet:
        return kWKSheet;
    case WebCore::AccessibilityRole::Slider:
        return kWKSlider;
    case WebCore::AccessibilityRole::SliderThumb:
        return kWKSliderThumb;
    case WebCore::AccessibilityRole::SpinButton:
        return kWKSpinButton;
    case WebCore::AccessibilityRole::SpinButtonPart:
        return kWKSpinButtonPart;
    case WebCore::AccessibilityRole::SplitGroup:
        return kWKSplitGroup;
    case WebCore::AccessibilityRole::Splitter:
        return kWKSplitter;
    case WebCore::AccessibilityRole::StaticText:
        return kWKStaticText;
    case WebCore::AccessibilityRole::Subscript:
        return kWKSubscript;
    case WebCore::AccessibilityRole::Suggestion:
        return kWKSuggestion;
    case WebCore::AccessibilityRole::Summary:
        return kWKSummary;
    case WebCore::AccessibilityRole::Superscript:
        return kWKSuperscript;
    case WebCore::AccessibilityRole::Switch:
        return kWKSwitch;
    case WebCore::AccessibilityRole::SystemWide:
        return kWKSystemWide;
    case WebCore::AccessibilityRole::SVGRoot:
        return kWKSVGRoot;
    case WebCore::AccessibilityRole::SVGText:
        return kWKSVGText;
    case WebCore::AccessibilityRole::SVGTSpan:
        return kWKSVGTSpan;
    case WebCore::AccessibilityRole::SVGTextPath:
        return kWKSVGTextPath;
    case WebCore::AccessibilityRole::TabGroup:
        return kWKTabGroup;
    case WebCore::AccessibilityRole::TabList:
        return kWKTabList;
    case WebCore::AccessibilityRole::TabPanel:
        return kWKTabPanel;
    case WebCore::AccessibilityRole::Tab:
        return kWKTab;
    case WebCore::AccessibilityRole::Table:
        return kWKTable;
    case WebCore::AccessibilityRole::TableHeaderContainer:
        return kWKTableHeaderContainer;
    case WebCore::AccessibilityRole::Term:
        return kWKTerm;
    case WebCore::AccessibilityRole::TextArea:
        return kWKTextArea;
    case WebCore::AccessibilityRole::TextField:
        return kWKTextField;
    case WebCore::AccessibilityRole::TextGroup:
        return kWKTextGroup;
    case WebCore::AccessibilityRole::Time:
        return kWKTime;
    case WebCore::AccessibilityRole::Tree:
        return kWKTree;
    case WebCore::AccessibilityRole::TreeGrid:
        return kWKTreeGrid;
    case WebCore::AccessibilityRole::TreeItem:
        return kWKTreeItem;
    case WebCore::AccessibilityRole::ToggleButton:
        return kWKToggleButton;
    case WebCore::AccessibilityRole::Toolbar:
        return kWKToolbar;
    case WebCore::AccessibilityRole::Unknown:
        return kWKUnknown;
    case WebCore::AccessibilityRole::UserInterfaceTooltip:
        return kWKUserInterfaceTooltip;
    case WebCore::AccessibilityRole::ValueIndicator:
        return kWKValueIndicator;
    case WebCore::AccessibilityRole::Video:
        return kWKVideo;
    case WebCore::AccessibilityRole::WebApplication:
        return kWKWebApplication;
    case WebCore::AccessibilityRole::WebArea:
        return kWKWebArea;
    case WebCore::AccessibilityRole::WebCoreLink:
        return kWKWebCoreLink;
    case WebCore::AccessibilityRole::Window:
        return kWKWindow;
    }

    ASSERT_NOT_REACHED();
    return kWKAnnotation;
}

WKAXButtonState toAPI(WebCore::AccessibilityButtonState state)
{
    switch (state) {
    case WebCore::AccessibilityButtonState::Off:
        return kWKButtonStateOff;
    case WebCore::AccessibilityButtonState::On:
        return kWKButtonStateOn;
    case WebCore::AccessibilityButtonState::Mixed:
        return kWKButtonStateMixed;
    }
    ASSERT_NOT_REACHED();
    return kWKButtonStateOff;
}
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

WKTypeID WKAXObjectGetTypeID()
{
    return toAPI(WebAccessibilityObject::APIType);
}

WKAXRole WKAXObjectRole(WKAXObjectRef axObjectref)
{
    return toAPI(toImpl(axObjectref)->role());
}

WKStringRef WKAXObjectCopyTitle(WKAXObjectRef axObjectref)
{
    return toCopiedAPI(toImpl(axObjectref)->title());
}

WKStringRef WKAXObjectCopyDescription(WKAXObjectRef axObjectref)
{
    return toCopiedAPI(toImpl(axObjectref)->description());
}

WKStringRef WKAXObjectCopyHelpText(WKAXObjectRef axObjectref)
{
    return toCopiedAPI(toImpl(axObjectref)->helpText());
}

WKStringRef WKAXObjectCopyURL(WKAXObjectRef axObjectref)
{
    return toCopiedAPI(toImpl(axObjectref)->url().string());
}

WKAXButtonState WKAXObjectButtonState(WKAXObjectRef axObjectref)
{
    return toAPI(toImpl(axObjectref)->buttonState());
}

WKPageRef WKAXObjectPage(WKAXObjectRef axObjectref)
{
    return toAPI(toImpl(axObjectref)->page());
}

WKFrameRef WKAXObjectFrame(WKAXObjectRef axObjectref)
{
    return toAPI(toImpl(axObjectref)->frame());
}

WKRectRef WKAXObjectRect(WKAXObjectRef axObjectref)
{
    return WKRectCreate(toAPI(toImpl(axObjectref)->rect()));
}

WKStringRef WKAXObjectCopyValue(WKAXObjectRef axObjectref)
{
    return toCopiedAPI(toImpl(axObjectref)->value());
}

WKBooleanRef WKAXObjectIsFocused(WKAXObjectRef axObjectref)
{
    return WKBooleanCreate(toImpl(axObjectref)->isFocused());
}

WKBooleanRef WKAXObjectIsDisabled(WKAXObjectRef axObjectref)
{
    return WKBooleanCreate(toImpl(axObjectref)->isDisabled());
}

WKBooleanRef WKAXObjectIsSelected(WKAXObjectRef axObjectref)
{
    return WKBooleanCreate(toImpl(axObjectref)->isSelected());
}

WKBooleanRef WKAXObjectIsVisited(WKAXObjectRef axObjectref)
{
    return WKBooleanCreate(toImpl(axObjectref)->isVisited());
}

WKBooleanRef WKAXObjectIsLinked(WKAXObjectRef axObjectref)
{
    return WKBooleanCreate(toImpl(axObjectref)->isLinked());
}

#endif // ENABLE(ACCESSIBILITY)
