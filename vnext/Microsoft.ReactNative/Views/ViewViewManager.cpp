// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ViewViewManager.h"
#include <cdebug.h>

#include "ViewControl.h"

#include <UI.Xaml.Automation.Peers.h>
#include "DynamicAutomationProperties.h"

#include <JSValueWriter.h>
#include <Modules/NativeUIManager.h>
#include <Modules/PaperUIManagerModule.h>
#include <Utils/AccessibilityUtils.h>
#include <Utils/PropertyUtils.h>

#include <INativeUIManager.h>
#include <IReactInstance.h>

#include <cxxreact/TraceSection.h>
#include <inspectable.h>
#include <unicode.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winstring.h>

#if defined(_DEBUG)
// Currently only used for tagging controls in debug
#include <winrt/Windows.Foundation.h>
#endif

using namespace facebook::react;

namespace Microsoft::ReactNative {

// ViewShadowNode

class ViewShadowNode : public ShadowNodeBase {
  using Super = ShadowNodeBase;

 public:
  ViewShadowNode() = default;

  void createView(const winrt::Microsoft::ReactNative::JSValueObject &props) override {
    Super::createView(props);

    auto panel = GetPanel();
    IsAccessible(false);
    IsFocusable(false);

    DynamicAutomationProperties::SetAccessibilityInvokeEventHandler(panel, [=]() {
      if (OnClick())
        DispatchEvent("topClick", std::move(folly::dynamic::object("target", m_tag)));
      else
        DispatchEvent("topAccessibilityTap", std::move(folly::dynamic::object("target", m_tag)));
    });

    DynamicAutomationProperties::SetAccessibilityActionEventHandler(
        panel, [=](winrt::Microsoft::ReactNative::AccessibilityAction const &action) {
          folly::dynamic eventData = folly::dynamic::object("target", m_tag);

          eventData.insert(
              "actionName", action.Label.empty() ? HstringToDynamic(action.Name) : HstringToDynamic(action.Label));

          DispatchEvent("topAccessibilityAction", std::move(eventData));
        });
  }

  void dispatchCommand(const std::string &commandId, winrt::Microsoft::ReactNative::JSValueArray &&commandArgs) {
    if (commandId == "focus") {
      if (auto uiManager = GetNativeUIManager(GetViewManager()->GetReactContext()).lock()) {
        uiManager->focus(m_tag);
      }
    } else if (commandId == "blur") {
      if (auto uiManager = GetNativeUIManager(GetViewManager()->GetReactContext()).lock()) {
        uiManager->blur(m_tag);
      }
    } else {
      Super::dispatchCommand(commandId, std::move(commandArgs));
    }
  }

  bool IsControl() {
    return m_isControl;
  }
  void IsControl(bool isControl) {
    m_isControl = isControl;
  }

  bool EnableFocusRing() {
    return m_enableFocusRing;
  }
  void EnableFocusRing(bool enable) {
    m_enableFocusRing = enable;

    if (IsControl())
      GetControl().UseSystemFocusVisuals(m_enableFocusRing);
  }

  int32_t TabIndex() {
    return m_tabIndex;
  }
  void TabIndex(int32_t tabIndex) {
    m_tabIndex = tabIndex;

    if (IsControl())
      GetControl().TabIndex(m_tabIndex);
  }

  bool OnClick() const {
    return m_onClick;
  }
  void OnClick(bool isSet) {
    m_onClick = isSet;
  }

  bool IsHitTestBrushRequired() const {
    return IsRegisteredForMouseEvents();
  }

  void AddView(ShadowNode &child, int64_t index) override {
    const auto &view = static_cast<ShadowNodeBase &>(child).GetView();
    if (view.try_as<xaml::UIElement>() == nullptr) {
      const auto &ii = view.as<winrt::IInspectable>();
      auto name = winrt::get_class_name(ii);
      YellowBox(
          std::string("ViewViewManager::AddView expected a UIElement but got a ") +
          Microsoft::Common::Unicode::Utf16ToUtf8(name.c_str()));
    }

    GetPanel().Children().InsertAt(static_cast<uint32_t>(index), view.as<xaml::UIElement>());
  }

  void RemoveChildAt(int64_t indexToRemove) override {
    if (indexToRemove == static_cast<uint32_t>(indexToRemove))
      GetPanel().Children().RemoveAt(static_cast<uint32_t>(indexToRemove));
  }

  void removeAllChildren() override {
    GetPanel().Children().Clear();

    XamlView current = m_view;

    // TODO NOW: Why do we do this? Removal of children doesn't seem to imply we
    // tear down the infrastructure
    if (IsControl()) {
      if (auto control = m_view.try_as<xaml::Controls::ContentControl>()) {
        current = control.Content().as<XamlView>();
        control.Content(nullptr);
      } else {
        std::string name = Microsoft::Common::Unicode::Utf16ToUtf8(winrt::get_class_name(current).c_str());
        cdebug << "Tearing down, IsControl=true but the control is not a ContentControl, it's a " << name << "\n";
      }
    }
  }

  void ReplaceChild(const XamlView &oldChildView, const XamlView &newChildView) override {
    auto pPanel = GetPanel();
    if (pPanel != nullptr) {
      uint32_t index;
      if (pPanel.Children().IndexOf(oldChildView.as<xaml::UIElement>(), index)) {
        pPanel.Children().RemoveAt(index);
        pPanel.Children().InsertAt(index, newChildView.as<xaml::UIElement>());
      } else {
        assert(false);
      }
    }
  }

  void RefreshProperties() {
    // The view may have been replaced, so transfer properties stored on the
    // shadow node to the view
    EnableFocusRing(EnableFocusRing());
    TabIndex(TabIndex());
    IsFocusable(IsFocusable());
    static_cast<FrameworkElementViewManager *>(GetViewManager())->RefreshTransformMatrix(this);
  }

  xaml::Controls::Panel GetPanel() {
    XamlView current = m_view;

    if (IsControl()) {
      if (auto control = m_view.try_as<xaml::Controls::ContentControl>()) {
        current = control.Content().as<XamlView>();
      }
    }

    auto panel = current.try_as<xaml::Controls::Panel>();
    assert(panel != nullptr);

    return panel;
  }

  winrt::Microsoft::ReactNative::ViewControl GetControl() {
    return IsControl() ? m_view.as<winrt::Microsoft::ReactNative::ViewControl>() : nullptr;
  }

  XamlView CreateViewControl() {
    auto contentControl = winrt::make<winrt::Microsoft::ReactNative::implementation::ViewControl>();

    m_contentControlGotFocusRevoker = contentControl.GotFocus(winrt::auto_revoke, [=](auto &&, auto &&args) {
      if (args.OriginalSource().try_as<xaml::UIElement>() == contentControl.as<xaml::UIElement>()) {
        auto tag = m_tag;
        DispatchEvent("topFocus", std::move(folly::dynamic::object("target", tag)));
      }
    });

    m_contentControlLostFocusRevoker = contentControl.LostFocus(winrt::auto_revoke, [=](auto &&, auto &&args) {
      if (args.OriginalSource().try_as<xaml::UIElement>() == contentControl.as<xaml::UIElement>()) {
        auto tag = m_tag;
        DispatchEvent("topBlur", std::move(folly::dynamic::object("target", tag)));
      }
    });

    return contentControl.try_as<XamlView>();
  }

  void DispatchEvent(std::string &&eventName, folly::dynamic &&eventData) {
    GetViewManager()->GetReactContext().DispatchEvent(m_tag, std::move(eventName), std::move(eventData));
  }

 private:
  bool m_isControl = false;
  bool m_enableFocusRing = true;
  bool m_onClick = false;
  int32_t m_tabIndex = std::numeric_limits<std::int32_t>::max();

  xaml::Controls::ContentControl::GotFocus_revoker m_contentControlGotFocusRevoker{};
  xaml::Controls::ContentControl::LostFocus_revoker m_contentControlLostFocusRevoker{};
};

// ViewViewManager

ViewViewManager::ViewViewManager(const Mso::React::IReactContext &context) : Super(context) {}

const wchar_t *ViewViewManager::GetName() const {
  return L"RCTView";
}

void ViewViewManager::GetExportedCustomDirectEventTypeConstants(
    const winrt::Microsoft::ReactNative::IJSValueWriter &writer) const {
  Super::GetExportedCustomDirectEventTypeConstants(writer);

  writer.WritePropertyName(L"topAccessibilityTap");
  writer.WriteObjectBegin();
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"registrationName", L"onAccessibilityTap");
  writer.WriteObjectEnd();
}

ShadowNode *ViewViewManager::createShadow() const {
  return new ViewShadowNode();
}

XamlView ViewViewManager::CreateViewCore(int64_t /*tag*/, const winrt::Microsoft::ReactNative::JSValueObject &) {
  auto panel = winrt::make<winrt::Microsoft::ReactNative::implementation::ViewPanel>();
  panel.VerticalAlignment(xaml::VerticalAlignment::Stretch);
  panel.HorizontalAlignment(xaml::HorizontalAlignment::Stretch);
  panel.BorderBrush(DefaultBrushStore::Instance().GetDefaultBorderBrush());
  return panel.as<XamlView>();
}

void ViewViewManager::GetNativeProps(const winrt::Microsoft::ReactNative::IJSValueWriter &writer) const {
  Super::GetNativeProps(writer);

  winrt::Microsoft::ReactNative::WriteProperty(writer, L"pointerEvents", L"string");
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"onClick", L"function");
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"focusable", L"boolean");
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"enableFocusRing", L"boolean");
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"tabIndex", L"number");
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"disabled", L"boolean");
  winrt::Microsoft::ReactNative::WriteProperty(writer, L"collapsable", L"boolean");
}

bool ViewViewManager::UpdateProperty(
    ShadowNodeBase *nodeToUpdate,
    const std::string &propertyName,
    const winrt::Microsoft::ReactNative::JSValue &propertyValue) {
  auto *pViewShadowNode = static_cast<ViewShadowNode *>(nodeToUpdate);

  auto pPanel = pViewShadowNode->GetPanel().as<winrt::Microsoft::ReactNative::ViewPanel>();
  bool ret = true;
  if (pPanel != nullptr) {
    if (TryUpdateBackgroundBrush(pPanel.as<xaml::Controls::Panel>(), propertyName, propertyValue)) {
    } else if (TryUpdateBorderProperties(nodeToUpdate, pPanel, propertyName, propertyValue)) {
    } else if (TryUpdateCornerRadiusOnNode(nodeToUpdate, pPanel, propertyName, propertyValue)) {
      // Do not clamp until a size has been set for the View
      auto maxCornerRadius = std::numeric_limits<double>::max();
      // The Width and Height properties are not always set on ViewPanel. In
      // cases where it is embedded in a Control, the values dimensions are
      // set on those wrapper elements. We cannot depend on the default
      // behavior of `UpdateCornerRadiusOnElement` to check for the clamp
      // dimension from only the ViewPanel.
      const xaml::FrameworkElement sizingElement =
          pViewShadowNode->IsControl() ? pViewShadowNode->GetControl().as<xaml::FrameworkElement>() : pPanel;
      if (sizingElement.ReadLocalValue(xaml::FrameworkElement::WidthProperty()) !=
              xaml::DependencyProperty::UnsetValue() &&
          sizingElement.ReadLocalValue(xaml::FrameworkElement::HeightProperty()) !=
              xaml::DependencyProperty::UnsetValue()) {
        maxCornerRadius = std::min(sizingElement.Width(), sizingElement.Height()) / 2;
      }
      UpdateCornerRadiusOnElement(nodeToUpdate, pPanel, maxCornerRadius);
    } else if (TryUpdateMouseEvents(nodeToUpdate, propertyName, propertyValue)) {
    } else if (propertyName == "onClick") {
      pViewShadowNode->OnClick(propertyValue.AsBoolean());
    } else if (propertyName == "focusable") {
      pViewShadowNode->IsFocusable(propertyValue.AsBoolean());
    } else if (propertyName == "enableFocusRing") {
      pViewShadowNode->EnableFocusRing(propertyValue.AsBoolean());
    } else if (propertyName == "tabIndex") {
      auto resetTabIndex = true;
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Int64 ||
          propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Double) {
        const auto tabIndex = propertyValue.AsInt64();
        if (tabIndex == static_cast<int32_t>(tabIndex)) {
          pViewShadowNode->TabIndex(static_cast<int32_t>(tabIndex));
          resetTabIndex = false;
        }
      }
      if (resetTabIndex) {
        pViewShadowNode->TabIndex(std::numeric_limits<std::int32_t>::max());
      }
    } else {
      if (propertyName == "accessible") {
        pViewShadowNode->IsAccessible(propertyValue.AsBoolean());
      }
      if (propertyName == "disabled") {
        pViewShadowNode->IsDisable(propertyValue.AsBoolean());
      }
      ret = Super::UpdateProperty(nodeToUpdate, propertyName, propertyValue);
    }
  }

  return ret;
}

void ViewViewManager::OnPropertiesUpdated(ShadowNodeBase *node) {
  auto *viewShadowNode = static_cast<ViewShadowNode *>(node);
  auto panel = viewShadowNode->GetPanel().as<winrt::Microsoft::ReactNative::ViewPanel>();

  if (panel.ReadLocalValue(ViewPanel::ViewBackgroundProperty()) == xaml::DependencyProperty::UnsetValue()) {
    // In XAML, a null background means no hit-test will happen.
    // We actually want hit-testing to happen if the app has registered
    // for mouse events, so detect that case and add a transparent background.
    if (viewShadowNode->IsHitTestBrushRequired()) {
      panel.ViewBackground(EnsureTransparentBrush());
    }
    // Note:  Technically we could detect when the transparent brush is
    // no longer needed, but this adds complexity and it can't hurt to
    // keep it around, so not adding that code (yet).
  }

  // If component is focusable, it should be a ViewControl.
  // If component is a View with accessible set to true, the component should be focusable, thus we need a ViewControl.
  bool shouldBeControl = viewShadowNode->IsDisable()
      ? false
      : ((viewShadowNode->IsFocusable() || (viewShadowNode->IsAccessible() && !viewShadowNode->OnClick())));

  if (auto view = viewShadowNode->GetView().try_as<xaml::UIElement>()) {
    // If we have DynamicAutomationProperties, we need a ViewControl with a
    // DynamicAutomationPeer
    shouldBeControl = shouldBeControl || HasDynamicAutomationProperties(view);
  }

  TryUpdateView(viewShadowNode, panel, shouldBeControl);
  SyncFocusableAndAccessible(viewShadowNode, shouldBeControl);
}

void ViewViewManager::TryUpdateView(
    ViewShadowNode *pViewShadowNode,
    winrt::Microsoft::ReactNative::ViewPanel &pPanel,
    bool useControl) {
  bool isControl = pViewShadowNode->IsControl();

  // This short-circuits all of the update code when we have the same hierarchy
  if (isControl == useControl)
    return;

  //
  // 1. Ensure we have the new 'root' and do the child replacement
  //      This is first to ensure that we can re-parent the ViewPanel
  //      we already have
  // 2. Transfer properties
  //      There are likely some complexities to handle here
  // 3. Do any sub=parenting
  //      This means Panel under Control
  //

  XamlView oldXamlView(pViewShadowNode->GetView());
  XamlView newXamlView(nullptr);

  //
  // 1. Determine new view & clean up any parent-child relationships
  //

  // If we need a Control then get existing reference or create it
  if (useControl) {
    newXamlView = pViewShadowNode->GetControl().try_as<XamlView>();
    if (newXamlView == nullptr) {
      newXamlView = pViewShadowNode->CreateViewControl();
    }
  }

  // Clean up child of Control if needed
  if (isControl) {
    pViewShadowNode->GetControl().Content(nullptr);
  }

  // If don't need a control, then set the Panel as the view root
  if (!useControl) {
    newXamlView = pPanel;
  }

  // ASSERT: One of the scenarios should be true, so we should have a root view
  assert(newXamlView != nullptr);

  //
  // 2. Transfer needed properties from old to new view
  //
  {
    TraceSection s("ViewViewManager::TransferProperties");
    // Transfer properties from old XamlView to the new one
    TransferProperties(oldXamlView, newXamlView);
  }

  // Update the meta-data in the shadow node
  pViewShadowNode->IsControl(useControl);

  //
  // 3. Setup any new parent-child relationships
  //

  // If we need to change the root of our view, do it now
  if (oldXamlView != newXamlView) {
    if (auto pNativeUiManager = GetNativeUIManager(*m_context).lock()) {
      // Inform the parent ShadowNode of this change so the hierarchy can be
      // updated
      int64_t parentTag = pViewShadowNode->GetParent();
      auto host = pNativeUiManager->getHost();
      auto *pParentNode = static_cast<ShadowNodeBase *>(host->FindShadowNodeForTag(parentTag));
      if (pParentNode != nullptr)
        pParentNode->ReplaceChild(oldXamlView, newXamlView);

      // Update the ShadowNode with the new XamlView
      pViewShadowNode->ReplaceView(newXamlView);
      pViewShadowNode->RefreshProperties();

      // Inform the NativeUIManager of this change so the yoga layout can be
      // updated
      pNativeUiManager->ReplaceView(*pViewShadowNode);
    }
  }

  if (useControl)
    pViewShadowNode->GetControl().Content(pPanel);
}

void ViewViewManager::SyncFocusableAndAccessible(ViewShadowNode *pViewShadowNode, bool useControl) {
  if (useControl) {
    const auto isFocusable = pViewShadowNode->IsFocusable();
    const auto isAccessible = pViewShadowNode->IsAccessible();
    const auto isDisabled = pViewShadowNode->IsDisable();
    const auto isTabStop = !isDisabled && isFocusable;
    const auto accessibilityView = isAccessible ? xaml::Automation::Peers::AccessibilityView::Content
                                                : xaml::Automation::Peers::AccessibilityView::Raw;
    pViewShadowNode->GetControl().IsTabStop(isTabStop);
    xaml::Automation::AutomationProperties::SetAccessibilityView(pViewShadowNode->GetControl(), accessibilityView);
  }
}

void ViewViewManager::SetLayoutProps(
    ShadowNodeBase &nodeToUpdate,
    const XamlView &viewToUpdate,
    float left,
    float top,
    float width,
    float height) {
  // When the View has a ContentControl the ViewPanel must also have the Width &
  // Height set
  // Do this first so that it is setup properly before any events are fired by
  // the Super implementation
  auto *pViewShadowNode = static_cast<ViewShadowNode *>(&nodeToUpdate);
  auto pPanel = pViewShadowNode->GetPanel().as<winrt::Microsoft::ReactNative::ViewPanel>();
  if (pViewShadowNode->IsControl()) {
    pPanel.Width(width);
    pPanel.Height(height);
  }

  Super::SetLayoutProps(nodeToUpdate, viewToUpdate, left, top, width, height);
  if (pPanel.ReadLocalValue(ViewPanel::CornerRadiusProperty()) != xaml::DependencyProperty::UnsetValue()) {
    if (pPanel.GetValue(React::ViewPanel::CornerRadiusProperty()) != xaml::DependencyProperty::UnsetValue()) {
      const auto maxCornerRadius = std::min(width, height) / 2;
      UpdateCornerRadiusOnElement(&nodeToUpdate, pPanel, maxCornerRadius);
      pPanel.CornerRadius(pPanel.CornerRadius());
    }
  }
}

xaml::Media::SolidColorBrush ViewViewManager::EnsureTransparentBrush() {
  if (!m_transparentBrush) {
    m_transparentBrush = xaml::Media::SolidColorBrush(winrt::Colors::Transparent());
  }
  return m_transparentBrush;
}

} // namespace Microsoft::ReactNative
