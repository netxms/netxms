/**
 * codemirror-widget.js
 *
 * RAP custom widget handler for CodeMirror 6 NXSL editor.
 *
 * License: GPLv2
 * Copyright (c) 2026 Raden Solutions
 */

(function() {
   'use strict';

   if (!window.netxms) {
      window.netxms = {};
   }

   rap.registerTypeHandler("netxms.CodeMirror", {
      factory: function(properties) {
         return new netxms.CodeMirrorWidget(properties);
      },
      destructor: "destroy",
      properties: [
         "text", "readOnly", "lineNumbers",
         "functions", "variables", "constants",
         "errorLines", "warningLines"
      ],
      events: ["textChanged", "textSync", "mouseDown"],
      methods: ["getText"]
   });

   netxms.CodeMirrorWidget = function(properties) {
      var self = this;
      this.parent = rap.getObject(properties.parent);
      this.remoteObjectId = properties.parent;

      // Create container div
      this.element = document.createElement("div");
      this.element.style.position = "absolute";
      this.element.style.left = "0";
      this.element.style.top = "0";
      this.element.style.right = "0";
      this.element.style.bottom = "0";
      this.element.style.overflow = "hidden";
      this.parent.append(this.element);

      // Create CodeMirror editor
      this.editor = window.CodeMirror.createEditor(this.element, {
         text: "",
         lineNumbers: true,
         readOnly: false
      });

      // Change notification with debounce
      this._changeTimer = null;
      this._lastNotifiedText = "";
      this.editor.onChange(function(text) {
         if (self._changeTimer) {
            clearTimeout(self._changeTimer);
         }
         self._changeTimer = setTimeout(function() {
            self._changeTimer = null;
            if (text !== self._lastNotifiedText) {
               self._lastNotifiedText = text;
               var remoteObject = rap.getRemoteObject(self);
               if (remoteObject) {
                  remoteObject.notify("textChanged", { text: text });
               }
            }
         }, 300);
      });

      // Mouse down notification
      this.editor.onMouseDown(function(coords) {
         var remoteObject = rap.getRemoteObject(self);
         if (remoteObject) {
            remoteObject.notify("mouseDown", { x: coords.x, y: coords.y });
         }
      });

      // Prevent editing keys from propagating to RAP dialog default buttons.
      // Use bubble phase (not capture) so CodeMirror processes the event first.
      this.element.addEventListener('keydown', function(e) {
         if (e.key === 'Enter' || e.key === 'Tab') {
            e.stopPropagation();
         }
      }, false);

      // Track parent resizing
      this._resizeObserver = new ResizeObserver(function() {
         // CodeMirror handles its own resize, but we need the container to match
         var parentEl = self.parent.$el || self.parent.getClientArea && self.parent;
         // Container already uses absolute positioning to fill parent
      });
      var parentNode = this.element.parentNode;
      if (parentNode) {
         this._resizeObserver.observe(parentNode);
      }
   };

   netxms.CodeMirrorWidget.prototype = {

      setText: function(text) {
         this.editor.setText(text || "");
         this._lastNotifiedText = text || "";
      },

      setReadOnly: function(readOnly) {
         this.editor.setReadOnly(!!readOnly);
      },

      setLineNumbers: function(show) {
         this.editor.setLineNumbers(!!show);
      },

      setFunctions: function(functions) {
         this.editor.setCompletions({ functions: Array.isArray(functions) ? functions : [] });
      },

      setVariables: function(variables) {
         this.editor.setCompletions({ variables: Array.isArray(variables) ? variables : [] });
      },

      setConstants: function(constants) {
         this.editor.setCompletions({ constants: Array.isArray(constants) ? constants : [] });
      },

      setErrorLines: function(lines) {
         this.editor.setErrorLines(lines || []);
      },

      setWarningLines: function(lines) {
         this.editor.setWarningLines(lines || []);
      },

      getText: function() {
         var text = this.editor.getText();
         var remoteObject = rap.getRemoteObject(this);
         if (remoteObject) {
            remoteObject.notify("textSync", { text: text });
         }
      },

      destroy: function() {
         if (this._changeTimer) {
            clearTimeout(this._changeTimer);
         }
         if (this._resizeObserver) {
            this._resizeObserver.disconnect();
         }
         this.editor.destroy();
         if (this.element.parentNode) {
            this.element.parentNode.removeChild(this.element);
         }
      }
   };
}());
