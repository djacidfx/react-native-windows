/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @format
 * @flow strict-local
 */

'use strict';

import RNTesterText from '../components/RNTesterText';
import React from 'react';
import {Image, TouchableHighlight, View} from 'react-native';

function Basic(): React.Node {
  return (
    <RNTesterText testID={'text-view'}>
      This text contains an inline blue view{' '}
      <View style={{width: 25, height: 25, backgroundColor: 'steelblue'}} /> and
      an inline image <Image source={require('../assets/flux.png')} />. Neat,
      huh?
    </RNTesterText>
  );
}

function NestedTexts(): React.Node {
  // [Windows] accessible = {true} is needed to find Views in e2etests
  return (
    <View testID="text-nested-view" accessible={true}>
      <RNTesterText>This is the first row</RNTesterText>
      <RNTesterText>
        <RNTesterText>
          <RNTesterText>This is a nested text </RNTesterText>
          <View style={{height: 20, width: 20, backgroundColor: 'red'}} />
          <RNTesterText> with a Red View</RNTesterText>
        </RNTesterText>
      </RNTesterText>
    </View>
  );
}

function ClippedByText(): React.Node {
  // [Windows] accessible = {true} is needed to find Views in e2etests
  return (
    <View testID="text-view-images-clipped" accessible={true}>
      {/*
       * Inline View
       **/}
      <RNTesterText>
        The{' '}
        <RNTesterText style={{fontWeight: 'bold'}}>inline view</RNTesterText>{' '}
        below is taller than its Text parent and should be clipped.
      </RNTesterText>
      <RNTesterText
        style={{
          overflow: 'hidden',
          width: 150,
          height: 75,
          backgroundColor: 'lightgrey',
        }}>
        This is an inline view
        {/* Render a red border around the steelblue rectangle to make it clear how the inline view is being clipped */}
        <View style={{width: 50, height: 100, backgroundColor: 'red'}}>
          <View
            style={{
              width: 48,
              height: 98,
              left: 1,
              top: 1,
              backgroundColor: 'steelblue',
            }}
          />
        </View>
      </RNTesterText>

      {/*
       * Inline Image
       **/}
      <RNTesterText style={{marginTop: 10}}>
        The{' '}
        <RNTesterText style={{fontWeight: 'bold'}}>inline image</RNTesterText>{' '}
        below is taller than its Text parent and should be clipped.
      </RNTesterText>
      <RNTesterText
        style={{
          overflow: 'hidden',
          width: 175,
          height: 100,
          backgroundColor: 'lightgrey',
        }}>
        This is an inline image
        <Image
          source={{
            uri: 'https://picsum.photos/100',
            width: 50,
            height: 100,
          }}
          style={{
            width: 50,
            height: 100,
          }}
        />
      </RNTesterText>
    </View>
  );
}

type ChangeSizeState = {|
  width: number,
|};

class ChangeImageSize extends React.Component<mixed, ChangeSizeState> {
  state: ChangeSizeState = {
    width: 50,
  };

  render(): React.Node {
    return (
      <View>
        <TouchableHighlight
          onPress={() => {
            this.setState({width: this.state.width === 50 ? 100 : 50});
          }}>
          <RNTesterText style={{fontSize: 15}}>
            Change Image Width (width={this.state.width})
          </RNTesterText>
        </TouchableHighlight>
        <RNTesterText>
          This is an
          <Image
            source={{
              uri: 'https://picsum.photos/50',
              width: this.state.width,
              height: 50,
            }}
            style={{
              width: this.state.width,
              height: 50,
            }}
          />
          inline image
        </RNTesterText>
      </View>
    );
  }
}

class ChangeViewSize extends React.Component<mixed, ChangeSizeState> {
  state: ChangeSizeState = {
    width: 50,
  };

  render(): React.Node {
    return (
      <View>
        <TouchableHighlight
          onPress={() => {
            this.setState({width: this.state.width === 50 ? 100 : 50});
          }}>
          <RNTesterText style={{fontSize: 15}}>
            Change View Width (width={this.state.width})
          </RNTesterText>
        </TouchableHighlight>
        <RNTesterText>
          This is an
          <View
            style={{
              width: this.state.width,
              height: 50,
              backgroundColor: 'steelblue',
            }}
          />
          inline view
        </RNTesterText>
      </View>
    );
  }
}

class ChangeInnerViewSize extends React.Component<mixed, ChangeSizeState> {
  state: ChangeSizeState = {
    width: 50,
  };

  render(): React.Node {
    return (
      <View>
        <TouchableHighlight
          onPress={() => {
            this.setState({width: this.state.width === 50 ? 100 : 50});
          }}>
          {/* When updating `state.width`, it's important that the only thing that
              changes is the width of the pink inline view. When we do this, we
              demonstrate a bug in RN Android where the pink view doesn't get
              rerendered and remains at its old size. If other things change
              (e.g. we display `state.width` as text somewhere) it could circumvent
              the bug and cause the pink view to be rerendered at its new size. */}
          <RNTesterText style={{fontSize: 15}}>
            Change Pink View Width
          </RNTesterText>
        </TouchableHighlight>
        <RNTesterText>
          This is an
          <View style={{width: 125, height: 75, backgroundColor: 'steelblue'}}>
            <View
              style={{
                width: this.state.width,
                height: 50,
                backgroundColor: 'pink',
              }}
            />
          </View>
          inline view
        </RNTesterText>
      </View>
    );
  }
}

module.exports = {
  Basic,
  NestedTexts,
  ClippedByText,
  ChangeImageSize,
  ChangeViewSize,
  ChangeInnerViewSize,
};
