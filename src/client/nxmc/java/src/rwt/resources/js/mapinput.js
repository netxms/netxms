/**
 * mapinput.js
 *
 * RAP custom widget handler that routes continuous pointer input (touch pinch,
 * touch drag, mouse wheel, mouse drag) on a map canvas into the map's own pan
 * and zoom on the server side.
 *
 * RAP delivers no server-side SWT.MouseMove or SWT.MouseWheel by documented
 * intent, so a map canvas receives no continuous input stream at all. This
 * handler supplies one, using the same RemoteObject mechanism msgproxy.js uses
 * for mouseHover/mouseExit.
 *
 * It also owns gesture classification inside the map canvas: a pinch or a pan
 * must never open the context menu, and a long press must open it even when
 * held well past the long tap duration. This handler classifies first and
 * suppresses longpress.js for any touch it has claimed - see touchEndCapture().
 *
 * License: MIT License - http://www.opensource.org/licenses/mit-license.php
 * Copyright (c) 2026 Raden Solutions
 */

(function() {
	'use strict';

	if (!window.netxms) {
		window.netxms = {};
	}

	/** Movement in px before a touch stops being a candidate tap/long press. */
	var MOVE_THRESHOLD = 10;

	/**
	 * Long press delay in ms. Deliberately has NO upper cutoff - the menu opens on this timer while
	 * the finger is still down, so holding longer never defeats it. The vendored longpress.js used
	 * 500 ms and fired on release; 500 ms proved too eager once the menu opened on press instead,
	 * and 1000 ms still read slightly quick on hardware.
	 *
	 * Any value above LONGPRESS_JS_THRESHOLD leaves a window in which longpress.js would otherwise
	 * open the menu first, so this constant cannot be raised without the touchEndCapture()
	 * suppression below being functional - see the startTime note there.
	 */
	var LONG_PRESS_DELAY = 1250;

	/**
	 * The threshold longpress.js uses (its own `duration`). It opens the context menu on release for
	 * any touch that lasted at least this long, and we cannot reach into its closure to stop it. So
	 * every touch at least this old must be suppressed here, not merely those our own long press
	 * timer has already claimed - otherwise longpress.js owns the whole window between its threshold
	 * and ours, and raising LONG_PRESS_DELAY only widens the window in which it wins.
	 */
	var LONGPRESS_JS_THRESHOLD = 500;

	/** Wheel notch to zoom factor. */
	var WHEEL_FACTOR = 0.0015;

	/** Clamp for a single coalesced zoom step, so one frame cannot jump wildly. */
	var MAX_STEP_IN = 4.0;
	var MAX_STEP_OUT = 0.25;

	/**
	 * A pinch is shown by scaling the last rendered frame in the browser, and sent to the server only
	 * once the fingers lift or have not moved for this long (ms). Sending every frame of a pinch kept
	 * RAP busy for the whole gesture: each frame is a round trip plus a full canvas redraw, and RAP
	 * drops touch input while it applies a response, so on a phone the pinch lagged and taps were lost.
	 */
	var PREVIEW_SETTLE_DELAY = 300;

	/** Give up on a preview whose redraw never arrives, rather than leave the canvas scaled. */
	var PREVIEW_TIMEOUT = 5000;

	/**
	 * A one-finger pan is previewed the same way, but only the visible part of the map exists in the
	 * browser, so moving it uncovers blank edges. While the finger moves, the pan so far is therefore
	 * sent whenever the previous one has been drawn and the map has moved at least this far (px): the
	 * map follows the finger at once, and the edges fill in at whatever rate the round trip allows.
	 */
	var PAN_COMMIT_DISTANCE = 48;

	/**
	 * Transforms of the viewport offset V (canvas pixels): V' = s * V + (tx, ty). On screen the same
	 * transform moves a canvas point p to s * p - (tx, ty).
	 */
	var IDENTITY = { s : 1.0, tx : 0, ty : 0 };

	/** a, then b */
	var compose = function(a, b) {
		return { s : b.s * a.s, tx : b.s * a.tx + b.tx, ty : b.s * a.ty + b.ty };
	};

	/**
	 * Send whatever RAP has queued now. RAP otherwise holds outgoing events for 60 ms to batch them, which a
	 * tap on the map pays on top of the round trip before the selection shows. Internal RAP API, so failure
	 * just leaves the normal batched send in place.
	 */
	var sendNow = function() {
		try {
			rwt.remote.Connection.getInstance().sendImmediate(true);
		} catch (err) {
		}
	};

	var isIdentity = function(t) {
		return t.s === 1.0 && Math.abs(t.tx) < 0.5 && Math.abs(t.ty) < 0.5;
	};

	rap.registerTypeHandler("netxms.MapInput", {

		factory : function(properties) {
			return new netxms.MapInput(properties);
		},

		destructor : "destroy",

		properties : [ "view" ],

		events : [ "zoom", "pan", "dragpan" ]
	});

	/**
	 * Registry of live MapInput instances, keyed by the RWT id of the widget they serve.
	 *
	 * Listeners are installed ONCE on the document, at script load, rather than on each map's DOM
	 * node when its widget is created. Binding to the node would fail in both directions: a gesture
	 * made before the binding arrived would reach no listener at all and Safari would take it as a
	 * whole page zoom, and a node replaced by a re-render would leave the listeners on a detached
	 * element. Ownership
	 * is therefore resolved per event, by walking up from the event target. While no map exists the
	 * registry is empty and these listeners do nothing anywhere in the UI.
	 */
	var instances = {};
	var listenersInstalled = false;

	/**
	 * Find the MapInput instance owning an event target, if any, and remember the widget node so
	 * that coordinates can be taken relative to it.
	 */
	var instanceCount = 0;

	var findOwner = function(target) {
		if (instanceCount === 0) {
			return null; // no map open anywhere - do not walk the tree on every pointer move
		}
		while (target != null && target !== document) {
			if (target.rwtWidget != null) {
				var owner = instances[target.rwtWidget._rwtId];
				if (owner) {
					owner.node = target;
					return owner;
				}
			}
			target = target.parentNode;
		}
		return null;
	};

	var installListeners = function() {
		if (listenersInstalled || !document.body) {
			return;
		}
		listenersInstalled = true;

		// A previewed pinch stays on screen until the server's redraw for it has been applied. Requests are
		// serialized, so the first render after the request carrying it is that redraw.
		var forEachInstance = function(method) {
			for (var id in instances) {
				if (instances.hasOwnProperty(id)) {
					instances[id][method]();
				}
			}
		};
		rap.on("send", function() {
			forEachInstance("previewSent");
		});
		rap.on("render", function() {
			forEachInstance("previewRendered");
		});

		var dispatch = function(method) {
			return function(e) {
				var owner = findOwner(e.target);
				if (owner != null) {
					owner[method](e);
				}
			};
		};

		// Capture phase throughout, so these run before longpress.js's listener on the touch target
		// and before RAP's own handlers on document.body, which are registered in the bubble phase.
		document.addEventListener("touchstart", dispatch("touchStart"), { capture : true, passive : false });
		document.addEventListener("touchmove", dispatch("touchMove"), { capture : true, passive : false });
		document.addEventListener("touchend", dispatch("touchEndCapture"), { capture : true, passive : false });
		document.addEventListener("touchcancel", dispatch("touchEndCapture"), { capture : true, passive : false });
		document.addEventListener("wheel", dispatch("wheel"), { capture : true, passive : false });
		document.addEventListener("pointerdown", dispatch("pointerDown"), true);
		document.addEventListener("pointermove", dispatch("pointerMove"), true);
		document.addEventListener("pointerup", dispatch("pointerUp"), true);
		document.addEventListener("pointercancel", dispatch("pointerUp"), true);
		document.addEventListener("click", dispatch("clickCapture"), true);

		// Safari raises these alongside the touch stream for a pinch, and they are what actually
		// drive whole page zoom on iOS. Cancel them only over a map we own. Cancelling them
		// unconditionally is precisely the defect reported as eclipse-rap#398, so the ownership
		// test is not optional.
		var cancelGesture = function(e) {
			if (findOwner(e.target) != null) {
				e.preventDefault();
			}
		};
		document.addEventListener("gesturestart", cancelGesture, { capture : true, passive : false });
		document.addEventListener("gesturechange", cancelGesture, { capture : true, passive : false });
		document.addEventListener("gestureend", cancelGesture, { capture : true, passive : false });
	};

	if (document.body) {
		installListeners();
	} else {
		document.addEventListener("DOMContentLoaded", installListeners);
	}

	netxms.MapInput = function(properties) {
		this.parentId = properties.parent;
		this.node = null;
		if (!instances[this.parentId]) {
			instanceCount++;
		}
		instances[this.parentId] = this;
		installListeners();

		// Coalescing state - everything is accumulated here and flushed once per
		// animation frame. The coalescing MUST happen in the browser: each notify
		// is a full client/server round trip, and every zoom change triggers a
		// full viewer refresh on the server side.
		//
		// What is accumulated is one transform of the viewport offset V (canvas pixels):
		// V' = scale * V + (transX, transY). Zoom and pan samples do not commute, so they cannot be
		// accumulated separately (multiplied factors, latest focal point, summed pans) without moving
		// the content away from under the fingers.
		this.frame = null;
		this.scale = 1.0;
		this.transX = 0;
		this.transY = 0;
		this.focalX = 0; // canvas-relative focal point sent with the zoom: that of the first zoom sample of the frame
		this.focalY = 0;
		this.zoomPending = false;

		// Pinch preview. While previewing, the accumulated transform is shown as a CSS transform of the
		// canvas instead of being sent. Committed transforms are tracked until their redraw is applied:
		// first unsent (waiting for RAP's next request), then in flight (waiting for its response).
		this.previewing = false;
		this.previewCanvas = null;
		this.previewUnsent = []; // records of what was sent: { s, tx, ty } as accumulated, and the focal point fx, fy sent with it
		this.previewInFlight = []; // one array of records per request
		this.view = null; // the server's view state for the rendered frame, see setView()
		this.previewSettleTimer = null;
		this.previewTimeoutTimer = null;

		this.resetGesture();
	};

	netxms.MapInput.prototype = {

		/**
		 * True if the event started on the map drawing surface rather than on a
		 * scrollbar or another child widget. Scrollbar dragging already works and
		 * must not be claimed.
		 */
		onCanvas : function(target) {
			while (target != null && target !== this.node) {
				if (target.tagName === "CANVAS") {
					return true;
				}
				target = target.parentNode;
			}
			return false;
		},

		resetGesture : function() {
			this.tracking = false;
			this.pinching = false;
			this.moved = false;
			this.consumed = false;
			this.startX = 0;
			this.startY = 0;
			this.lastX = 0;
			this.lastY = 0;
			this.lastDistance = 0;
			this.startTime = 0;
			this.cancelLongPress();
		},

		cancelLongPress : function() {
			if (this.longPressTimer) {
				clearTimeout(this.longPressTimer);
				this.longPressTimer = null;
			}
		},

		touchStart : function(e) {
			if (e.touches.length > 1) {
				// Second finger down - this is a pinch, never a press.
				this.cancelLongPress();
				if (!this.tracking) {
					// Both fingers landed together closely enough that the first touchstart we
					// saw already carried two touches, so there was never a single touch event
					// to begin tracking on. Without this the gesture goes unclaimed and Safari
					// takes it as a page zoom.
					if (!this.onCanvas(e.target) && !this.onCanvas(e.touches[0].target)) {
						return;
					}
					this.resetGesture();
					this.tracking = true;
					this.startTime = new Date().getTime();
				}
				this.pinching = true;
				this.consumed = true;
				this.lastDistance = this.distance(e.touches);
				var mid = this.midpoint(e.touches);
				this.lastX = mid.x;
				this.lastY = mid.y;
				return;
			}

			if (!this.onCanvas(e.target)) {
				return;
			}

			this.resetGesture();
			this.tracking = true;
			this.startTime = new Date().getTime();
			var t = e.touches[0];
			this.startX = this.lastX = t.clientX;
			this.startY = this.lastY = t.clientY;

			// Our own long press: single touch, below the movement threshold, and
			// with no upper time limit.
			var self = this;
			var target = e.target;
			var x = t.pageX;
			var y = t.pageY;
			this.longPressTimer = setTimeout(function() {
				self.longPressTimer = null;
				if (self.tracking && !self.moved && !self.pinching) {
					self.consumed = true;
					self.openContextMenu(target, x, y);
				}
			}, LONG_PRESS_DELAY);
		},

		touchMove : function(e) {
			if (!this.tracking) {
				return;
			}

			if (e.touches.length > 1) {
				this.cancelLongPress();
				if (!this.previewing) {
					this.startPreview();
				}
				this.pinching = true;
				this.moved = true;
				this.consumed = true;
				var distance = this.distance(e.touches);
				var mid = this.midpoint(e.touches);
				if (this.lastDistance > 0 && distance > 0) {
					// The content that was under the previous midpoint must end up under the new one: zoom about
					// the previous midpoint, then move by how far the midpoint moved.
					this.queueZoom(distance / this.lastDistance, this.lastX, this.lastY);
					this.queuePan(mid.x - this.lastX, mid.y - this.lastY);
				}
				this.lastDistance = distance;
				this.lastX = mid.x;
				this.lastY = mid.y;
				e.preventDefault();
				return;
			}

			var t = e.touches[0];
			if (!this.moved) {
				if (Math.abs(t.clientX - this.startX) < MOVE_THRESHOLD && Math.abs(t.clientY - this.startY) < MOVE_THRESHOLD) {
					return;
				}
				this.moved = true;
				this.consumed = true;
				this.cancelLongPress();
			}
			if (!this.previewing) {
				this.startPreview();
			}

			this.queuePan(t.clientX - this.lastX, t.clientY - this.lastY);
			this.lastX = t.clientX;
			this.lastY = t.clientY;
			e.preventDefault();
		},

		/**
		 * Capture-phase end of touch. If this touch was claimed as a pan, a pinch or a long press we
		 * already handled, stop it here so that longpress.js - which listens on the touch target, and
		 * so would otherwise run first - cannot also act on it.
		 *
		 * The `held` arm is what covers the window between longpress.js's own threshold and ours, in
		 * which the touch is too old for longpress.js to ignore but too young for our timer to have
		 * claimed it. It depends on startTime being stamped in touchStart: without that stamp `held`
		 * is always 0, this arm is dead, and longpress.js owns that entire window.
		 *
		 * A quick tap is below the threshold and is never suppressed, so ordinary tap-to-select and
		 * RAP's click synthesis are untouched. A deliberately SLOW tap is suppressed, and losing its
		 * selection is the known cost of closing the window.
		 */
		touchEndCapture : function(e) {
			var held = (this.startTime > 0) ? (new Date().getTime() - this.startTime) : 0;
			if (this.consumed || held >= LONGPRESS_JS_THRESHOLD) {
				e.stopPropagation();
			}
			// A claimed gesture must not also become a click. Without this the browser follows the final touchend with
			// compatibility mousedown/mouseup/click at the touch point - which is exactly where a long press has just opened
			// the context menu, so the click lands on the menu and closes it.
			if (this.consumed && e.cancelable) {
				e.preventDefault();
			}
			if (e.touches && e.touches.length > 0) {
				if (this.tracking) {
					this.reseedRemainingTouches(e.touches);
				}
				return;
			}
			if (this.previewing) {
				this.commitPreview();
				this.previewing = false;
			} else {
				this.flush();
			}
			this.resetGesture();
		},

		/**
		 * Some fingers lifted while others stay down. Continue the gesture from where the remaining
		 * fingers are now: the stored position and distance belong to the previous set of touches,
		 * and measuring the next move against them makes the map jump by the distance between the
		 * old and new reference points. The gesture stays claimed and long press stays disabled.
		 */
		reseedRemainingTouches : function(touches) {
			if (touches.length > 1) {
				var mid = this.midpoint(touches);
				this.lastX = mid.x;
				this.lastY = mid.y;
				this.lastDistance = this.distance(touches);
			} else {
				this.startX = this.lastX = touches[0].clientX;
				this.startY = this.lastY = touches[0].clientY;
				this.lastDistance = 0;
			}
		},

		openContextMenu : function(target, x, y) {
			setTimeout(function() {
				try {
					var object = rwt.event.EventHandlerUtil.getTargetObject(target);
					var control = rwt.widgets.util.WidgetUtil.getControl(object);
					var contextMenu = control ? control.getContextMenu() : null;
					if (contextMenu != null) {
						contextMenu.setLocation(x, y);
						contextMenu.setOpener(control);
						contextMenu.show();
					}
				} catch (err) {
					console.log(err);
				}
			}, 50);
		},

		wheel : function(e) {
			if (!this.onCanvas(e.target)) {
				return;
			}
			// queueZoom() takes client coordinates and makes them canvas-relative itself
			this.queueZoom(Math.exp(-e.deltaY * WHEEL_FACTOR), e.clientX, e.clientY);
			e.preventDefault();
		},

		/**
		 * A click on the map - a mouse click, or the compatibility mouse events a browser makes from a tap - is
		 * what selects objects. RAP turns it into MouseDown/MouseUp in its own handlers, after this one, and then
		 * holds them for 60 ms; send them as soon as this event has been handled instead.
		 */
		clickCapture : function(e) {
			if (this.onCanvas(e.target)) {
				setTimeout(sendNow, 0);
			}
		},

		pointerDown : function(e) {
			// Shift+drag on the map background is box selection (ExtendedGraphViewer), not a pan
			if (e.pointerType === "touch" || e.button !== 0 || e.shiftKey || !this.onCanvas(e.target)) {
				return;
			}
			this.mousePanning = true;
			this.mouseMoved = false;
			this.lastMouseX = e.clientX;
			this.lastMouseY = e.clientY;
			// The server decides from where a drag started whether it pans the view or moves an object
			var rect = this.node.getBoundingClientRect();
			this.mouseGesture = (this.mouseGesture || 0) + 1;
			this.mouseStartX = Math.round(e.clientX - rect.left);
			this.mouseStartY = Math.round(e.clientY - rect.top);
		},

		pointerMove : function(e) {
			if (!this.mousePanning || e.buttons !== 1) {
				return;
			}
			var dx = e.clientX - this.lastMouseX;
			var dy = e.clientY - this.lastMouseY;
			if (!this.mouseMoved && Math.abs(dx) < MOVE_THRESHOLD && Math.abs(dy) < MOVE_THRESHOLD) {
				return;
			}
			this.mouseMoved = true;
			this.queuePan(dx, dy);
			this.lastMouseX = e.clientX;
			this.lastMouseY = e.clientY;
		},

		pointerUp : function(e) {
			if (this.mousePanning) {
				this.flush(); // still a drag pan
				this.mousePanning = false;
			}
		},

		distance : function(touches) {
			var dx = touches[0].clientX - touches[1].clientX;
			var dy = touches[0].clientY - touches[1].clientY;
			return Math.sqrt(dx * dx + dy * dy);
		},

		midpoint : function(touches) {
			return {
				x : (touches[0].clientX + touches[1].clientX) / 2,
				y : (touches[0].clientY + touches[1].clientY) / 2
			};
		},

		queueZoom : function(factor, x, y) {
			if (!isFinite(factor) || factor <= 0) {
				return;
			}
			var rect = this.node.getBoundingClientRect();
			var fx = x - rect.left;
			var fy = y - rect.top;
			if (!this.zoomPending) {
				this.focalX = Math.round(fx);
				this.focalY = Math.round(fy);
				this.zoomPending = true;
			}
			// Zoom about f: V' = factor * (V + f) - f
			this.scale = this.scale * factor;
			this.transX = factor * this.transX + (factor - 1) * fx;
			this.transY = factor * this.transY + (factor - 1) * fy;
			this.schedule();
		},

		queuePan : function(dx, dy) {
			if (dx === 0 && dy === 0) {
				return;
			}
			// Content follows the pointer, so the viewport offset moves the opposite way
			this.transX -= dx;
			this.transY -= dy;
			this.schedule();
		},

		schedule : function() {
			var self = this;
			if (this.previewing) {
				if (this.previewSettleTimer !== null) {
					clearTimeout(this.previewSettleTimer);
				}
				this.previewSettleTimer = setTimeout(function() {
					self.previewSettleTimer = null;
					self.commitPreview(); // fingers still down but not moving: render at this zoom now
				}, PREVIEW_SETTLE_DELAY);
			}
			if (this.frame !== null) {
				return;
			}
			this.frame = window.requestAnimationFrame(function() {
				self.frame = null;
				if (self.previewing) {
					self.showPreview();
					self.commitPanIfIdle();
				} else {
					self.flush();
				}
			});
		},

		startPreview : function() {
			// Whatever is still accumulated from before the pinch becomes part of the preview
			if (this.frame !== null) {
				window.cancelAnimationFrame(this.frame);
				this.frame = null;
			}
			this.previewing = true;
			var best = null;
			var canvases = this.node.getElementsByTagName("canvas");
			for (var i = 0; i < canvases.length; i++) {
				var r = canvases[i].getBoundingClientRect();
				if (best === null || r.width * r.height > best.area) {
					best = { canvas : canvases[i], area : r.width * r.height };
				}
			}
			this.previewCanvas = (best !== null) ? best.canvas : null;
		},

		/**
		 * Show everything not yet in the rendered frame - in flight, unsent and still accumulating - as one
		 * CSS transform of the canvas. The widget element clips it to the map's area.
		 */
		showPreview : function() {
			var canvas = this.previewCanvas;
			if (canvas === null) {
				return;
			}
			var records = [];
			for (var i = 0; i < this.previewInFlight.length; i++) {
				records = records.concat(this.previewInFlight[i]);
			}
			records = records.concat(this.previewUnsent);
			if (this.previewing) {
				records.push(this.accumulatedRecord());
			}
			var t = (this.view !== null) ? this.replayServer(records) : records.reduce(compose, IDENTITY);
			if (isIdentity(t)) {
				canvas.style.transform = "";
				canvas.style.transformOrigin = "";
				if (!this.previewing) {
					this.previewCanvas = null;
				}
				return;
			}
			// Transform coordinates are relative to the canvas, gesture coordinates to the widget node, which is the
			// canvas's offset parent. offsetLeft/Top are layout positions, unaffected by the transform itself.
			var ox = canvas.offsetLeft;
			var oy = canvas.offsetTop;
			canvas.style.transformOrigin = "0 0";
			canvas.style.transform = "matrix(" + t.s + ",0,0," + t.s + "," + ((t.s - 1) * ox - t.tx) + "," + ((t.s - 1) * oy - t.ty) + ")";
		},

		/**
		 * What is accumulated but not yet sent, as a record like those flush() sends.
		 */
		accumulatedRecord : function() {
			return {
				s : this.scale,
				tx : this.transX,
				ty : this.transY,
				fx : this.zoomPending ? this.focalX : 0,
				fy : this.zoomPending ? this.focalY : 0
			};
		},

		/**
		 * Where the server will put the view once it has applied these records, as a transform from the
		 * rendered frame. Each record is replayed exactly as ExtendedGraphViewer applies it: zoom (clamped to
		 * the zoom range) about the focal point, then pan, then keep the view inside the map. Without this the
		 * preview could go past an edge or a zoom limit and then jump back when the redraw arrived.
		 */
		replayServer : function(records) {
			var v = this.view;
			var z = v.zoom;
			var x = v.x;
			var y = v.y;
			for (var i = 0; i < records.length; i++) {
				var r = records[i];
				var a = Math.min(v.maxZoom, Math.max(v.minZoom, z * r.s)) / z;
				var dx = (r.s - 1) * r.fx - r.tx; // the pan flush() sends with this record
				var dy = (r.s - 1) * r.fy - r.ty;
				x = a * (x + r.fx) - r.fx - dx;
				y = a * (y + r.fy) - r.fy - dy;
				z = z * a;
				x = Math.max(0, Math.min(x, v.w * z - v.vw));
				y = Math.max(0, Math.min(y, v.h * z - v.vh));
			}
			var s = z / v.zoom;
			return { s : s, tx : x - s * v.x, ty : y - s * v.y };
		},

		/**
		 * The server's view state, sent with every response that changes it - so it always describes the
		 * frame the browser has just drawn.
		 */
		setView : function(view) {
			this.view = view;
		},

		/**
		 * Send the previewed transform. The preview stays on screen until the redraw for it has been applied.
		 */
		commitPreview : function() {
			if (this.previewSettleTimer !== null) {
				clearTimeout(this.previewSettleTimer);
				this.previewSettleTimer = null;
			}
			if (this.frame !== null) {
				window.cancelAnimationFrame(this.frame);
				this.frame = null;
			}
			var record = this.accumulatedRecord();
			if (isIdentity(record)) {
				this.showPreview();
				return;
			}
			// Recorded before flush(): it sends at once, and RAP's "send" event - which moves unsent records in flight -
			// fires inside it
			this.previewUnsent.push(record);
			this.flush(true);
			// What was sent is the accumulated transform less the sub-pixel remainder flush() kept back (the record may
			// already be in flight; it is the same object)
			record.tx -= this.transX;
			record.ty -= this.transY;
			this.showPreview();

			var self = this;
			if (this.previewTimeoutTimer !== null) {
				clearTimeout(this.previewTimeoutTimer);
			}
			this.previewTimeoutTimer = setTimeout(function() {
				self.previewTimeoutTimer = null;
				self.previewUnsent = [];
				self.previewInFlight = [];
				self.showPreview();
			}, PREVIEW_TIMEOUT);
		},

		/**
		 * During a one-finger pan, send the pan so far if nothing sent earlier is still waiting to be drawn
		 * and it has moved far enough to uncover a noticeable strip. A pinch is sent only when it settles.
		 */
		commitPanIfIdle : function() {
			if (!this.previewing || this.pinching || this.scale !== 1.0) {
				return;
			}
			if (this.previewInFlight.length > 0 || this.previewUnsent.length > 0) {
				return;
			}
			if (Math.abs(this.transX) >= PAN_COMMIT_DISTANCE || Math.abs(this.transY) >= PAN_COMMIT_DISTANCE) {
				this.commitPreview();
			}
		},

		previewSent : function() {
			if (this.previewUnsent.length > 0) {
				this.previewInFlight.push(this.previewUnsent);
				this.previewUnsent = [];
			}
		},

		previewRendered : function() {
			if (this.previewInFlight.length === 0) {
				return;
			}
			this.previewInFlight.shift();
			if (this.previewInFlight.length === 0 && this.previewUnsent.length === 0 && this.previewTimeoutTimer !== null) {
				clearTimeout(this.previewTimeoutTimer);
				this.previewTimeoutTimer = null;
			}
			this.showPreview();
			this.commitPanIfIdle(); // the finger may have kept moving while that redraw was on its way
		},

		/**
		 * Send at most one zoom and one pan notification per animation frame.
		 *
		 * @param unclamped true for a committed preview: the user has already seen the whole step, so it is
		 *        not limited to MAX_STEP_IN/OUT (the server still clamps to the map's zoom range)
		 */
		flush : function(unclamped) {
			if (this.frame !== null) {
				window.cancelAnimationFrame(this.frame);
				this.frame = null;
			}

			var remote = rap.getRemoteObject(this);

			// The server zooms about the focal point F and then pans by D: V' = scale * (V + F) - F - D.
			// The D that makes this equal the accumulated transform is (scale - 1) * F - trans. It is computed
			// from the scale accumulated, not the one sent: D is the movement left over once the zoom about F is
			// taken out, which does not depend on how far the zoom goes - so it stays right when the step is
			// capped below, or when the server clamps the zoom to the map's range.
			var dx = (this.scale - 1) * (this.zoomPending ? this.focalX : 0) - this.transX;
			var dy = (this.scale - 1) * (this.zoomPending ? this.focalY : 0) - this.transY;
			var sendX = Math.round(dx);
			var sendY = Math.round(dy);
			var sent = true;
			if (this.zoomPending) {
				// One operation, so the server clamps the view to the map once, for the combined result
				remote.notify("zoom", {
					factor : unclamped ? this.scale : Math.min(MAX_STEP_IN, Math.max(MAX_STEP_OUT, this.scale)),
					x : this.focalX,
					y : this.focalY,
					dx : sendX,
					dy : sendY
				});
			} else if ((sendX !== 0 || sendY !== 0) && this.mousePanning) {
				remote.notify("dragpan", {
					dx : sendX,
					dy : sendY,
					g : this.mouseGesture,
					sx : this.mouseStartX,
					sy : this.mouseStartY
				});
			} else if (sendX !== 0 || sendY !== 0) {
				remote.notify("pan", {
					dx : sendX,
					dy : sendY
				});
			} else {
				sent = false;
			}
			if (sent) {
				sendNow();
			}

			// Carry the sub-pixel remainder into the next frame instead of dropping it
			this.scale = 1.0;
			this.zoomPending = false;
			this.transX = sendX - dx;
			this.transY = sendY - dy;
		},

		destroy : function() {
			this.cancelLongPress();
			if (this.previewSettleTimer !== null) {
				clearTimeout(this.previewSettleTimer);
			}
			if (this.previewTimeoutTimer !== null) {
				clearTimeout(this.previewTimeoutTimer);
			}
			if (this.previewCanvas !== null) {
				this.previewCanvas.style.transform = "";
			}
			if (this.frame !== null) {
				window.cancelAnimationFrame(this.frame);
				this.frame = null;
			}
			// The document listeners are shared and stay installed; dropping out of the registry is
			// what stops events being routed here.
			if (instances[this.parentId] === this) {
				delete instances[this.parentId];
				instanceCount--;
			}
			this.node = null;
		}
	};
}());
