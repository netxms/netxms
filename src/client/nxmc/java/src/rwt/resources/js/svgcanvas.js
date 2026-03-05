/**
 * svgcanvas.js
 *
 * RAP custom widget handler for native browser SVG rendering.
 *
 * License: MIT License - http://www.opensource.org/licenses/mit-license.php
 * Copyright (c) 2026 Raden Solutions
 */

(function() {
	'use strict';

	if (!window.netxms) {
		window.netxms = {};
	}

	rap.registerTypeHandler("netxms.SVGCanvas", {

		factory : function(properties) {
			return new netxms.SVGCanvas(properties);
		},

		destructor : "destroy",

		properties : [ "svgContent", "svgColor" ]
	});

	netxms.SVGCanvas = function(properties) {
		this.parent = rap.getObject(properties.parent);
		this.element = document.createElement("div");
		this.element.style.position = "absolute";
		this.element.style.left = "0";
		this.element.style.top = "0";
		this.element.style.right = "0";
		this.element.style.bottom = "0";
		this.element.style.display = "flex";
		this.element.style.alignItems = "center";
		this.element.style.justifyContent = "center";
		this.element.style.overflow = "hidden";
		this.element.style.pointerEvents = "none";
		this.parent.append(this.element);
		this.svgColor = null;
	};

	netxms.SVGCanvas.prototype = {

		setSvgContent : function(content) {
			this.element.innerHTML = content;
			var svg = this.element.querySelector("svg");
			if (svg) {
				svg.removeAttribute("width");
				svg.removeAttribute("height");
				svg.style.width = "100%";
				svg.style.height = "100%";
				if (this.svgColor) {
					svg.style.fill = this.svgColor;
				}
			}
		},

		setSvgColor : function(color) {
			this.svgColor = color;
			var svg = this.element.querySelector("svg");
			if (svg) {
				svg.style.fill = color;
			}
		},

		destroy : function() {
			this.element.parentNode.removeChild(this.element);
		}
	};
}());
