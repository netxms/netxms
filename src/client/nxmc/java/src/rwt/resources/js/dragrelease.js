/**
 * dragrelease.js
 *
 * Emulation of mouse pointer grab for drag operations.
 *
 * Native toolkits grab the pointer for the duration of mouse button press, so widget that received
 * mouse down always receives matching mouse up. Browser does not do that - mouse up is delivered to
 * the element under the pointer, and mouse button released outside of browser window does not
 * produce mouse up event for the page at all. As a result widget that started drag operation (for
 * example network map) never learns that button was released and keeps dragging until next click.
 *
 * Deliver synthetic mouse up event to the element that received mouse down whenever real mouse up
 * was delivered to a different widget, or when pointer returns to the page with button no longer
 * pressed, or when window loses focus while button is pressed. Event coordinates are clamped to the
 * bounds of that element, so that widget interprets it as release within itself.
 */
(function() {
   var buttonPressed = false;
   var pressTarget = null;
   var lastPosition = null;
   var lastTouchTime = 0;

   function savePosition(e)
   {
      lastPosition = { x: e.clientX, y: e.clientY };
   }

   /**
    * Find element of the widget owning given element.
    */
   function widgetElement(element)
   {
      while((element != null) && (element.rwtWidget == null))
         element = element.parentElement;
      return element;
   }

   /**
    * Check if given mouse event was emulated by browser for touch input. Such events may report no
    * buttons pressed while touch drag is in progress, which is indistinguishable from mouse button
    * released outside of browser window.
    */
   function emulatedByTouch(e)
   {
      if ((e.sourceCapabilities != null) && e.sourceCapabilities.firesTouchEvents)
         return true;
      return (new Date().getTime() - lastTouchTime) < 1000;
   }

   /**
    * Deliver synthetic mouse up event to element that received mouse down. Delivery is delayed until
    * current event is fully processed, so that real mouse up completes drag and drop operation
    * started by the browser before element that received mouse down is told that button is released.
    */
   function releasePressTarget()
   {
      var target = pressTarget;
      var position = lastPosition;
      pressTarget = null;
      if ((target === null) || (position === null))
         return;

      setTimeout(function() {
         if (!target.isConnected)
            return;

         // Widget ignores release outside of its bounds, and if pointer left the browser window last
         // known position is usually outside of them
         var bounds = target.getBoundingClientRect();
         var x = Math.round(Math.min(Math.max(position.x, bounds.left), bounds.right - 1));
         var y = Math.round(Math.min(Math.max(position.y, bounds.top), bounds.bottom - 1));

         target.dispatchEvent(new MouseEvent("mouseup", {
            view: window,
            bubbles: true,
            cancelable: true,
            button: 0,
            buttons: 0,
            clientX: x,
            clientY: y,
            screenX: x,
            screenY: y
         }));
      }, 0);
   }

   function registerTouch()
   {
      lastTouchTime = new Date().getTime();
   }

   window.addEventListener("touchstart", registerTouch, true);
   window.addEventListener("touchmove", registerTouch, true);
   window.addEventListener("touchend", registerTouch, true);
   window.addEventListener("touchcancel", registerTouch, true);

   window.addEventListener("mousedown", function(e) {
      if (e.button === 0)
      {
         buttonPressed = true;
         pressTarget = e.target;
         savePosition(e);
      }
   }, true);

   window.addEventListener("mouseup", function(e) {
      if (e.button !== 0)
         return;

      buttonPressed = false;
      if (!e.isTrusted)
         return;

      if (widgetElement(e.target) !== widgetElement(pressTarget))
      {
         savePosition(e);
         releasePressTarget();
      }
      else
      {
         pressTarget = null;
      }
   }, true);

   window.addEventListener("mousemove", function(e) {
      if (!buttonPressed)
         return;

      if ((e.buttons & 1) !== 0)
      {
         savePosition(e);
      }
      else if (!emulatedByTouch(e))
      {
         buttonPressed = false;
         releasePressTarget();
      }
   }, true);

   window.addEventListener("blur", function() {
      if (buttonPressed)
      {
         buttonPressed = false;
         releasePressTarget();
      }
   });
})();
