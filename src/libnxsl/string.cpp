/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: string.cpp
**/

#include "libnxsl.h"

/**
 * String functions
 */
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

/**
 * Compare two strings
 */
static int SM_compareTo(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;
   *result = vm->createValue(_tcscmp(thisString->getValueAsCString(), argv[0]->getValueAsCString()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Compare two strings in case-insensitive mode
 */
static int SM_compareToIgnoreCase(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;
   *result = vm->createValue(_tcsicmp(thisString->getValueAsCString(), argv[0]->getValueAsCString()));
   return NXSL_ERR_SUCCESS;
}

/**
 * Check if one string contains another
 */
static int SM_contains(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->createValue(_tcsstr(thisString->getValueAsCString(), argv[0]->getValueAsCString()) != nullptr);
   return NXSL_ERR_SUCCESS;
}

/**
 * Check if this string, interpreted as element list, contains given element
 *
 * containsListElement(separator, value)
 *
 * Examples:
 *   "1,2,3".containsListElement(",", "1") -> true
 *   "ab|cd|ef".containsListElement("|", "test") -> false
 */
int SM_containsListElement(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   bool success = false;
   if ((thisString->getValueAsCString()[0] != 0) && (argv[0]->getValueAsCString()[0] != 0) && (argv[1]->getValueAsCString()[0] != 0))
   {
      const TCHAR *value = argv[1]->getValueAsCString();
      String::split(thisString->getValueAsCString(), argv[0]->getValueAsCString(), true,
         [value, &success] (const String& element) -> void
         {
            if (!_tcscmp(value, element))
               success = true;
         });
   }
   *result = vm->createValue(success);
   return 0;
}

/**
 * Check if string ends with another string
 */
static int SM_endsWith(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t len, slen;
   const TCHAR *s = thisString->getValueAsString(&len);
   const TCHAR *suffix = argv[0]->getValueAsString(&slen);
   if (len >= slen)
   {
      *result = vm->createValue(memcmp(&s[len - slen], suffix, slen * sizeof(TCHAR)) == 0);
   }
   else
   {
      *result = vm->createValue(false);
   }

   return NXSL_ERR_SUCCESS;
}

/**
 * Check if two strings are equal
 */
static int SM_equals(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t l1, l2;
   const TCHAR *s1 = thisString->getValueAsString(&l1);
   const TCHAR *s2 = argv[0]->getValueAsString(&l2);
   *result = vm->createValue((l1 == l2) && (memcmp(s1, s2, l1 * sizeof(TCHAR)) == 0));
   return NXSL_ERR_SUCCESS;
}

/**
 * Check if two strings are equal in case-insensitive mode
 */
static int SM_equalsIgnoreCase(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t l1, l2;
   const TCHAR *s1 = thisString->getValueAsString(&l1);
   const TCHAR *s2 = argv[0]->getValueAsString(&l2);
   *result = vm->createValue((l1 == l2) && (_tcsicmp(s1, s2) == 0));
   return NXSL_ERR_SUCCESS;
}

/**
 * Check if two strings are equal using fuzzy comparison
 */
static int SM_fuzzyEquals(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   double threshold = (argc == 2) ? argv[1]->getValueAsReal() : 0.2;

   const TCHAR *s1 = thisString->getValueAsCString();
   const TCHAR *s2 = argv[0]->getValueAsCString();
   *result = vm->createValue(FuzzyMatchStrings(s1, s2, threshold));
   return NXSL_ERR_SUCCESS;
}

/**
 * Check if two strings are equal in case-insensitive mode using fuzzy comparison
 */
static int SM_fuzzyEqualsIgnoreCase(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   double threshold = (argc == 2) ? argv[1]->getValueAsReal() : 0.2;

   const TCHAR *s1 = thisString->getValueAsCString();
   const TCHAR *s2 = argv[0]->getValueAsCString();
   *result = vm->createValue(FuzzyMatchStringsIgnoreCase(s1, s2, threshold));
   return NXSL_ERR_SUCCESS;
}

/**
 * Common implementation for indexOf and lastIndexOf methods
 */
static int IndexOfCommon(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm, bool reverse)
{
   if ((argc != 1) && (argc != 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t strLength, substrLength;
   const TCHAR *str = thisString->getValueAsString(&strLength);
   const TCHAR *substr = argv[0]->getValueAsString(&substrLength);

   int start;
   if (argc == 2)
   {
      if (!argv[1]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      start = argv[1]->getValueAsInt32();
      if (start >= 0)
      {
         if (reverse && (start > (int)strLength - (int)substrLength))
            start = (int)strLength - (int)substrLength;
      }
      else
      {
         start = reverse ? (int)strLength - (int)substrLength : 0;
      }
   }
   else
   {
      start = reverse ? (int)strLength - (int)substrLength : 0;
   }

   int32_t index = -1;   // -1 = not found
   if ((substrLength <= strLength) && (substrLength > 0))
   {
      if (reverse)
      {
         for(int i = start; i >= 0; i--)
         {
            if (!memcmp(&str[i], substr, substrLength * sizeof(TCHAR)))
            {
               index = i;
               break;
            }
         }
      }
      else
      {
         for(int i = start; i <= (int)(strLength - substrLength); i++)
         {
            if (!memcmp(&str[i], substr, substrLength * sizeof(TCHAR)))
            {
               index = i;
               break;
            }
         }
      }
   }
   else if ((substrLength == strLength) && (substrLength > 0))
   {
      index = !memcmp(str, substr, substrLength * sizeof(TCHAR)) ? 0 : -1;
   }

   *result = vm->createValue(index);
   return NXSL_ERR_SUCCESS;
}

/**
 * Find first occurence of substring within string
 */
static int SM_indexOf(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   return IndexOfCommon(thisString, argc, argv, result, vm, false);
}

/**
 * Find last occurence of substring within string
 */
static int SM_lastIndexOf(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   return IndexOfCommon(thisString, argc, argv, result, vm, true);
}

/**
 * left() - take leftmost part of a string and pad or truncate it as necessary
 * Format: left(len, [pad])
 */
int SM_left(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc == 2) && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   int newLen = argv[0]->getValueAsInt32();
   if (newLen > 0)
   {
      TCHAR pad;
      if (argc == 2)
      {
         pad = *(argv[1]->getValueAsCString());
         if (pad == 0)
            pad = _T(' ');
      }
      else
      {
         pad = _T(' ');
      }

      uint32_t len;
      const TCHAR *str = thisString->getValueAsString(&len);
      if (len > (uint32_t)newLen)
         len = (uint32_t)newLen;
      TCHAR *newStr = MemAllocString(newLen);
      memcpy(newStr, str, len * sizeof(TCHAR));
      for(uint32_t i = len; i < (uint32_t)newLen; i++)
         newStr[i] = pad;
      *result = vm->createValue(newStr, newLen);
      MemFree(newStr);
   }
   else
   {
      *result = vm->createValue(_T(""));
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * replace(target, replacement)
 * Replaces each substring of this string that matches the literal target sequence with the specified literal replacement
 * sequence. The replacement proceeds from the beginning of the string to the end, for example, replacing "aa" with "b"
 * in the string "aaa" will result in "ba" rather than "ab".
 */
static int SM_replace(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   StringBuffer sb(thisString->getValueAsCString());
   sb.replace(argv[0]->getValueAsCString(), argv[1]->getValueAsCString());
   *result = vm->createValue(sb);
   return 0;
}

/**
 * right() - take rightmost part of a string and pad or truncate it as necessary
 * Format: right(len, [pad])
 */
int SM_right(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc == 2) && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   int newLen = argv[0]->getValueAsInt32();
   if (newLen > 0)
   {
      TCHAR pad;
      if (argc == 2)
      {
         pad = *(argv[1]->getValueAsCString());
         if (pad == 0)
            pad = _T(' ');
      }
      else
      {
         pad = _T(' ');
      }

      uint32_t len, shift;
      const TCHAR *str = thisString->getValueAsString(&len);
      if (len > (uint32_t)newLen)
      {
         shift = len - (uint32_t)newLen;
         len = (uint32_t)newLen;
      }
      else
      {
         shift = 0;
      }
      TCHAR *newStr = MemAllocString(newLen);
      memcpy(&newStr[(uint32_t)newLen - len], &str[shift], len * sizeof(TCHAR));
      for(uint32_t i = 0; i < (uint32_t)newLen - len; i++)
         newStr[i] = pad;
      *result = vm->createValue(newStr, newLen);
      MemFree(newStr);
   }
   else
   {
      *result = vm->createValue(_T(""));
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Split string into array of strings at given delimiter
 */
int SM_split(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Array *a = new NXSL_Array(vm);
   String::split(thisString->getValueAsCString(), argv[0]->getValueAsCString(), (argc > 1) && argv[1]->isTrue(),
      [a, vm] (const String& s)
      {
         a->append(vm->createValue(s));
      });
   *result = vm->createValue(a);
   return NXSL_ERR_SUCCESS;
}

/**
 * Check if string starts with another string
 */
static int SM_startsWith(NXSL_Value* thisString, int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t len, plen;
   const TCHAR *s = thisString->getValueAsString(&len);
   const TCHAR *prefix = argv[0]->getValueAsString(&plen);
   if (len >= plen)
   {
      *result = vm->createValue(memcmp(s, prefix, plen * sizeof(TCHAR)) == 0);
   }
   else
   {
      *result = vm->createValue(false);
   }

   return NXSL_ERR_SUCCESS;
}

/**
 * Get substring of a string
 * Possible usage:
 *    substring(start)    - all characters from position 'start'
 *    substring(start, n) - n characters from position 'start'
 *    substring(NULL, n)  - first n characters
 */
static int SM_substring(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   int start;
   if (argv[0]->isNull())
   {
      start = 0;
   }
   else if (argv[0]->isInteger())
   {
      start = argv[0]->getValueAsInt32();
      if (start < 0)
         start = 0;
   }
   else
   {
      return NXSL_ERR_NOT_INTEGER;
   }

   int count;
   if (argc == 2)
   {
      if (!argv[1]->isInteger())
         return NXSL_ERR_NOT_INTEGER;
      count = argv[1]->getValueAsInt32();
      if (count < 0)
         count = 0;
   }
   else
   {
      count = -1;
   }

   uint32_t len;
   const TCHAR *base = thisString->getValueAsString(&len);
   if ((uint32_t)start < len)
   {
      base += start;
      len -= start;
      if ((count == -1) || ((uint32_t)count > len))
      {
         count = len;
      }
      *result = vm->createValue(base, (uint32_t)count);
   }
   else
   {
      *result = vm->createValue(_T(""));
   }

   return NXSL_ERR_SUCCESS;
}

/**
 * Convert string to lower case
 */
static int SM_toLowerCase(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return F_lower(1, &thisString, result, vm);
}

/**
 * Convert string to upper case
 */
static int SM_toUpperCase(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return F_upper(1, &thisString, result, vm);
}

/**
 * Trim whitespace characters from the string
 */
static int SM_trim(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return F_trim(1, &thisString, result, vm);
}

/**
 * Trim leading whitespace characters from the string
 */
static int SM_trimLeft(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return F_ltrim(1, &thisString, result, vm);
}

/**
 * Trim trailing whitespace characters from the string
 */
static int SM_trimRight(NXSL_Value* thisString, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return F_rtrim(1, &thisString, result, vm);
}

/**
 * List of string methods
 */
static struct
{
   NXSL_Identifier name;
   int (*handler)(NXSL_Value*, int, NXSL_Value**, NXSL_Value**, NXSL_VM*);
   int argc;
} s_methods[] =
{
   { NXSL_Identifier("compareTo"), SM_compareTo, 1 },
   { NXSL_Identifier("compareToIgnoreCase"), SM_compareToIgnoreCase, 1 },
   { NXSL_Identifier("contains"), SM_contains, 1 },
   { NXSL_Identifier("containsListElement"), SM_containsListElement, 2 },
   { NXSL_Identifier("endsWith"), SM_endsWith, 1 },
   { NXSL_Identifier("equals"), SM_equals, 1 },
   { NXSL_Identifier("equalsIgnoreCase"), SM_equalsIgnoreCase, 1 },
   { NXSL_Identifier("fuzzyEquals"), SM_fuzzyEquals, -1 },
   { NXSL_Identifier("fuzzyEqualsIgnoreCase"), SM_fuzzyEqualsIgnoreCase, -1 },
   { NXSL_Identifier("indexOf"), SM_indexOf, -1 },
   { NXSL_Identifier("lastIndexOf"), SM_lastIndexOf, -1 },
   { NXSL_Identifier("left"), SM_left, -1 },
   { NXSL_Identifier("replace"), SM_replace, 2 },
   { NXSL_Identifier("right"), SM_right, -1 },
   { NXSL_Identifier("split"), SM_split, -1 },
   { NXSL_Identifier("startsWith"), SM_startsWith, 1 },
   { NXSL_Identifier("substring"), SM_substring, -1 },
   { NXSL_Identifier("toLowerCase"), SM_toLowerCase, 0 },
   { NXSL_Identifier("toUpperCase"), SM_toUpperCase, 0 },
   { NXSL_Identifier("trim"), SM_trim, 0 },
   { NXSL_Identifier("trimLeft"), SM_trimLeft, 0 },
   { NXSL_Identifier("trimRight"), SM_trimRight, 0 },
   { NXSL_Identifier(), nullptr, 0 }
};

/**
 * Call method on string
 */
int NXSL_VM::callStringMethod(NXSL_Value *s, const NXSL_Identifier& name, int argc, NXSL_Value **argv, NXSL_Value **result)
{
   for(int i = 0; s_methods[i].handler != nullptr; i++)
      if (s_methods[i].name.equals(name))
      {
         if ((s_methods[i].argc != -1) && (s_methods[i].argc != argc))
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         return s_methods[i].handler(s, argc, argv, result, this);
      }
   return NXSL_ERR_NO_SUCH_METHOD;
}

/**
 * Get string's attribute
 */
void NXSL_VM::getStringAttribute(NXSL_Value *v, const NXSL_Identifier& attribute, bool safe)
{
   static NXSL_Identifier A_isEmpty("isEmpty");
   static NXSL_Identifier A_length("length");

   if (A_isEmpty.equals(attribute))
   {
      uint32_t length;
      v->getValueAsString(&length);
      m_dataStack.push(createValue(length == 0));
   }
   else if (A_length.equals(attribute))
   {
      uint32_t length;
      v->getValueAsString(&length);
      m_dataStack.push(createValue(length));
   }
   else
   {
      if (safe)
         m_dataStack.push(createValue());
      else
         error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
   }
}
