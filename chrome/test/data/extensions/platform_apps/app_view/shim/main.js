// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var util = {};
var embedder = {};
embedder.baseGuestURL = '';
embedder.emptyGuestURL = '';
embedder.windowOpenGuestURL = '';
embedder.noReferrerGuestURL = '';
embedder.redirectGuestURL = '';
embedder.redirectGuestURLDest = '';
embedder.closeSocketURL = '';
embedder.tests = {};

embedder.setUp_ = function(config) {
  if (!config || !config.testServer) {
    return;
  }
  embedder.baseGuestURL = 'http://localhost:' + config.testServer.port;
  embedder.emptyGuestURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/empty_guest.html';
};

window.runTest = function(testName, appToEmbed) {
  if (!embedder.test.testList[testName]) {
    window.console.log('Incorrect testName: ' + testName);
    embedder.test.fail();
    return;
  }

  // Run the test.
  embedder.test.testList[testName](appToEmbed);
};

var LOG = function(msg) {
  window.console.log(msg);
};

embedder.test = {};
embedder.test.succeed = function() {
  chrome.test.sendMessage('TEST_PASSED');
};

embedder.test.fail = function() {
  chrome.test.sendMessage('TEST_FAILED');
};

embedder.test.assertEq = function(a, b) {
  if (a != b) {
    console.log('assertion failed: ' + a + ' != ' + b);
    embedder.test.fail();
  }
};

embedder.test.assertTrue = function(condition) {
  if (!condition) {
    console.log('assertion failed: true != ' + condition);
    embedder.test.fail();
  }
};

embedder.test.assertFalse = function(condition) {
  if (condition) {
    console.log('assertion failed: false != ' + condition);
    embedder.test.fail();
  }
};

// Tests begin.
function testAppViewWithUndefinedDataShouldSucceed(appToEmbed) {
  var appview = new AppView();
  LOG('appToEmbed  ' + appToEmbed);
  document.body.appendChild(appview);
  // Step 1: Attempt to connect to a non-existant app.
  LOG('attempting to connect to non-existant app.');
  appview.connect('abc123', undefined, function(success) {
    // Make sure we fail.
    if (success) {
      LOG('UNEXPECTED CONNECTION.');
      embedder.test.fail();
      return;
    }
    LOG('failed to connect to non-existant app.');
    LOG('attempting to connect to known app.');
    // Step 2: Attempt to connect to an app we know exists.
    appview.connect(appToEmbed, undefined, function(success) {
      // Make sure we don't fail.
      if (!success) {
        LOG('FAILED TO CONNECT.');
        embedder.test.fail();
        return;
      }
      LOG('CONNECTED.');
      embedder.test.succeed();
    });
  });
};

function testAppViewRefusedDataShouldFail(appToEmbed) {
  var appview = new AppView();
  LOG('appToEmbed  ' + appToEmbed);
  document.body.appendChild(appview);
  LOG('Attempting to connect to app with refused params.');
  appview.connect(appToEmbed, { 'foo': 'bar' }, function(success) {
    // Make sure we fail.
    if (success) {
      LOG('UNEXPECTED CONNECTION.');
      embedder.test.fail();
      return;
    }
    LOG('FAILED TO CONNECT.');
    embedder.test.succeed();
  });
};

function testAppViewGoodDataShouldSucceed(appToEmbed) {
  var appview = new AppView();
  LOG('appToEmbed  ' + appToEmbed);
  document.body.appendChild(appview);
  LOG('Attempting to connect to app with good params.');
  // Step 2: Attempt to connect to an app with good params.
  appview.connect(appToEmbed, { 'foo': 'bleep' }, function(success) {
    // Make sure we don't fail.
    if (!success) {
      LOG('FAILED TO CONNECT.');
      embedder.test.fail();
      return;
    }
    LOG('CONNECTED.');
    embedder.test.succeed();
  });
};

embedder.test.testList = {
  'testAppViewWithUndefinedDataShouldSucceed':
      testAppViewWithUndefinedDataShouldSucceed,
  'testAppViewRefusedDataShouldFail': testAppViewRefusedDataShouldFail,
  'testAppViewGoodDataShouldSucceed': testAppViewGoodDataShouldSucceed
};

onload = function() {
  chrome.test.getConfig(function(config) {
    embedder.setUp_(config);
    chrome.test.sendMessage('Launched');
  });
};
