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
	
	var body = document.body;
	body.addEventListener('touchstart', touchStartCallback);

	function touchStartCallback(e) {
		var startTime = new Date().getTime();

		e.target.addEventListener('touchend', touchEndCallback);

		// cancel after too long wait
		var timeout = setTimeout(function() {
			e.target.removeEventListener('touchend', touchEndCallback);
		}, duration * 4);

		function touchEndCallback(e) {
			var tapDuration = new Date().getTime() - startTime;

			// if touch is released before long tap duration, clear the timeout.
			if (tapDuration < duration) {
				clearTimeout(timeout);
			} else {
				openContextMenu(e);
			}

			e.target.removeEventListener('touchend', touchEndCallback);
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
})();
