/**
 * msgproxy.js
 * 
 * License: MIT License - http://www.opensource.org/licenses/mit-license.php
 * Copyright (c) 2018 Raden Solutions
 */

(function() {
	'use strict';
 
	if (!window.netxms) {
		window.netxms = {};
	}
	
	rap.registerTypeHandler("netxms.MsgProxy", {
 
		factory : function(properties) {
			return new netxms.MsgProxy(properties);
		},
 
		destructor : "destroy",
 
		properties : [ ],
 
		events : [ "mouseExit", "mouseHover" ]
	});
 
	netxms.MsgProxy = function(properties) {
		try {
			bindAll(this, [ "layout", "onReady", "onSend", "onRender" ]);
			this.parent = rap.getObject(properties.parent);
			this.element = document.createElement("div");
			this.parent.append(this.element);
		} catch (e) {
			console.log(e);
		}
	};
 
	netxms.MsgProxy.prototype = {
		ready : false,

		onReady : function() {
			this.ready = true;
			this.layout();
		},
		
		onRender : function() {
			if (this.element.parentNode) {
				this.onReady();
				rap.on("send", this.onSend);
			}
		},
 
		onSend : function() {
		},
		
		query : function(q) {
			if (typeof this.data == 'undefined') {
				this.data = {
					results : []
				};
			}
 
			q.callback(this.data);
		},
 
		format : function(item) {
			return item.text;
		},
 
		destroy : function() {
			rap.off("send", this.onSend);
 			this.element.parentNode.removeChild(this.element);
		},
 
		layout : function() {
			if (this.ready) {
				var area = this.parent.getClientArea();
				this.element.style.left = "0px";
				this.element.style.top = "0px";
				this.element.style.right = "0px";
				this.element.style.bottom = "0px";
			}
		}
	};
 
	var bind = function(context, method) {
		return function() {
			return method.apply(context, arguments);
		};
	};
 
	var bindAll = function(context, methodNames) {
		for ( var i = 0; i < methodNames.length; i++) {
			var method = context[methodNames[i]];
			context[methodNames[i]] = bind(context, method);
		}
	};
}());
