/**
 * CodeMirror 6 bundle for NetXMS NXSL script editor.
 *
 * Exposes window.CodeMirror with a factory method for creating editor instances.
 */
import { EditorView, keymap, lineNumbers, highlightActiveLineGutter,
         highlightSpecialChars, drawSelection, highlightActiveLine,
         rectangularSelection, crosshairCursor } from '@codemirror/view';
import { EditorState, Compartment } from '@codemirror/state';
import { defaultHighlightStyle, syntaxHighlighting, indentOnInput,
         bracketMatching, foldGutter, foldKeymap } from '@codemirror/language';
import { defaultKeymap, history, historyKeymap, indentWithTab } from '@codemirror/commands';
import { searchKeymap, highlightSelectionMatches } from '@codemirror/search';
import { autocompletion, completionKeymap, acceptCompletion } from '@codemirror/autocomplete';
import { tags as t } from '@lezer/highlight';
import { HighlightStyle } from '@codemirror/language';
import { nxslLanguage, KEYWORDS, OPERATOR_KEYWORDS, BUILTIN_TYPES } from './nxsl-mode.js';

// Light theme colors matching NetXMS UI
var nxslHighlightStyle = HighlightStyle.define([
   { tag: t.keyword, color: '#7f0055', fontWeight: 'bold' },
   { tag: t.operatorKeyword, color: '#7f0055', fontWeight: 'bold' },
   { tag: t.typeName, color: '#7f0055', fontWeight: 'bold' },
   { tag: t.literal, color: '#7f0055', fontWeight: 'bold' },
   { tag: t.string, color: '#2a00ff' },
   { tag: t.number, color: '#1a6800' },
   { tag: [t.lineComment, t.blockComment], color: '#3f7f5f' },
   { tag: t.meta, color: '#808000' },
   { tag: t.special(t.variableName), color: '#6a5acd' },
   { tag: t.operator, color: '#333333' },
   { tag: t.variableName, color: '#000000' },
]);

// Editor theme (UI chrome, not syntax)
var editorTheme = EditorView.theme({
   '&': {
      fontSize: '13px',
      height: '100%',
   },
   '.cm-content': {
      fontFamily: '"Source Code Pro", "Consolas", "Monaco", monospace',
   },
   '.cm-gutters': {
      backgroundColor: '#f5f5f5',
      borderRight: '1px solid #ddd',
      color: '#999',
   },
   '.cm-activeLineGutter': {
      backgroundColor: '#e8e8e8',
   },
   '.cm-activeLine': {
      backgroundColor: '#f0f4ff',
   },
   '.cm-nxsl-error-line': {
      backgroundColor: 'rgba(255, 0, 0, 0.15)',
   },
   '.cm-nxsl-warning-line': {
      backgroundColor: 'rgba(224, 224, 0, 0.25)',
   },
   '.cm-nxsl-error-gutter': {
      color: '#ff0000',
      fontWeight: 'bold',
   },
   '.cm-nxsl-warning-gutter': {
      color: '#cc8800',
      fontWeight: 'bold',
   },
   '&.cm-focused': {
      outline: 'none',
   },
   '.cm-scroller': {
      overflow: 'auto',
   },
});

// Gutter marker classes for error/warning indicators
class ErrorGutterMarker {
   constructor() { this.elementClass = 'cm-nxsl-error-gutter'; }
   toDOM() {
      var el = document.createElement('span');
      el.textContent = '\u25cf'; // filled circle
      return el;
   }
   eq(other) { return other instanceof ErrorGutterMarker; }
}

class WarningGutterMarker {
   constructor() { this.elementClass = 'cm-nxsl-warning-gutter'; }
   toDOM() {
      var el = document.createElement('span');
      el.textContent = '\u25cf';
      return el;
   }
   eq(other) { return other instanceof WarningGutterMarker; }
}

/**
 * Create a CodeMirror editor instance.
 *
 * @param {HTMLElement} parentElement - DOM element to attach editor to
 * @param {Object} options - Initial options
 * @returns {Object} Editor handle with control methods
 */
function createEditor(parentElement, options) {
   options = options || {};

   var readOnlyComp = new Compartment();
   var lineNumbersComp = new Compartment();
   var completionsComp = new Compartment();
   var changeCallback = null;

   // Completion data (updated from server)
   var completionData = {
      functions: [],
      variables: [],
      constants: []
   };

   // Build static keyword completions once
   var keywordCompletions = [];
   KEYWORDS.forEach(function(k) {
      keywordCompletions.push({ label: k, type: 'keyword' });
   });
   OPERATOR_KEYWORDS.forEach(function(k) {
      keywordCompletions.push({ label: k, type: 'keyword' });
   });
   BUILTIN_TYPES.forEach(function(k) {
      keywordCompletions.push({ label: k, type: 'type' });
   });

   function nxslCompletions(context) {
      var word = context.matchBefore(/[A-Za-z_$:][A-Za-z_$0-9:]*/);
      if (!word && !context.explicit) return null;
      var options = keywordCompletions.slice();
      completionData.functions.forEach(function(f) {
         options.push({ label: f, type: 'function' });
      });
      completionData.variables.forEach(function(v) {
         options.push({ label: v, type: 'variable' });
      });
      completionData.constants.forEach(function(c) {
         options.push({ label: c, type: 'constant' });
      });
      return {
         from: word ? word.from : context.pos,
         options: options,
         validFor: /^[A-Za-z_$:][A-Za-z_$0-9:]*$/
      };
   }

   var extensions = [
      lineNumbersComp.of(options.lineNumbers !== false ? lineNumbers() : []),
      highlightActiveLineGutter(),
      highlightSpecialChars(),
      history(),
      drawSelection(),
      EditorState.allowMultipleSelections.of(true),
      indentOnInput(),
      syntaxHighlighting(defaultHighlightStyle, { fallback: true }),
      syntaxHighlighting(nxslHighlightStyle),
      bracketMatching(),
      rectangularSelection(),
      crosshairCursor(),
      highlightActiveLine(),
      highlightSelectionMatches(),
      readOnlyComp.of(EditorState.readOnly.of(!!options.readOnly)),
      completionsComp.of(autocompletion({
         override: [nxslCompletions],
         activateOnTyping: true,
      })),
      keymap.of([
         ...defaultKeymap,
         ...historyKeymap,
         ...foldKeymap,
         ...searchKeymap,
         ...completionKeymap,
         indentWithTab,
      ]),
      EditorView.updateListener.of(function(update) {
         if (update.docChanged && changeCallback) {
            changeCallback(update.state.doc.toString());
         }
      }),
      nxslLanguage,
      editorTheme,
   ];

   var state = EditorState.create({
      doc: options.text || '',
      extensions: extensions,
   });

   var view = new EditorView({
      state: state,
      parent: parentElement,
   });

   // Line decoration support for error/warning highlighting
   var errorLines = [];
   var warningLines = [];

   function updateLineDecorations() {
      requestAnimationFrame(function() {
         // Remove existing markers
         var existing = parentElement.querySelectorAll('.cm-nxsl-error-line, .cm-nxsl-warning-line');
         existing.forEach(function(el) {
            el.classList.remove('cm-nxsl-error-line', 'cm-nxsl-warning-line');
         });

         var cmLines = parentElement.querySelectorAll('.cm-line');
         errorLines.forEach(function(lineNum) {
            if (lineNum >= 1 && lineNum <= cmLines.length) {
               cmLines[lineNum - 1].classList.add('cm-nxsl-error-line');
            }
         });
         warningLines.forEach(function(lineNum) {
            if (lineNum >= 1 && lineNum <= cmLines.length) {
               cmLines[lineNum - 1].classList.add('cm-nxsl-warning-line');
            }
         });
      });
   }

   return {
      /**
       * Set editor content, replacing all existing text.
       */
      setText: function(text) {
         view.dispatch({
            changes: { from: 0, to: view.state.doc.length, insert: text || '' }
         });
      },

      /**
       * Get current editor content.
       */
      getText: function() {
         return view.state.doc.toString();
      },

      /**
       * Set read-only state.
       */
      setReadOnly: function(readOnly) {
         view.dispatch({
            effects: readOnlyComp.reconfigure(EditorState.readOnly.of(!!readOnly))
         });
      },

      /**
       * Show or hide line numbers.
       */
      setLineNumbers: function(show) {
         view.dispatch({
            effects: lineNumbersComp.reconfigure(show ? lineNumbers() : [])
         });
      },

      /**
       * Update autocompletion data.
       */
      setCompletions: function(data) {
         if (data.functions) completionData.functions = data.functions;
         if (data.variables) completionData.variables = data.variables;
         if (data.constants) completionData.constants = data.constants;
      },

      /**
       * Set error line numbers (1-based).
       */
      setErrorLines: function(lines) {
         errorLines = lines || [];
         updateLineDecorations();
      },

      /**
       * Set warning line numbers (1-based).
       */
      setWarningLines: function(lines) {
         warningLines = lines || [];
         updateLineDecorations();
      },

      /**
       * Register a change callback.
       */
      onChange: function(callback) {
         changeCallback = callback;
      },

      /**
       * Register a mouse down callback.
       */
      onMouseDown: function(callback) {
         view.dom.addEventListener('mousedown', function(e) {
            callback({ x: e.clientX, y: e.clientY });
         });
      },

      /**
       * Focus the editor.
       */
      focus: function() {
         view.focus();
      },

      /**
       * Get the EditorView instance for advanced usage.
       */
      getView: function() {
         return view;
      },

      /**
       * Destroy the editor.
       */
      destroy: function() {
         view.destroy();
      }
   };
}

// Expose globally for RAP widget
window.CodeMirror = {
   createEditor: createEditor
};
