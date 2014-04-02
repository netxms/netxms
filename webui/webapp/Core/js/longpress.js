/**
 * longtap.js
 *
 * License: MIT License - http://www.opensource.org/licenses/mit-license.php
 *
 * Based on longpress.js: http://github.com/vaidik/longpress.js/
 *
 * Copyright (c) 2008-2013, Vaidik Kapoor (kapoor [*dot*] vaidik -[at]- gmail [*dot*] com)
 */

(function() {
    // default configurations for longpress
    var config = {
        duration: 500,
    };

    // override default_config
    if (typeof longpress_config == "object") {
        for (var key in longpress_config) {
            config[key] = longpress_config[key];
        }
    }

    var body = document.body;
    body.addEventListener('mousedown', mousedown_callback);
    body.addEventListener('touchstart', mousedown_callback);

    function mousedown_callback(e) {
        var touch = e.type.indexOf('touch') === 0;
        
        if (!touch && (e.button !== 0))
           return;

        var mousedown_time = new Date().getTime();
        
        e.target.addEventListener('mouseup', mouseup_callback);
        e.target.addEventListener('touchend', mouseup_callback);

        // dispatch longpress event after the default longpress duration timeout
        var timeout = setTimeout(function() {
            refireEvent(e, touch);

            // we don't need mouseup event handlers any more
            clean_mouseup(e);
        }, config.duration);

        function mouseup_callback(e) {
            var click_duration = new Date().getTime() - mousedown_time;

            // if touch/click is released before longpress duration, clear the
            // timeout.
            if (click_duration < config.duration) {
                clearTimeout(timeout);
            } else {
               refireEvent(e, touch);
            }

            // we don't need mouseup event handlers any more
            clean_mouseup(e);
        }

        // removes mouseup/touchup event handlers after successful longpress
        // event or release before timeout of longpress duration
        function clean_mouseup(e) {
            e.target.removeEventListener('mouseup', mouseup_callback);
            e.target.removeEventListener('touchend', mouseup_callback);
        }
        
        function refireEvent(originalEvent, touch)
        {
           var clientX, clientY, screenX, screenY;
           if (touch)
           {
               screenX = originalEvent.changedTouches[0].pageX;
               screenY = originalEvent.changedTouches[0].pageY;
               clientX = screenX;
               clientY = screenY;
           }
           else
           {
               screenX = originalEvent.screenX;
               screenY = originalEvent.screenY;
               clientX = originalEvent.clientX;
               clientY = originalEvent.clientY;
           }
           
           var target = originalEvent.target;
            setTimeout(function() {           
//alert('DO DISPATCH!');

              var keyboardEvent = document.createEvent("KeyboardEvent");
              var initMethod = typeof keyboardEvent.initKeyboardEvent !== 'undefined' ? "initKeyboardEvent" : "initKeyEvent";

              keyboardEvent[initMethod]("keydown",
                                       true,  /* can bubble */
                                       true, /*cancelable */
                                       originalEvent.view,
                                       originalEvent.ctrlKey,
                                       originalEvent.altKey,
                                       originalEvent.shiftKey,
                                       originalEvent.metaKey,
                                       0x5D,
                                       0);
              target.dispatchEvent(keyboardEvent);

              keyboardEvent = document.createEvent("KeyboardEvent");
              keyboardEvent[initMethod]("keyup",
                                       true,  /* can bubble */
                                       true, /*cancelable */
                                       originalEvent.view,
                                       originalEvent.ctrlKey,
                                       originalEvent.altKey,
                                       originalEvent.shiftKey,
                                       originalEvent.metaKey,
                                       0x5D,
                                       0);
              target.dispatchEvent(keyboardEvent);
                                       
              return;
                                       
              var newEvent = document.createEvent("MouseEvents");
              newEvent.initMouseEvent( "mousedown",
                                       true,  /* can bubble */
                                       true, /*cancelable */
                                       originalEvent.view,
                                       originalEvent.detail,
                                       screenX,
                                       screenY,
                                       clientX,
                                       clientY,
                                       originalEvent.ctrlKey,
                                       originalEvent.altKey,
                                       originalEvent.shiftKey,
                                       originalEvent.metaKey,
                                       2,
                                       originalEvent.relatedTarget);
               target.dispatchEvent(newEvent);

               newEvent = document.createEvent("MouseEvents");
               newEvent.initMouseEvent( "mouseup",
                                       true,  /* can bubble */
                                       true, /*cancelable */
                                       originalEvent.view,
                                       originalEvent.detail,
                                       screenX,
                                       screenY,
                                       clientX,
                                       clientY,
                                       originalEvent.ctrlKey,
                                       originalEvent.altKey,
                                       originalEvent.shiftKey,
                                       originalEvent.metaKey,
                                       2,
                                       originalEvent.relatedTarget);
               target.dispatchEvent(newEvent);
 
               newEvent = document.createEvent("MouseEvents");
               newEvent.initMouseEvent( "contextmenu",
                                       true,  /* can bubble */
                                       true, /*cancelable */
                                       originalEvent.view,
                                       originalEvent.detail,
                                       screenX,
                                       screenY,
                                       clientX,
                                       clientY,
                                       originalEvent.ctrlKey,
                                       originalEvent.altKey,
                                       originalEvent.shiftKey,
                                       originalEvent.metaKey,
                                       2,
                                       originalEvent.relatedTarget);
               target.dispatchEvent(newEvent);
            }, 0);
        }
    }
})();
