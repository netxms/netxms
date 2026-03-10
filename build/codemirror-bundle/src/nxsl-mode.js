/**
 * NXSL (NetXMS Scripting Language) mode for CodeMirror 6
 *
 * StreamLanguage-based tokenizer. Token categories derived from
 * the tree-sitter-nxsl grammar (https://github.com/netxms/tree-sitter-nxsl).
 */
import { StreamLanguage } from '@codemirror/language';
import { tags as t } from '@lezer/highlight';

export const KEYWORDS = new Set([
   'abort', 'array', 'break', 'case', 'catch', 'const', 'continue',
   'default', 'do', 'else', 'exit', 'for', 'foreach', 'function',
   'global', 'if', 'import', 'local', 'new', 'return', 'select',
   'switch', 'try', 'use', 'when', 'while', 'with'
]);

export const OPERATOR_KEYWORDS = new Set([
   'and', 'or', 'not', 'like', 'ilike', 'match', 'imatch', 'in', 'has'
]);

export const BUILTIN_TYPES = new Set([
   'boolean', 'int32', 'int64', 'uint32', 'uint64', 'real', 'string'
]);

const LITERALS = new Set([
   'true', 'false', 'TRUE', 'FALSE', 'null', 'NULL'
]);

function tokenBase(stream, state) {
   // Whitespace
   if (stream.eatSpace()) return null;

   var ch = stream.peek();

   // Single-line comment
   if (ch === '/' && stream.match('//')) {
      stream.skipToEnd();
      return 'lineComment';
   }

   // Block comment start
   if (ch === '/' && stream.match('/*')) {
      state.tokenize = tokenBlockComment;
      return tokenBlockComment(stream, state);
   }

   // Triple-quoted string
   if (ch === '"' && stream.match('"""')) {
      state.tokenize = tokenTripleString;
      return tokenTripleString(stream, state);
   }

   // F-string
   if ((ch === 'F' || ch === 'f') && stream.peek() !== undefined) {
      var pos = stream.pos;
      stream.next();
      if (stream.peek() === '"') {
         stream.next();
         state.tokenize = tokenFString;
         return 'string';
      }
      stream.pos = pos;
   }

   // Double-quoted string
   if (ch === '"') {
      stream.next();
      state.tokenize = tokenDoubleString;
      return tokenDoubleString(stream, state);
   }

   // Single-quoted string
   if (ch === '\'') {
      stream.next();
      while (!stream.eol()) {
         if (stream.next() === '\'') break;
      }
      return 'string';
   }

   // Metadata
   if (ch === '@') {
      stream.next();
      if (stream.match('meta') || stream.match('optional')) {
         return 'meta';
      }
      return 'operator';
   }

   // Storage item
   if (ch === '#') {
      stream.next();
      if (stream.match(/^[A-Za-z_$][A-Za-z_$0-9]*/)) {
         return 'special';
      }
      return 'operator';
   }

   // Numbers
   if (ch === '0' && stream.match(/^0x[0-9A-Fa-f]+(?:U?L?|LU)?/i)) {
      return 'number';
   }
   if (/[0-9]/.test(ch)) {
      stream.match(/^[0-9]*\.?[0-9]*(?:U?L?|LU)?/i);
      return 'number';
   }

   // Identifiers and keywords
   if (/[A-Za-z_$]/.test(ch)) {
      stream.match(/^[A-Za-z_$0-9]*/);
      // Check for compound identifier (e.g. Base64::Encode)
      while (stream.match('::')) {
         if (!stream.match(/^[A-Za-z_$][A-Za-z_$0-9]*/)) {
            stream.match('*'); // wildcard import
            break;
         }
      }
      var word = stream.current();
      // Strip compound prefix for keyword check
      var base = word.indexOf('::') >= 0 ? word.split('::').pop() : word;

      if (KEYWORDS.has(word)) return 'keyword';
      if (OPERATOR_KEYWORDS.has(word)) return 'operatorKeyword';
      if (BUILTIN_TYPES.has(word)) return 'typeName';
      if (LITERALS.has(word)) return 'literal';
      return 'variable';
   }

   // Operators
   stream.next();
   return 'operator';
}

function tokenBlockComment(stream, state) {
   while (!stream.eol()) {
      if (stream.match('*/')) {
         state.tokenize = tokenBase;
         return 'blockComment';
      }
      stream.next();
   }
   return 'blockComment';
}

function tokenTripleString(stream, state) {
   while (!stream.eol()) {
      if (stream.match('"""')) {
         state.tokenize = tokenBase;
         return 'string';
      }
      stream.next();
   }
   return 'string';
}

function tokenDoubleString(stream, state) {
   while (!stream.eol()) {
      var ch = stream.next();
      if (ch === '\\') {
         stream.next(); // skip escaped char
      } else if (ch === '"') {
         state.tokenize = tokenBase;
         return 'string';
      }
   }
   // Unterminated string - reset to base
   state.tokenize = tokenBase;
   return 'string';
}

function tokenFString(stream, state) {
   while (!stream.eol()) {
      var ch = stream.next();
      if (ch === '\\') {
         stream.next();
      } else if (ch === '{') {
         state.tokenize = tokenFStringExpr;
         state.braceDepth = 1;
         return 'string';
      } else if (ch === '"') {
         state.tokenize = tokenBase;
         return 'string';
      }
   }
   state.tokenize = tokenBase;
   return 'string';
}

function tokenFStringExpr(stream, state) {
   while (!stream.eol()) {
      var ch = stream.next();
      if (ch === '{') {
         state.braceDepth++;
      } else if (ch === '}') {
         state.braceDepth--;
         if (state.braceDepth === 0) {
            state.tokenize = tokenFString;
            return 'variable';
         }
      }
   }
   return 'variable';
}

export const nxslLanguage = StreamLanguage.define({
   name: 'nxsl',

   startState: function() {
      return { tokenize: tokenBase, braceDepth: 0 };
   },

   token: function(stream, state) {
      return state.tokenize(stream, state);
   },

   languageData: {
      commentTokens: { line: '//', block: { open: '/*', close: '*/' } },
      closeBrackets: { brackets: ['(', '[', '{', '"', '\''] },
   },

   tokenTable: {
      keyword: t.keyword,
      operatorKeyword: t.operatorKeyword,
      typeName: t.typeName,
      literal: t.literal,
      string: t.string,
      number: t.number,
      lineComment: t.lineComment,
      blockComment: t.blockComment,
      meta: t.meta,
      special: t.special(t.variableName),
      operator: t.operator,
      variable: t.variableName,
   }
});
