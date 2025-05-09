// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <react/components/rnwcore/Props.h>
#include <react/renderer/attributedstring/conversions.h>
#include <react/renderer/components/text/BaseTextProps.h>
#include <react/renderer/core/propsConversions.h>

namespace facebook::react {

struct CompWindowsTextInputSelectionStruct {
  int start;
  int end;
};

static inline void
fromRawValue(const PropsParserContext &context, const RawValue &value, CompWindowsTextInputSelectionStruct &result) {
  auto map = (std::unordered_map<std::string, RawValue>)value;

  auto tmp_start = map.find("start");
  if (tmp_start != map.end()) {
    fromRawValue(context, tmp_start->second, result.start);
  }
  auto tmp_end = map.find("end");
  if (tmp_end != map.end()) {
    fromRawValue(context, tmp_end->second, result.end);
  }
}

static inline std::string toString(const CompWindowsTextInputSelectionStruct &value) {
  return "[Object CompWindowsTextInputSelectionStruct]";
}

struct CompWindowsTextInputSubmitKeyEventsStruct {
  bool altKey{false};
  bool ctrlKey{false};
  bool metaKey{false};
  bool shiftKey{false};
  std::string code;
};

static inline void fromRawValue(
    const PropsParserContext &context,
    const RawValue &value,
    CompWindowsTextInputSubmitKeyEventsStruct &result) {
  auto map = (std::unordered_map<std::string, RawValue>)value;

  auto tmp_altKey = map.find("altKey");
  if (tmp_altKey != map.end()) {
    fromRawValue(context, tmp_altKey->second, result.altKey);
  }
  auto tmp_ctrlKey = map.find("ctrlKey");
  if (tmp_ctrlKey != map.end()) {
    fromRawValue(context, tmp_ctrlKey->second, result.ctrlKey);
  }
  auto tmp_metaKey = map.find("metaKey");
  if (tmp_metaKey != map.end()) {
    fromRawValue(context, tmp_metaKey->second, result.metaKey);
  }
  auto tmp_shiftKey = map.find("shiftKey");
  if (tmp_shiftKey != map.end()) {
    fromRawValue(context, tmp_shiftKey->second, result.shiftKey);
  }
  auto tmp_code = map.find("code");
  if (tmp_code != map.end()) {
    fromRawValue(context, tmp_code->second, result.code);
  }
}

static inline std::string toString(const CompWindowsTextInputSubmitKeyEventsStruct &value) {
  return "[Object CompWindowsTextInputSubmitKeyEventsStruct]";
}

static inline void fromRawValue(
    const PropsParserContext &context,
    const RawValue &value,
    std::vector<CompWindowsTextInputSubmitKeyEventsStruct> &result) {
  auto items = (std::vector<RawValue>)value;
  for (const auto &item : items) {
    CompWindowsTextInputSubmitKeyEventsStruct newItem;
    fromRawValue(context, item, newItem);
    result.emplace_back(newItem);
  }
}

class WindowsTextInputProps final : public ViewProps, public BaseTextProps {
 public:
  WindowsTextInputProps() = default;
  WindowsTextInputProps(
      const PropsParserContext &context,
      const WindowsTextInputProps &sourceProps,
      const RawProps &rawProps);

  void
  setProp(const PropsParserContext &context, RawPropsPropNameHash hash, const char *propName, RawValue const &value);

  bool allowFontScaling{true};
  bool autoCorrect{true};
  bool clearTextOnFocus{false};
  bool editable{true};
  int maxLength{0};
  bool multiline{false};
  std::string placeholder{};
  SharedColor placeholderTextColor{};
  bool scrollEnabled{true};
  SharedColor cursorColor{};
  CompWindowsTextInputSelectionStruct selection{};
  SharedColor selectionColor{};
  bool selectTextOnFocus{false};
  bool spellCheck{true};
  std::string text{};
  int mostRecentEventCount{0};
  bool secureTextEntry{false};
  std::string keyboardType{};
  bool contextMenuHidden{false};
  bool caretHidden{false};
  std::string autoCapitalize{};
  bool clearTextOnSubmit{false};
  std::vector<CompWindowsTextInputSubmitKeyEventsStruct> submitKeyEvents{};
  bool autoFocus{false};
  facebook::react::TextAlignment textAlign{};
};

} // namespace facebook::react
