/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

import type {HostInstance} from '../../../src/private/types/HostInstance';
import type {EventSubscription} from '../../vendor/emitter/EventEmitter';

import RCTDeviceEventEmitter from '../../EventEmitter/RCTDeviceEventEmitter';
import {sendAccessibilityEvent} from '../../ReactNative/RendererProxy';
import Platform from '../../Utilities/Platform';
import legacySendAccessibilityEvent from './legacySendAccessibilityEvent';
import NativeAccessibilityInfo from './NativeAccessibilityInfo';
import NativeAccessibilityInfoAndroid from './NativeAccessibilityInfo';
import NativeAccessibilityInfoWin32 from './NativeAccessibilityInfoWin32';
import NativeAccessibilityManagerIOS from './NativeAccessibilityManager';

// Events that are only supported on Android.
type AccessibilityEventDefinitionsAndroid = {
  accessibilityServiceChanged: [boolean],
  highTextContrastChanged: [boolean],
};

// Events that are only supported on iOS.
type AccessibilityEventDefinitionsIOS = {
  announcementFinished: [{announcement: string, success: boolean}],
  boldTextChanged: [boolean],
  grayscaleChanged: [boolean],
  invertColorsChanged: [boolean],
  reduceTransparencyChanged: [boolean],
  darkerSystemColorsChanged: [boolean],
};

type AccessibilityEventDefinitions = {
  ...AccessibilityEventDefinitionsAndroid,
  ...AccessibilityEventDefinitionsIOS,
  change: [boolean], // screenReaderChanged
  reduceMotionChanged: [boolean],
  screenReaderChanged: [boolean],
};

type AccessibilityEventTypes = 'click' | 'focus' | 'viewHoverEnter';

// Mapping of public event names to platform-specific event names.
const EventNames: Map<
  $Keys<AccessibilityEventDefinitions>,
  string,
> = Platform.OS === 'android'
  ? new Map([
      ['change', 'touchExplorationDidChange'],
      ['reduceMotionChanged', 'reduceMotionDidChange'],
      ['highTextContrastChanged', 'highTextContrastDidChange'],
      ['screenReaderChanged', 'touchExplorationDidChange'],
      ['accessibilityServiceChanged', 'accessibilityServiceDidChange'],
      ['invertColorsChanged', 'invertColorDidChange'],
      ['grayscaleChanged', 'grayscaleModeDidChange'],
    ])
  : Platform.OS === 'win32'
    ? new Map([
        ['change', 'TOUCH_EXPLORATION_EVENT'],
        ['reduceMotionChanged', 'REDUCE_MOTION_EVENT'],
        ['screenReaderChanged', 'TOUCH_EXPLORATION_EVENT'],
      ])
    : new Map([
        ['announcementFinished', 'announcementFinished'],
        ['boldTextChanged', 'boldTextChanged'],
        ['change', 'screenReaderChanged'],
        ['grayscaleChanged', 'grayscaleChanged'],
        ['invertColorsChanged', 'invertColorsChanged'],
        ['reduceMotionChanged', 'reduceMotionChanged'],
        ['reduceTransparencyChanged', 'reduceTransparencyChanged'],
        ['screenReaderChanged', 'screenReaderChanged'],
        ['darkerSystemColorsChanged', 'darkerSystemColorsChanged'],
      ]);

/**
 * Sometimes it's useful to know whether or not the device has a screen reader
 * that is currently active. The `AccessibilityInfo` API is designed for this
 * purpose. You can use it to query the current state of the screen reader as
 * well as to register to be notified when the state of the screen reader
 * changes.
 *
 * See https://reactnative.dev/docs/accessibilityinfo
 */
const AccessibilityInfo = {
  /**
   * Query whether bold text is currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when bold text is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#isBoldTextEnabled
   */
  isBoldTextEnabled(): Promise<boolean> {
    if (Platform.OS === 'android' || Platform.OS === 'win32') {
      return Promise.resolve(false);
    } else {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityManagerIOS != null) {
          NativeAccessibilityManagerIOS.getCurrentBoldTextState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      });
    }
  },

  /**
   * Query whether grayscale is currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when grayscale is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#isGrayscaleEnabled
   */
  isGrayscaleEnabled(): Promise<boolean> {
    if (Platform.OS === 'android') {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityInfoAndroid?.isGrayscaleEnabled != null) {
          NativeAccessibilityInfoAndroid.isGrayscaleEnabled(resolve);
        } else {
          reject(null);
        }
      });
    } else if (Platform.OS === 'win32') {
      return Promise.resolve(false);
    } else {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityManagerIOS != null) {
          NativeAccessibilityManagerIOS.getCurrentGrayscaleState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      });
    }
  },

  /**
   * Query whether inverted colors are currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when invert color is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#isInvertColorsEnabled
   */
  isInvertColorsEnabled(): Promise<boolean> {
    if (Platform.OS === 'android') {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityInfoAndroid?.isInvertColorsEnabled != null) {
          NativeAccessibilityInfoAndroid.isInvertColorsEnabled(resolve);
        } else {
          reject(null);
        }
      });
    } else if (Platform.OS === 'win32') {
      return Promise.resolve(false);
    } else {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityManagerIOS != null) {
          NativeAccessibilityManagerIOS.getCurrentInvertColorsState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      });
    }
  },

  /**
   * Query whether reduced motion is currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when a reduce motion is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#isReduceMotionEnabled
   */
  isReduceMotionEnabled(): Promise<boolean> {
    return new Promise((resolve, reject) => {
      if (Platform.OS === 'android') {
        if (NativeAccessibilityInfo != null) {
          NativeAccessibilityInfo.isReduceMotionEnabled(resolve);
        } else {
          reject(null);
        }
      } else if (Platform.OS === 'win32') {
        if (NativeAccessibilityInfoWin32 != null) {
          NativeAccessibilityInfoWin32.isReduceMotionEnabled(resolve);
        } else {
          reject(null);
        }
      } else {
        if (NativeAccessibilityManagerIOS != null) {
          NativeAccessibilityManagerIOS.getCurrentReduceMotionState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      }
    });
  },

  /**
   * Query whether high text contrast is currently enabled. Android only.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when high text contrast is enabled and `false` otherwise.
   */
  isHighTextContrastEnabled(): Promise<boolean> {
    return new Promise((resolve, reject) => {
      if (Platform.OS === 'android') {
        if (NativeAccessibilityInfo?.isHighTextContrastEnabled != null) {
          NativeAccessibilityInfo.isHighTextContrastEnabled(resolve);
        } else {
          reject(null);
        }
      } else {
        return Promise.resolve(false);
      }
    });
  },

  /**
   * Query whether dark system colors is currently enabled. iOS only.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when dark system colors is enabled and `false` otherwise.
   */
  isDarkerSystemColorsEnabled(): Promise<boolean> {
    return new Promise((resolve, reject) => {
      if (Platform.OS === 'android') {
        return Promise.resolve(false);
      } else {
        if (
          NativeAccessibilityManagerIOS?.getCurrentDarkerSystemColorsState !=
          null
        ) {
          NativeAccessibilityManagerIOS.getCurrentDarkerSystemColorsState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      }
    });
  },

  /**
   * Query whether reduce motion and prefer cross-fade transitions settings are currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when  prefer cross-fade transitions is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#prefersCrossFadeTransitions
   */
  prefersCrossFadeTransitions(): Promise<boolean> {
    return new Promise((resolve, reject) => {
      if (Platform.OS === 'android') {
        return Promise.resolve(false);
      } else {
        if (
          NativeAccessibilityManagerIOS?.getCurrentPrefersCrossFadeTransitionsState !=
          null
        ) {
          NativeAccessibilityManagerIOS.getCurrentPrefersCrossFadeTransitionsState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      }
    });
  },

  /**
   * Query whether reduced transparency is currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when a reduce transparency is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#isReduceTransparencyEnabled
   */
  isReduceTransparencyEnabled(): Promise<boolean> {
    if (Platform.OS === 'android' || Platform.OS === 'win32') {
      return Promise.resolve(false);
    } else {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityManagerIOS != null) {
          NativeAccessibilityManagerIOS.getCurrentReduceTransparencyState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      });
    }
  },

  /**
   * Query whether a screen reader is currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when a screen reader is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#isScreenReaderEnabled
   */
  isScreenReaderEnabled(): Promise<boolean> {
    return new Promise((resolve, reject) => {
      if (Platform.OS === 'android') {
        if (NativeAccessibilityInfo != null) {
          NativeAccessibilityInfo.isTouchExplorationEnabled(resolve);
        } else {
          reject(null);
        }
      } else if (Platform.OS === 'win32') {
        if (NativeAccessibilityInfoWin32 != null) {
          NativeAccessibilityInfoWin32.isTouchExplorationEnabled(resolve);
        } else {
          reject(null);
        }
      } else {
        if (NativeAccessibilityManagerIOS != null) {
          NativeAccessibilityManagerIOS.getCurrentVoiceOverState(
            resolve,
            reject,
          );
        } else {
          reject(null);
        }
      }
    });
  },

  /**
   * Query whether Accessibility Service is currently enabled.
   *
   * Returns a promise which resolves to a boolean.
   * The result is `true` when any service is enabled and `false` otherwise.
   *
   * @platform android
   *
   * See https://reactnative.dev/docs/accessibilityinfo/#isaccessibilityserviceenabled-android
   */
  isAccessibilityServiceEnabled(): Promise<boolean> {
    return new Promise((resolve, reject) => {
      if (Platform.OS === 'android') {
        if (
          NativeAccessibilityInfo != null &&
          NativeAccessibilityInfo.isAccessibilityServiceEnabled != null
        ) {
          NativeAccessibilityInfo.isAccessibilityServiceEnabled(resolve);
        } else {
          reject(null);
        }
      } else {
        reject(null);
      }
    });
  },

  /**
   * Add an event handler. Supported events:
   *
   * - `reduceMotionChanged`: Fires when the state of the reduce motion toggle changes.
   *   The argument to the event handler is a boolean. The boolean is `true` when a reduce
   *   motion is enabled (or when "Transition Animation Scale" in "Developer options" is
   *   "Animation off") and `false` otherwise.
   * - `screenReaderChanged`: Fires when the state of the screen reader changes. The argument
   *   to the event handler is a boolean. The boolean is `true` when a screen
   *   reader is enabled and `false` otherwise.
   *
   * These events are only supported on iOS:
   *
   * - `boldTextChanged`: iOS-only event. Fires when the state of the bold text toggle changes.
   *   The argument to the event handler is a boolean. The boolean is `true` when a bold text
   *   is enabled and `false` otherwise.
   * - `grayscaleChanged`: iOS-only event. Fires when the state of the gray scale toggle changes.
   *   The argument to the event handler is a boolean. The boolean is `true` when a gray scale
   *   is enabled and `false` otherwise.
   * - `invertColorsChanged`: iOS-only event. Fires when the state of the invert colors toggle
   *   changes. The argument to the event handler is a boolean. The boolean is `true` when a invert
   *   colors is enabled and `false` otherwise.
   * - `reduceTransparencyChanged`: iOS-only event. Fires when the state of the reduce transparency
   *   toggle changes.  The argument to the event handler is a boolean. The boolean is `true`
   *   when a reduce transparency is enabled and `false` otherwise.
   * - `announcementFinished`: iOS-only event. Fires when the screen reader has
   *   finished making an announcement. The argument to the event handler is a
   *   dictionary with these keys:
   *     - `announcement`: The string announced by the screen reader.
   *     - `success`: A boolean indicating whether the announcement was
   *       successfully made.
   * - `darkerSystemColorsChanged`: iOS-only event. Fires when the state of the dark system colors
   *   toggle changes. The argument to the event handler is a boolean. The boolean is `true` when
   *   dark system colors is enabled and `false` otherwise.
   *
   * These events are only supported on Android:
   *
   * - `highTextContrastChanged`: Android-only event. Fires when the state of the high text contrast
   *   toggle changes. The argument to the event handler is a boolean. The boolean is `true` when
   *   high text contrast is enabled and `false` otherwise.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#addeventlistener
   */
  addEventListener<K: $Keys<AccessibilityEventDefinitions>>(
    eventName: K,
    // $FlowIssue[incompatible-type] - Flow bug with unions and generics (T128099423)
    handler: (...AccessibilityEventDefinitions[K]) => void,
  ): EventSubscription {
    const deviceEventName = EventNames.get(eventName);
    return deviceEventName == null
      ? {remove(): void {}}
      : // $FlowFixMe[incompatible-call]
        RCTDeviceEventEmitter.addListener(deviceEventName, handler);
  },

  /**
   * Set accessibility focus to a React component.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#setaccessibilityfocus
   */
  setAccessibilityFocus(reactTag: number): void {
    legacySendAccessibilityEvent(reactTag, 'focus');
  },

  /**
   * Send a named accessibility event to a HostComponent.
   */
  sendAccessibilityEvent(
    handle: HostInstance,
    eventType: AccessibilityEventTypes,
  ) {
    // iOS only supports 'focus' event types
    if (Platform.OS === 'ios' && eventType === 'click') {
      return;
    }
    // route through React renderer to distinguish between Fabric and non-Fabric handles
    sendAccessibilityEvent(handle, eventType);
  },

  /**
   * Post a string to be announced by the screen reader.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#announceforaccessibility
   */
  announceForAccessibility(announcement: string): void {
    if (Platform.OS === 'android') {
      NativeAccessibilityInfo?.announceForAccessibility(announcement);
    } else if (Platform.OS === 'win32') {
      NativeAccessibilityInfoWin32?.announceForAccessibility(announcement);
    } else {
      NativeAccessibilityManagerIOS?.announceForAccessibility(announcement);
    }
  },

  /**
   * Post a string to be announced by the screen reader.
   * - `announcement`: The string announced by the screen reader.
   * - `options`: An object that configures the reading options.
   *   - `queue`: The announcement will be queued behind existing announcements. iOS only.
   *   - `nativeID`: The nativeID of the element to send the announcement from. win32 only.
   */
  announceForAccessibilityWithOptions(
    announcement: string,
    options: {
      queue?: boolean,
      nativeID?: string, // win32
    },
  ): void {
    if (Platform.OS === 'android') {
      NativeAccessibilityInfo?.announceForAccessibility(announcement);
    } else if (Platform.OS === 'win32') {
      if (NativeAccessibilityInfoWin32?.announceForAccessibilityWithOptions) {
        NativeAccessibilityInfoWin32?.announceForAccessibilityWithOptions(
          announcement,
          options,
        );
      } else {
        NativeAccessibilityInfoWin32?.announceForAccessibility(announcement);
      }
    } else {
      if (NativeAccessibilityManagerIOS?.announceForAccessibilityWithOptions) {
        const {nativeID: _, ...iosOptions} = options;
        // $FlowFixMe[prop-missing]
        NativeAccessibilityManagerIOS?.announceForAccessibilityWithOptions(
          announcement,
          iosOptions,
        );
      } else {
        NativeAccessibilityManagerIOS?.announceForAccessibility(announcement);
      }
    }
  },

  /**
   * Get the recommended timeout for changes to the UI needed by this user.
   *
   * See https://reactnative.dev/docs/accessibilityinfo#getrecommendedtimeoutmillis
   */
  getRecommendedTimeoutMillis(originalTimeout: number): Promise<number> {
    if (Platform.OS === 'android') {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityInfo?.getRecommendedTimeoutMillis) {
          NativeAccessibilityInfo.getRecommendedTimeoutMillis(
            originalTimeout,
            resolve,
          );
        } else {
          resolve(originalTimeout);
        }
      });
    } else if (Platform.OS === 'win32') {
      return new Promise((resolve, reject) => {
        if (NativeAccessibilityInfoWin32?.getRecommendedTimeoutMillis) {
          NativeAccessibilityInfoWin32.getRecommendedTimeoutMillis(
            originalTimeout,
            resolve,
          );
        } else {
          resolve(originalTimeout);
        }
      });
    } else {
      return Promise.resolve(originalTimeout);
    }
  },
};

export default AccessibilityInfo;
