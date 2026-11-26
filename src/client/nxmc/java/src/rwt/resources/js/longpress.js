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
	var duration = 500;
	
	/** Movement in px after which a touch is a drag or scroll, never a long tap. */
	var moveThreshold = 10;

	/**
	 * The one touch currently being timed. Only one is ever kept: the touchend listener is on the touch
	 * target, so when another handler stops that touchend (the map's gesture input does, for pinches and
	 * pans) the listener never runs and never removes itself - and the next touch to end on the same
	 * element would then fire it with the old start time and open a context menu nobody asked for.
	 */
	var pending = null;

	var body = document.body;
	body.addEventListener('touchstart', touchStartCallback);
	body.addEventListener('touchmove', touchMoveCallback);

	function cancelPending() {
		if (pending != null) {
			pending.target.removeEventListener('touchend', pending.callback);
			clearTimeout(pending.timeout);
			pending = null;
		}
	}

	function touchStartCallback(e) {
		cancelPending();

		// A second finger makes this a pinch, which is never a long tap; the touches that remain after it are
		// not timed either, because they do not start again
		if (e.touches.length > 1) {
			return;
		}

		var startTime = new Date().getTime();
		var touch = e.touches[0];
		var target = e.target;

		target.addEventListener('touchend', touchEndCallback);

		// cancel after too long wait
		var timeout = setTimeout(cancelPending, duration * 4);

		pending = { target: target, callback: touchEndCallback, timeout: timeout, x: touch.clientX, y: touch.clientY };

		function touchEndCallback(e) {
			var tapDuration = new Date().getTime() - startTime;
			cancelPending();

			// if touch is released before long tap duration, it was an ordinary tap
			if (tapDuration >= duration) {
				openContextMenu(e);
			}
		}

		function openContextMenu(originalEvent) {
			var screenX = originalEvent.changedTouches[0].pageX;
			var screenY = originalEvent.changedTouches[0].pageY;

			var target = originalEvent.target;
			setTimeout(
					function() {
						var object = rwt.event.EventHandlerUtil.getTargetObject(target);
						var control = rwt.widgets.util.WidgetUtil.getControl(object);
						var contextMenu = control ? control.getContextMenu() : null;
						if (contextMenu != null) {
							contextMenu.setLocation(screenX, screenY);
							contextMenu.setOpener(control);
							contextMenu.show();
						}
					}, 50);
		}
	}

	// A touch that moves is a drag or a scroll, and a second finger is a pinch: neither is a long tap
	function touchMoveCallback(e) {
		if (pending == null) {
			return;
		}
		var touch = e.touches[0];
		if (e.touches.length > 1 || Math.abs(touch.clientX - pending.x) >= moveThreshold || Math.abs(touch.clientY - pending.y) >= moveThreshold) {
			cancelPending();
		}
	}
})();
