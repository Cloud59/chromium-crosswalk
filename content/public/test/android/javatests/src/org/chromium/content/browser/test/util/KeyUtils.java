// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.View;

import org.chromium.base.ThreadUtils;

/**
 * Collection of keyboard utilities.
 */
public class KeyUtils {
    /**
     * Sends (synchronously) a single key down/up pair of events to the specified view.
     * <p>
     * Does not use the event injecting framework, but instead relies on
     * {@link View#dispatchKeyEventPreIme(KeyEvent)} and {@link View#dispatchKeyEvent(KeyEvent)} of
     * the view itself
     * <p>
     * The event injecting framework requires INJECT_EVENTS permission and that has been flaky on
     * our perf bots.  So until a root cause of the issue can be found, we should use this instead
     * of the functionality provided by {@link #sendKeys(int...)}.
     *
     * @param i The application being instrumented.
     * @param v The view to receive the key event.
     * @param keyCode The keycode for the event to be issued.
     */
    public static void singleKeyEventView(Instrumentation i, final View v, int keyCode) {
        long downTime = SystemClock.uptimeMillis();
        long eventTime = SystemClock.uptimeMillis();

        final KeyEvent downEvent =
                new KeyEvent(downTime, eventTime, KeyEvent.ACTION_DOWN, keyCode, 0);
        dispatchKeyEvent(i, v, downEvent);

        downTime = SystemClock.uptimeMillis();
        eventTime = SystemClock.uptimeMillis();
        final KeyEvent upEvent =
                new KeyEvent(downTime, eventTime, KeyEvent.ACTION_UP, keyCode, 0);
        dispatchKeyEvent(i, v, upEvent);
    }

    private static void dispatchKeyEvent(final Instrumentation i, final View v,
            final KeyEvent event) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                if (!v.dispatchKeyEventPreIme(event)) {
                    v.dispatchKeyEvent(event);
                }
            }
        });
        i.waitForIdleSync();
    }
}