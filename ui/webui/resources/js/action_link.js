// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Action links are elements that are used to perform an in-page navigation or
// action (e.g. showing a dialog).
//
// They look like normal anchor (<a>) tags as their text color is blue. However,
// they're subtly different as they're not initially underlined (giving users a
// clue that underlined links navigate while action links don't).
//
// Action links look very similar to normal links when hovered (hand cursor,
// underlined). This gives the user an idea that clicking this link will do
// something similar to navigation but in the same page.
//
// They can be created in JavaScript like this:
//
//   var link = document.createElement('a', 'action-link');  // Note second arg.
//
// or with a constructor like this:
//
//   var link = new ActionLink();
//
// They can be used easily from HTML as well, like so:
//
//   <a is="action-link">Click me!</a>
//
// NOTE: <action-link> and document.createElement('action-link') don't work.

/**
 * @constructor
 * @extends {HTMLAnchorElement}
 */
var ActionLink = document.registerElement('action-link', {
  prototype: {
    __proto__: HTMLAnchorElement.prototype,

    /** @this {ActionLink} */
    createdCallback: function() {
      // Links aren't tabble unless there's an [href] attribute set. Setting
      // this adds undesirable "Open link in new tab..." context menu handlers
      // so just manually add to tab order instead.
      this.tabIndex = 0;

      this.addEventListener('keydown', function(e) {
        if (e.keyIdentifier == 'Enter') {
          // Schedule a click asynchronously because other 'keydown' handlers
          // may still run later (e.g. document.addEventListener('keydown')).
          // Specifically options dialogs break when this timeout isn't here.
          // NOTE: this affects the "trusted" state of the ensuing click. I
          // haven't found anything that breaks because of this (yet).
          window.setTimeout(this.click.bind(this), 0);
        }
      });
    },
  },
  extends: 'a',
});
