/**
 * activitytracker.js
 *
 * Copyright (C) 2026 Raden Solutions
 *
 * Reports real user input (mouse button press, mouse wheel, key press) to the server side
 * inactivity timer. Listeners are attached in capture phase to the main document and to the
 * documents of same-origin iframes (Browser widgets, VNC viewer), so input is seen regardless
 * of whether the widget under the pointer listens for it. Listeners are passive and do not
 * affect event processing. Notifications are sent at most once per REPORT_INTERVAL milliseconds.
 */
(function() {
   'use strict';

   var REPORT_INTERVAL = 5000;
   var EVENTS = [ "mousedown", "wheel", "keydown" ];
   var LISTENER_OPTIONS = { capture: true, passive: true };

   rap.registerTypeHandler("netxms.ActivityTracker", {
      factory : function() {
         return new ActivityTracker();
      },
      destructor : "destroy",
      events : [ "activity" ]
   });

   function ActivityTracker() {
      var self = this;
      var lastReport = -REPORT_INTERVAL;
      this.trackedFrames = new WeakSet();
      this.listener = function() {
         var now = performance.now();
         if (now - lastReport >= REPORT_INTERVAL) {
            lastReport = now;
            rap.getRemoteObject(self).notify("activity");
         }
      };
      this.frameLoadListener = function(e) {
         self.attachToFrameDocument(e.target);
      };
      this.attach(document);

      // Browser widgets replace iframe content on each navigation, so listeners are attached again on every load
      this.observer = new MutationObserver(function(mutations) {
         mutations.forEach(function(m) {
            m.addedNodes.forEach(function(node) {
               if (node.nodeType !== Node.ELEMENT_NODE)
                  return;
               if (node.tagName === "IFRAME")
                  self.trackFrame(node);
               else
                  Array.prototype.forEach.call(node.getElementsByTagName("iframe"), function(f) { self.trackFrame(f); });
            });
         });
      });
      this.observer.observe(document.body, { childList: true, subtree: true });
      Array.prototype.forEach.call(document.getElementsByTagName("iframe"), function(f) { self.trackFrame(f); });
   }

   ActivityTracker.prototype.attach = function(doc) {
      var self = this;
      EVENTS.forEach(function(type) {
         doc.addEventListener(type, self.listener, LISTENER_OPTIONS);
      });
   };

   ActivityTracker.prototype.trackFrame = function(frame) {
      if (this.trackedFrames.has(frame))
         return;
      this.trackedFrames.add(frame);
      frame.addEventListener("load", this.frameLoadListener);
      this.attachToFrameDocument(frame);
   };

   // contentDocument is null for cross-origin content, such iframes are not tracked
   ActivityTracker.prototype.attachToFrameDocument = function(frame) {
      try {
         var doc = frame.contentDocument;
         if (doc)
            this.attach(doc);
      } catch (e) {
      }
   };

   ActivityTracker.prototype.destroy = function() {
      var self = this;
      this.observer.disconnect();
      EVENTS.forEach(function(type) {
         document.removeEventListener(type, self.listener, LISTENER_OPTIONS);
      });
   };
}());
