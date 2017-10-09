/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: fraser@google.com (Neil Fraser)
 * Author: mikeslemmer@gmail.com (Mike Slemmer)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Diff Match and Patch
 * http://code.google.com/p/google-diff-match-patch/
 */

#include "libnetxms.h"
#include "diff.h"

// Define some regex patterns for matching boundaries.
#define BLANKLINEEND _T("\\n\\r?\\n$")
#define BLANKLINESTART _T("^\\r?\\n\\r?\\n")

template<typename T> class MutableListIterator
{
private:
   ObjectArray<T> *m_array;
   int m_pos;
   bool m_forward;

public:
   MutableListIterator(ObjectArray<T> *array)
   {
      m_array = array;
      m_pos = 0;
      m_forward = true;
   }

   bool hasNext() { return m_pos < m_array->size(); }
   bool hasPrevious() { return m_pos > 0; }

   T *next() { m_forward = true; return hasNext() ? m_array->get(m_pos++) : NULL; }
   T *previous() { m_forward = false; return hasPrevious() ? m_array->get(--m_pos) : NULL; }

   void insert(T *value) { m_array->insert(m_pos++, value); }
   void remove()
   {
      if (m_forward)
      {
         m_array->remove(m_pos - 1);
         m_pos--;
      }
      else
      {
         m_array->remove(m_pos);
      }
   }

   void setValue(T *v) { m_array->replace(m_forward ? (m_pos - 1) : m_pos, v); }

   void toFront() { m_pos = 0; }
   void toBack() { m_pos = m_array->size(); }
};

template<typename T> class Stack
{
private:
   Array m_data;

public:
   Stack() : m_data(16, 16, false) { }

   void push(T *v) { m_data.add((void *)v); }
   T *pop() { int p = m_data.size(); T *v = (T *)m_data.get(p - 1); m_data.remove(p - 1); return v; }
   T *top() { return (T *)m_data.get(m_data.size() - 1); }
   void clear() { m_data.clear(); }
   bool isEmpty() { return m_data.isEmpty(); }
};

//////////////////////////
//
// Diff Class
//
//////////////////////////

/**
 * Constructor.  Initializes the diff with the provided values.
 * @param operation One of INSERT, DELETE or EQUAL
 * @param text The text being applied
 */
Diff::Diff(DiffOperation _operation, const String &_text) : text(_text)
{
   operation = _operation;
}

Diff::Diff(const Diff *src)
{
   operation = src->operation;
   text = src->text;
}

Diff::Diff(const Diff &src)
{
   operation = src.operation;
   text = src.text;
}

Diff::Diff()
{
   operation = DIFF_EQUAL;
}

String Diff::strOperation(DiffOperation op)
{
   switch(op)
   {
      case DIFF_INSERT:
         return _T("INSERT");
      case DIFF_DELETE:
         return _T("DELETE");
      case DIFF_EQUAL:
         return _T("EQUAL");
   }
   return _T("Invalid operation");
}

/**
 * Display a human-readable version of this Diff.
 * @return text version
 */
String Diff::toString() const
{
   String prettyText = _T("Diff(");
   prettyText.append(strOperation(operation));
   prettyText.append(_T(",\""));
   prettyText.append(text);
   prettyText.append(_T("\")"));
   // Replace linebreaks with Pilcrow signs.
   return prettyText;
}

/**
 * Is this Diff equivalent to another Diff?
 * @param d Another Diff to compare against
 * @return true or false
 */
bool Diff::operator==(const Diff &d) const
{
   return (d.operation == this->operation) && (d.text == this->text);
}

bool Diff::operator!=(const Diff &d) const
{
   return !(operator == (d));
}

/////////////////////////////////////////////
//
// DiffEngine Class
//
/////////////////////////////////////////////

DiffEngine::DiffEngine()
{
   Diff_Timeout = 5000;
   Diff_EditCost = 4;
}

ObjectArray<Diff> *DiffEngine::diff_main(const String &text1, const String &text2)
{
   return diff_main(text1, text2, true);
}

ObjectArray<Diff> *DiffEngine::diff_main(const String &text1, const String &text2, bool checklines)
{
   // Set a deadline by which time the diff must be complete.
   INT64 deadline;
   if (Diff_Timeout <= 0)
   {
      deadline = _LL(0x7FFFFFFFFFFFFFFF);
   }
   else
   {
      deadline = GetCurrentTimeMs() + Diff_Timeout;
   }
   return diff_main(text1, text2, checklines, deadline);
}

ObjectArray<Diff> *DiffEngine::diff_main(const String &text1, const String &text2, bool checklines, INT64 deadline)
{
   // Check for equality (speedup).
   if (text1 == text2)
   {
      ObjectArray<Diff> *diffs = new ObjectArray<Diff>(16, 16, true);
      if (!text1.isEmpty())
      {
         diffs->add(new Diff(DIFF_EQUAL, text1));
      }
      return diffs;
   }

   ObjectArray<Diff> *diffs;
   if (checklines)
   {
      diffs = diff_compute(text1, text2, checklines, deadline);
   }
   else
   {
      // Trim off common prefix (speedup).
      int commonlength = diff_commonPrefix(text1, text2);
      const String &commonprefix = text1.left(commonlength);
      String textChopped1 = text1.substring(commonlength, -1);
      String textChopped2 = text2.substring(commonlength, -1);

      // Trim off common suffix (speedup).
      commonlength = diff_commonSuffix(textChopped1, textChopped2);
      const String &commonsuffix = textChopped1.right(commonlength);
      textChopped1 = textChopped1.left(textChopped1.length() - commonlength);
      textChopped2 = textChopped2.left(textChopped2.length() - commonlength);

      // Compute the diff on the middle block.
      diffs = diff_compute(textChopped1, textChopped2, checklines, deadline);

      // Restore the prefix and suffix.
      if (!commonprefix.isEmpty())
      {
         diffs->insert(0, new Diff(DIFF_EQUAL, commonprefix));
      }
      if (!commonsuffix.isEmpty())
      {
         diffs->add(new Diff(DIFF_EQUAL, commonsuffix));
      }

      diff_cleanupMerge(*diffs);
   }
   return diffs;
}

ObjectArray<Diff> *DiffEngine::diff_compute(String text1, String text2, bool checklines, INT64 deadline)
{
   if (text1.isEmpty())
   {
      // Just add some text (speedup).
      ObjectArray<Diff> *diffs = new ObjectArray<Diff>(64, 64, true);
      diffs->add(new Diff(DIFF_INSERT, text2));
      return diffs;
   }

   if (text2.isEmpty())
   {
      // Just delete some text (speedup).
      ObjectArray<Diff> *diffs = new ObjectArray<Diff>(64, 64, true);
      diffs->add(new Diff(DIFF_DELETE, text1));
      return diffs;
   }

   if (!checklines)
   {
      ObjectArray<Diff> *diffs = new ObjectArray<Diff>(64, 64, true);
      const String longtext = text1.length() > text2.length() ? text1 : text2;
      const String shorttext = text1.length() > text2.length() ? text2 : text1;
      const int i = longtext.find(shorttext);
      if (i != String::npos)
      {
         // Shorter text is inside the longer text (speedup).
         const DiffOperation op = (text1.length() > text2.length()) ? DIFF_DELETE : DIFF_INSERT;
         diffs->add(new Diff(op, longtext.left(i)));
         diffs->add(new Diff(DIFF_EQUAL, shorttext));
         diffs->add(new Diff(op, safeMid(longtext, i + shorttext.length())));
         return diffs;
      }

      if (shorttext.length() == 1)
      {
         // Single character string.
         // After the previous speedup, the character can't be an equality.
         diffs->add(new Diff(DIFF_DELETE, text1));
         diffs->add(new Diff(DIFF_INSERT, text2));
         return diffs;
      }
      delete diffs;
   }

   // Check to see if the problem can be split in two.
   if (!checklines)
   {
      const StringList *hm = diff_halfMatch(text1, text2);
      if (hm->size() > 0)
      {
         // A half-match was found, sort out the return data.
         // Send both pairs off for separate processing.
         ObjectArray<Diff> *diffs_a = diff_main(hm->get(0), hm->get(2), checklines, deadline);
         ObjectArray<Diff> *diffs_b = diff_main(hm->get(1), hm->get(3), checklines, deadline);
         // Merge the results.
         diffs_a->add(new Diff(DIFF_EQUAL, hm->get(4)));
         for(int i = 0; i < diffs_b->size(); i++)
            diffs_a->add(diffs_b->get(i));
         diffs_b->setOwner(false);
         delete diffs_b;
         delete hm;
         return diffs_a;
      }
      delete hm;
   }

   // Perform a real diff.
   if (checklines && !text1.isEmpty() && !text2.isEmpty())
   {
      return diff_lineMode(text1, text2, deadline);
   }

   return diff_bisect(text1, text2, deadline);
}

ObjectArray<Diff> *DiffEngine::diff_lineMode(const String& text1, const String& text2, INT64 deadline)
{
   // Scan the text on a line-by-line basis first.
   Array *b = diff_linesToChars(text1, text2);
   String *ltext1 = (String *)b->get(0);
   String *ltext2 = (String *)b->get(1);
   StringList *linearray = (StringList *)b->get(2);
   delete b;

   ObjectArray<Diff> *diffs = diff_main(*ltext1, *ltext2, false, deadline);
   delete ltext1;
   delete ltext2;

   // Convert the diff back to original text.
   diff_charsToLines(diffs, linearray);
   delete linearray;

   // Eliminate freak matches (e.g. blank lines)
   diff_cleanupSemantic(*diffs);

   return diffs;
}

ObjectArray<Diff> *DiffEngine::diff_bisect(const String &text1, const String &text2, INT64 deadline)
{
   // Cache the text lengths to prevent multiple calls.
   const int text1_length = (int)text1.length();
   const int text2_length = (int)text2.length();
   const int max_d = (text1_length + text2_length + 1) / 2;
   const int v_offset = max_d;
   const int v_length = 2 * max_d;
   int *v1 = new int[v_length];
   int *v2 = new int[v_length];
   for(int x = 0; x < v_length; x++)
   {
      v1[x] = -1;
      v2[x] = -1;
   }
   v1[v_offset + 1] = 0;
   v2[v_offset + 1] = 0;
   const int delta = text1_length - text2_length;
   // If the total number of characters is odd, then the front path will
   // collide with the reverse path.
   const bool front = (delta % 2 != 0);
   // Offsets for start and end of k loop.
   // Prevents mapping of space beyond the grid.
   int k1start = 0;
   int k1end = 0;
   int k2start = 0;
   int k2end = 0;
   for(int d = 0; d < max_d; d++)
   {
      // Bail out if deadline is reached.
      if (GetCurrentTimeMs() > deadline)
      {
         break;
      }

      // Walk the front path one step.
      for(int k1 = -d + k1start; k1 <= d - k1end; k1 += 2)
      {
         const int k1_offset = v_offset + k1;
         int x1;
         if (k1 == -d || (k1 != d && v1[k1_offset - 1] < v1[k1_offset + 1]))
         {
            x1 = v1[k1_offset + 1];
         }
         else
         {
            x1 = v1[k1_offset - 1] + 1;
         }
         int y1 = x1 - k1;
         while(x1 < text1_length && y1 < text2_length && text1[x1] == text2[y1])
         {
            x1++;
            y1++;
         }
         v1[k1_offset] = x1;
         if (x1 > text1_length)
         {
            // Ran off the right of the graph.
            k1end += 2;
         }
         else if (y1 > text2_length)
         {
            // Ran off the bottom of the graph.
            k1start += 2;
         }
         else if (front)
         {
            int k2_offset = v_offset + delta - k1;
            if (k2_offset >= 0 && k2_offset < v_length && v2[k2_offset] != -1)
            {
               // Mirror x2 onto top-left coordinate system.
               int x2 = text1_length - v2[k2_offset];
               if (x1 >= x2)
               {
                  // Overlap detected.
                  delete[] v1;
                  delete[] v2;
                  return diff_bisectSplit(text1, text2, x1, y1, deadline);
               }
            }
         }
      }

      // Walk the reverse path one step.
      for(int k2 = -d + k2start; k2 <= d - k2end; k2 += 2)
      {
         const int k2_offset = v_offset + k2;
         int x2;
         if (k2 == -d || (k2 != d && v2[k2_offset - 1] < v2[k2_offset + 1]))
         {
            x2 = v2[k2_offset + 1];
         }
         else
         {
            x2 = v2[k2_offset - 1] + 1;
         }
         int y2 = x2 - k2;
         while(x2 < text1_length && y2 < text2_length && text1[text1_length - x2 - 1] == text2[text2_length - y2 - 1])
         {
            x2++;
            y2++;
         }
         v2[k2_offset] = x2;
         if (x2 > text1_length)
         {
            // Ran off the left of the graph.
            k2end += 2;
         }
         else if (y2 > text2_length)
         {
            // Ran off the top of the graph.
            k2start += 2;
         }
         else if (!front)
         {
            int k1_offset = v_offset + delta - k2;
            if (k1_offset >= 0 && k1_offset < v_length && v1[k1_offset] != -1)
            {
               int x1 = v1[k1_offset];
               int y1 = v_offset + x1 - k1_offset;
               // Mirror x2 onto top-left coordinate system.
               x2 = text1_length - x2;
               if (x1 >= x2)
               {
                  // Overlap detected.
                  delete[] v1;
                  delete[] v2;
                  return diff_bisectSplit(text1, text2, x1, y1, deadline);
               }
            }
         }
      }
   }
   delete[] v1;
   delete[] v2;
   // Diff took too long and hit the deadline or
   // number of diffs equals number of characters, no commonality at all.
   ObjectArray<Diff> *diffs = new ObjectArray<Diff>(16, 16, true);
   diffs->add(new Diff(DIFF_DELETE, text1));
   diffs->add(new Diff(DIFF_INSERT, text2));
   return diffs;
}

ObjectArray<Diff> *DiffEngine::diff_bisectSplit(const String &text1, const String &text2, int x, int y, INT64 deadline)
{
   String text1a = text1.left(x);
   String text2a = text2.left(y);
   String text1b = safeMid(text1, x);
   String text2b = safeMid(text2, y);

   // Compute both diffs serially.
   ObjectArray<Diff> *diffs = diff_main(text1a, text2a, false, deadline);
   ObjectArray<Diff> *diffsb = diff_main(text1b, text2b, false, deadline);
   for(int i = 0; i < diffsb->size(); i++)
      diffs->add(diffsb->get(i));
   diffsb->setOwner(false);
   delete diffsb;

   return diffs;
}

Array *DiffEngine::diff_linesToChars(const String &text1, const String &text2)
{
   StringList *lineArray = new StringList();
   StringIntMap<int> lineHash;
   // e.g. linearray[4] == "Hello\n"
   // e.g. linehash.get("Hello\n") == 4

   // "\x00" is a valid character, but various debuggers don't like it.
   // So we'll insert a junk entry to avoid generating a null character.
   lineArray->add(_T(""));

   const String chars1 = diff_linesToCharsMunge(text1, *lineArray, lineHash);
   const String chars2 = diff_linesToCharsMunge(text2, *lineArray, lineHash);

   Array *listRet = new Array(3, 3, false);
   listRet->add(new String(chars1));
   listRet->add(new String(chars2));
   listRet->add(lineArray);
   return listRet;
}

String DiffEngine::diff_linesToCharsMunge(const String &text, StringList &lineArray, StringIntMap<int>& lineHash)
{
   size_t lineStart = 0;
   size_t lineEnd = 0;
   String line;
   String chars;
   // Walk the text, pulling out a substring for each line.
   // text.split('\n') would would temporarily double our memory footprint.
   // Modifying text would create many large strings to garbage collect.
   while(lineEnd < text.length())
   {
      lineEnd = text.find(_T("\n"), lineStart);
      if (lineEnd == String::npos)
      {
         lineEnd = text.length();
      }
      line = safeMid(text, lineStart, (int)(lineEnd - lineStart + 1));
      lineStart = lineEnd + 1;

      if (lineHash.contains(line))
      {
         chars.append(static_cast<WCHAR>(lineHash.get(line)));
      }
      else
      {
         lineArray.add(line);
         lineHash.set(line, lineArray.size() - 1);
         chars.append(static_cast<WCHAR>(lineArray.size() - 1));
      }
   }
   return chars;
}

void DiffEngine::diff_charsToLines(ObjectArray<Diff> *diffs, const StringList &lineArray)
{
   MutableListIterator<Diff> i(diffs);
   while(i.hasNext())
   {
      Diff *diff = i.next();
      String text;
      for(int y = 0; y < (int)diff->text.length(); y++)
      {
         text += lineArray.get(static_cast<int>(diff->text.charAt(y)));
      }
      diff->text = text;
   }
}

int DiffEngine::diff_commonPrefix(const String &text1, const String &text2)
{
   // Performance analysis: http://neil.fraser.name/news/2007/10/09/
   const int n = (int)std::min(text1.length(), text2.length());
   for(int i = 0; i < n; i++)
   {
      if (text1.charAt(i) != text2.charAt(i))
      {
         return i;
      }
   }
   return n;
}

int DiffEngine::diff_commonSuffix(const String &text1, const String &text2)
{
   // Performance analysis: http://neil.fraser.name/news/2007/10/09/
   const int text1_length = (int)text1.length();
   const int text2_length = (int)text2.length();
   const int n = std::min(text1_length, text2_length);
   for(int i = 1; i <= n; i++)
   {
      if (text1.charAt(text1_length - i) != text2.charAt(text2_length - i))
      {
         return i - 1;
      }
   }
   return n;
}

size_t DiffEngine::diff_commonOverlap(const String &text1, const String &text2)
{
   // Cache the text lengths to prevent multiple calls.
   const size_t text1_length = text1.length();
   const size_t text2_length = text2.length();
   // Eliminate the null case.
   if (text1_length == 0 || text2_length == 0)
   {
      return 0;
   }
   // Truncate the longer string.
   String text1_trunc = text1;
   String text2_trunc = text2;
   if (text1_length > text2_length)
   {
      text1_trunc = text1.right(text2_length);
   }
   else if (text1_length < text2_length)
   {
      text2_trunc = text2.left(text1_length);
   }
   const size_t text_length = std::min(text1_length, text2_length);
   // Quick check for the worst case.
   if (text1_trunc == text2_trunc)
   {
      return text_length;
   }

   // Start by looking for a single character match
   // and increase length until no match is found.
   // Performance analysis: http://neil.fraser.name/news/2010/11/04/
   size_t best = 0;
   size_t length = 1;
   while(true)
   {
      String pattern = text1_trunc.right(length);
      int found = text2_trunc.find(pattern);
      if (found == String::npos)
      {
         return best;
      }
      length += found;
      if (found == 0 || text1_trunc.right(length) == text2_trunc.left(length))
      {
         best = length;
         length++;
      }
   }
}

StringList *DiffEngine::diff_halfMatch(const String &text1, const String &text2)
{
   if (Diff_Timeout <= 0)
   {
      // Don't risk returning a non-optimal diff if we have unlimited time.
      return new StringList();
   }
   const String longtext = text1.length() > text2.length() ? text1 : text2;
   const String shorttext = text1.length() > text2.length() ? text2 : text1;
   if (longtext.length() < 4 || shorttext.length() * 2 < longtext.length())
   {
      return new StringList();  // Pointless.
   }

   // First check if the second quarter is the seed for a half-match.
   StringList *hm1 = diff_halfMatchI(longtext, shorttext, ((int)longtext.length() + 3) / 4);
   // Check again based on the third quarter.
   StringList *hm2 = diff_halfMatchI(longtext, shorttext, ((int)longtext.length() + 1) / 2);
   if (hm1->isEmpty() && hm2->isEmpty())
   {
      delete hm1;
      delete hm2;
      return new StringList();
   }

   StringList *hm;
   if (hm2->isEmpty())
   {
      hm = hm1;
      delete hm2;
   }
   else if (hm1->isEmpty())
   {
      hm = hm2;
      delete hm1;
   }
   else
   {
      // Both matched.  Select the longest.
      if (_tcslen(hm1->get(4)) > _tcslen(hm2->get(4)))
      {
         hm = hm1;
         delete hm2;
      }
      else
      {
         hm = hm2;
         delete hm1;
      }
   }

   // A half-match was found, sort out the return data.
   if (text1.length() > text2.length())
   {
      return hm;
   }

   StringList *listRet = new StringList();
   listRet->add(hm->get(2));
   listRet->add(hm->get(3));
   listRet->add(hm->get(0));
   listRet->add(hm->get(1));
   listRet->add(hm->get(4));
   delete hm;
   return listRet;
}

StringList *DiffEngine::diff_halfMatchI(const String &longtext, const String &shorttext, int i)
{
   // Start with a 1/4 length substring at position i as a seed.
   const String seed = safeMid(longtext, i, (int)(longtext.length() / 4));
   int j = -1;
   String best_common;
   String best_longtext_a, best_longtext_b;
   String best_shorttext_a, best_shorttext_b;
   while((j = shorttext.find(seed, j + 1)) != -1)
   {
      const int prefixLength = diff_commonPrefix(safeMid(longtext, i), safeMid(shorttext, j));
      const int suffixLength = diff_commonSuffix(longtext.left(i), shorttext.left(j));
      if ((int)best_common.length() < suffixLength + prefixLength)
      {
         best_common = safeMid(shorttext, j - suffixLength, suffixLength);
         best_common.append(safeMid(shorttext, j, prefixLength));

         best_longtext_a = longtext.left(i - suffixLength);
         best_longtext_b = safeMid(longtext, i + prefixLength);

         best_shorttext_a = shorttext.left(j - suffixLength);
         best_shorttext_b = safeMid(shorttext, j + prefixLength);
      }
   }
   if (best_common.length() * 2 >= longtext.length())
   {
      StringList *listRet = new StringList;
      listRet->add(best_longtext_a);
      listRet->add(best_longtext_b);
      listRet->add(best_shorttext_a);
      listRet->add(best_shorttext_b);
      listRet->add(best_common);
      return listRet;
   }
   else
   {
      return new StringList();
   }
}

void DiffEngine::diff_cleanupSemantic(ObjectArray<Diff> &diffs)
{
   if (diffs.isEmpty())
   {
      return;
   }
   bool changes = false;
   Stack<Diff> equalities;  // Stack of equalities.
   String lastequality;  // Always equal to equalities.lastElement().text
   MutableListIterator<Diff> pointer(&diffs);
   // Number of characters that changed prior to the equality.
   int length_insertions1 = 0;
   int length_deletions1 = 0;
   // Number of characters that changed after the equality.
   int length_insertions2 = 0;
   int length_deletions2 = 0;
   Diff *thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   while(thisDiff != NULL)
   {
      if (thisDiff->operation == DIFF_EQUAL)
      {
         // Equality found.
         equalities.push(thisDiff);
         length_insertions1 = length_insertions2;
         length_deletions1 = length_deletions2;
         length_insertions2 = 0;
         length_deletions2 = 0;
         lastequality = thisDiff->text;
      }
      else
      {
         // An insertion or deletion.
         if (thisDiff->operation == DIFF_INSERT)
         {
            length_insertions2 += (int)thisDiff->text.length();
         }
         else
         {
            length_deletions2 += (int)thisDiff->text.length();
         }
         // Eliminate an equality that is smaller or equal to the edits on both
         // sides of it.
         if (!lastequality.isEmpty() && ((int)lastequality.length() <= std::max(length_insertions1, length_deletions1))
                  && ((int)lastequality.length() <= std::max(length_insertions2, length_deletions2)))
         {
            // printf("Splitting: '%s'\n", qPrintable(lastequality));
            // Walk back to offending equality.
            while(*thisDiff != *equalities.top())
            {
               thisDiff = pointer.previous();
            }
            pointer.next();

            // Replace equality with a delete.
            pointer.setValue(new Diff(DIFF_DELETE, lastequality));
            // Insert a corresponding an insert.
            pointer.insert(new Diff(DIFF_INSERT, lastequality));

            equalities.pop();  // Throw away the equality we just deleted.
            if (!equalities.isEmpty())
            {
               // Throw away the previous equality (it needs to be reevaluated).
               equalities.pop();
            }
            if (equalities.isEmpty())
            {
               // There are no previous equalities, walk back to the start.
               while(pointer.hasPrevious())
               {
                  pointer.previous();
               }
            }
            else
            {
               // There is a safe equality we can fall back to.
               thisDiff = equalities.top();
               while(*thisDiff != *pointer.previous())
               {
                  // Intentionally empty loop.
               }
            }

            length_insertions1 = 0;  // Reset the counters.
            length_deletions1 = 0;
            length_insertions2 = 0;
            length_deletions2 = 0;
            lastequality = String();
            changes = true;
         }
      }
      thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   }

   // Normalize the diff.
   if (changes)
   {
      diff_cleanupMerge(diffs);
   }
   diff_cleanupSemanticLossless(diffs);

   // Find any overlaps between deletions and insertions.
   // e.g: <del>abcxxx</del><ins>xxxdef</ins>
   //   -> <del>abc</del>xxx<ins>def</ins>
   // e.g: <del>xxxabc</del><ins>defxxx</ins>
   //   -> <ins>def</ins>xxx<del>abc</del>
   // Only extract an overlap if it is as big as the edit ahead or behind it.
   pointer.toFront();
   Diff *prevDiff = NULL;
   thisDiff = NULL;
   if (pointer.hasNext())
   {
      prevDiff = pointer.next();
      if (pointer.hasNext())
      {
         thisDiff = pointer.next();
      }
   }
   while(thisDiff != NULL)
   {
      if (prevDiff->operation == DIFF_DELETE && thisDiff->operation == DIFF_INSERT)
      {
         String deletion = prevDiff->text;
         String insertion = thisDiff->text;
         size_t overlap_length1 = diff_commonOverlap(deletion, insertion);
         size_t overlap_length2 = diff_commonOverlap(insertion, deletion);
         if (overlap_length1 >= overlap_length2)
         {
            if (overlap_length1 >= deletion.length() / 2.0 || overlap_length1 >= insertion.length() / 2.0)
            {
               // Overlap found.  Insert an equality and trim the surrounding edits.
               pointer.previous();
               pointer.insert(new Diff(DIFF_EQUAL, insertion.left(overlap_length1)));
               prevDiff->text = deletion.left(deletion.length() - overlap_length1);
               thisDiff->text = safeMid(insertion, overlap_length1);
               // pointer.insert inserts the element before the cursor, so there is
               // no need to step past the new element.
            }
         }
         else
         {
            if (overlap_length2 >= deletion.length() / 2.0 || overlap_length2 >= insertion.length() / 2.0)
            {
               // Reverse overlap found.
               // Insert an equality and swap and trim the surrounding edits.
               pointer.previous();
               pointer.insert(new Diff(DIFF_EQUAL, deletion.left(overlap_length2)));
               prevDiff->operation = DIFF_INSERT;
               prevDiff->text = insertion.left(insertion.length() - overlap_length2);
               thisDiff->operation = DIFF_DELETE;
               thisDiff->text = safeMid(deletion, overlap_length2);
               // pointer.insert inserts the element before the cursor, so there is
               // no need to step past the new element.
            }
         }
         thisDiff = pointer.hasNext() ? pointer.next() : NULL;
      }
      prevDiff = thisDiff;
      thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   }
}

void DiffEngine::diff_cleanupSemanticLossless(ObjectArray<Diff> &diffs)
{
   String equality1, edit, equality2;
   String commonString;
   int commonOffset;
   int score, bestScore;
   String bestEquality1, bestEdit, bestEquality2;
   // Create a new iterator at the start.
   MutableListIterator<Diff> pointer(&diffs);
   Diff *prevDiff = pointer.hasNext() ? pointer.next() : NULL;
   Diff *thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   Diff *nextDiff = pointer.hasNext() ? pointer.next() : NULL;

   // Intentionally ignore the first and last element (don't need checking).
   while(nextDiff != NULL)
   {
      if (prevDiff->operation == DIFF_EQUAL && nextDiff->operation == DIFF_EQUAL)
      {
         // This is a single edit surrounded by equalities.
         equality1 = prevDiff->text;
         edit = thisDiff->text;
         equality2 = nextDiff->text;

         // First, shift the edit as far left as possible.
         commonOffset = diff_commonSuffix(equality1, edit);
         if (commonOffset != 0)
         {
            commonString = safeMid(edit, edit.length() - commonOffset);
            equality1 = equality1.left(equality1.length() - commonOffset);
            edit = commonString + edit.left(edit.length() - commonOffset);
            equality2 = commonString + equality2;
         }

         // Second, step character by character right, looking for the best fit.
         bestEquality1 = equality1;
         bestEdit = edit;
         bestEquality2 = equality2;
         bestScore = diff_cleanupSemanticScore(equality1, edit) + diff_cleanupSemanticScore(edit, equality2);
         while(!edit.isEmpty() && !equality2.isEmpty() && edit.charAt(0) == equality2.charAt(0))
         {
            equality1.append(edit.charAt(0));
            edit = safeMid(edit, 1);
            edit.append(equality2.charAt(0));
            equality2 = safeMid(equality2, 1);
            score = diff_cleanupSemanticScore(equality1, edit) + diff_cleanupSemanticScore(edit, equality2);
            // The >= encourages trailing rather than leading whitespace on edits.
            if (score >= bestScore)
            {
               bestScore = score;
               bestEquality1 = equality1;
               bestEdit = edit;
               bestEquality2 = equality2;
            }
         }

         if (prevDiff->text != bestEquality1)
         {
            // We have an improvement, save it back to the diff.
            if (!bestEquality1.isEmpty())
            {
               prevDiff->text = bestEquality1;
            }
            else
            {
               pointer.previous();  // Walk past nextDiff.
               pointer.previous();  // Walk past thisDiff.
               pointer.previous();  // Walk past prevDiff.
               pointer.remove();  // Delete prevDiff.
               pointer.next();  // Walk past thisDiff.
               pointer.next();  // Walk past nextDiff.
            }
            thisDiff->text = bestEdit;
            if (!bestEquality2.isEmpty())
            {
               nextDiff->text = bestEquality2;
            }
            else
            {
               pointer.remove(); // Delete nextDiff.
               nextDiff = thisDiff;
               thisDiff = prevDiff;
            }
         }
      }
      prevDiff = thisDiff;
      thisDiff = nextDiff;
      nextDiff = pointer.hasNext() ? pointer.next() : NULL;
   }
}

int DiffEngine::diff_cleanupSemanticScore(const String &one, const String &two)
{
   if (one.isEmpty() || two.isEmpty())
   {
      // Edges are the best.
      return 6;
   }

   // Each port of this function behaves slightly differently due to
   // subtle differences in each language's definition of things like
   // 'whitespace'.  Since this function's purpose is largely cosmetic,
   // the choice has been made to use each language's native features
   // rather than force total conformity.
   TCHAR char1 = one.charAt(one.length() - 1);
   TCHAR char2 = two.charAt(0);
   bool nonAlphaNumeric1 = !_istalnum(char1);
   bool nonAlphaNumeric2 = !_istalnum(char2);
   bool whitespace1 = nonAlphaNumeric1 && _istspace(char1);
   bool whitespace2 = nonAlphaNumeric2 && _istspace(char2);
   bool lineBreak1 = whitespace1 && (char1 == '\n');
   bool lineBreak2 = whitespace2 && (char2 == '\n');
   bool blankLine1 = lineBreak1 && RegexpMatch(one, BLANKLINEEND, true);
   bool blankLine2 = lineBreak2 && RegexpMatch(two, BLANKLINESTART, true);

   if (blankLine1 || blankLine2)
   {
      // Five points for blank lines.
      return 5;
   }
   else if (lineBreak1 || lineBreak2)
   {
      // Four points for line breaks.
      return 4;
   }
   else if (nonAlphaNumeric1 && !whitespace1 && whitespace2)
   {
      // Three points for end of sentences.
      return 3;
   }
   else if (whitespace1 || whitespace2)
   {
      // Two points for whitespace.
      return 2;
   }
   else if (nonAlphaNumeric1 || nonAlphaNumeric2)
   {
      // One point for non-alphanumeric.
      return 1;
   }
   return 0;
}

void DiffEngine::diff_cleanupEfficiency(ObjectArray<Diff> &diffs)
{
   if (diffs.isEmpty())
   {
      return;
   }
   bool changes = false;
   Stack<Diff> equalities;  // Stack of equalities.
   String lastequality;  // Always equal to equalities.lastElement().text
   MutableListIterator<Diff> pointer(&diffs);
   // Is there an insertion operation before the last equality.
   bool pre_ins = false;
   // Is there a deletion operation before the last equality.
   bool pre_del = false;
   // Is there an insertion operation after the last equality.
   bool post_ins = false;
   // Is there a deletion operation after the last equality.
   bool post_del = false;

   Diff *thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   Diff *safeDiff = thisDiff;

   while(thisDiff != NULL)
   {
      if (thisDiff->operation == DIFF_EQUAL)
      {
         // Equality found.
         if ((int)thisDiff->text.length() < Diff_EditCost && (post_ins || post_del))
         {
            // Candidate found.
            equalities.push(thisDiff);
            pre_ins = post_ins;
            pre_del = post_del;
            lastequality = thisDiff->text;
         }
         else
         {
            // Not a candidate, and can never become one.
            equalities.clear();
            lastequality = String();
            safeDiff = thisDiff;
         }
         post_ins = post_del = false;
      }
      else
      {
         // An insertion or deletion.
         if (thisDiff->operation == DIFF_DELETE)
         {
            post_del = true;
         }
         else
         {
            post_ins = true;
         }
         /*
          * Five types to be split:
          * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
          * <ins>A</ins>X<ins>C</ins><del>D</del>
          * <ins>A</ins><del>B</del>X<ins>C</ins>
          * <ins>A</del>X<ins>C</ins><del>D</del>
          * <ins>A</ins><del>B</del>X<del>C</del>
          */
         if (!lastequality.isEmpty()
                  && ((pre_ins && pre_del && post_ins && post_del)
                           || (((int)lastequality.length() < Diff_EditCost / 2)
                                    && ((pre_ins ? 1 : 0) + (pre_del ? 1 : 0) + (post_ins ? 1 : 0) + (post_del ? 1 : 0)) == 3)))
         {
            // printf("Splitting: '%s'\n", qPrintable(lastequality));
            // Walk back to offending equality.
            while(*thisDiff != *equalities.top())
            {
               thisDiff = pointer.previous();
            }
            pointer.next();

            // Replace equality with a delete.
            pointer.setValue(new Diff(DIFF_DELETE, lastequality));
            // Insert a corresponding an insert.
            pointer.insert(new Diff(DIFF_INSERT, lastequality));
            thisDiff = pointer.previous();
            pointer.next();

            equalities.pop();  // Throw away the equality we just deleted.
            lastequality = String();
            if (pre_ins && pre_del)
            {
               // No changes made which could affect previous entry, keep going.
               post_ins = post_del = true;
               equalities.clear();
               safeDiff = thisDiff;
            }
            else
            {
               if (!equalities.isEmpty())
               {
                  // Throw away the previous equality (it needs to be reevaluated).
                  equalities.pop();
               }
               if (equalities.isEmpty())
               {
                  // There are no previous questionable equalities,
                  // walk back to the last known safe diff.
                  thisDiff = safeDiff;
               }
               else
               {
                  // There is an equality we can fall back to.
                  thisDiff = equalities.top();
               }
               while(*thisDiff != *pointer.previous())
               {
                  // Intentionally empty loop.
               }
               post_ins = post_del = false;
            }

            changes = true;
         }
      }
      thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   }

   if (changes)
   {
      diff_cleanupMerge(diffs);
   }
}

void DiffEngine::diff_cleanupMerge(ObjectArray<Diff> &diffs)
{
   diffs.add(new Diff(DIFF_EQUAL, _T("")));  // Add a dummy entry at the end.
   MutableListIterator<Diff> pointer(&diffs);
   int count_delete = 0;
   int count_insert = 0;
   String text_delete;
   String text_insert;
   Diff *thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   Diff *prevEqual = NULL;
   int commonlength;
   while(thisDiff != NULL)
   {
      switch(thisDiff->operation)
      {
         case DIFF_INSERT:
            count_insert++;
            text_insert += thisDiff->text;
            prevEqual = NULL;
            break;
         case DIFF_DELETE:
            count_delete++;
            text_delete += thisDiff->text;
            prevEqual = NULL;
            break;
         case DIFF_EQUAL:
            if (count_delete + count_insert > 1)
            {
               bool both_types = count_delete != 0 && count_insert != 0;
               // Delete the offending records.
               pointer.previous();  // Reverse direction.
               while(count_delete-- > 0)
               {
                  pointer.previous();
                  pointer.remove();
               }
               while(count_insert-- > 0)
               {
                  pointer.previous();
                  pointer.remove();
               }
               if (both_types)
               {
                  // Factor out any common prefixies.
                  commonlength = diff_commonPrefix(text_insert, text_delete);
                  if (commonlength != 0)
                  {
                     if (pointer.hasPrevious())
                     {
                        thisDiff = pointer.previous();
                        if (thisDiff->operation != DIFF_EQUAL)
                        {
                           return;
                        }
                        thisDiff->text += text_insert.left(commonlength);
                        pointer.next();
                     }
                     else
                     {
                        pointer.insert(new Diff(DIFF_EQUAL, text_insert.left(commonlength)));
                     }
                     text_insert = safeMid(text_insert, commonlength);
                     text_delete = safeMid(text_delete, commonlength);
                  }
                  // Factor out any common suffixies.
                  commonlength = diff_commonSuffix(text_insert, text_delete);
                  if (commonlength != 0)
                  {
                     thisDiff = pointer.next();
                     String s = safeMid(text_insert, text_insert.length() - commonlength);
                     s.append(thisDiff->text);
                     thisDiff->text = s;
                     text_insert = text_insert.left(text_insert.length() - commonlength);
                     text_delete = text_delete.left(text_delete.length() - commonlength);
                     pointer.previous();
                  }
               }
               // Insert the merged records.
               if (!text_delete.isEmpty())
               {
                  pointer.insert(new Diff(DIFF_DELETE, text_delete));
               }
               if (!text_insert.isEmpty())
               {
                  pointer.insert(new Diff(DIFF_INSERT, text_insert));
               }
               // Step forward to the equality.
               thisDiff = pointer.hasNext() ? pointer.next() : NULL;
            }
            else if (prevEqual != NULL)
            {
               // Merge this equality with the previous one.
               prevEqual->text += thisDiff->text;
               pointer.remove();
               thisDiff = pointer.previous();
               pointer.next();  // Forward direction
            }
            count_insert = 0;
            count_delete = 0;
            text_delete = _T("");
            text_insert = _T("");
            prevEqual = thisDiff;
            break;
      }
      thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   }
   if (diffs.last()->text.isEmpty())
   {
      diffs.remove(diffs.size() - 1);  // Remove the dummy entry at the end.
   }

   /*
    * Second pass: look for single edits surrounded on both sides by equalities
    * which can be shifted sideways to eliminate an equality.
    * e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
    */
   bool changes = false;
   // Create a new iterator at the start.
   // (As opposed to walking the current one back.)
   pointer.toFront();
   Diff *prevDiff = pointer.hasNext() ? pointer.next() : NULL;
   thisDiff = pointer.hasNext() ? pointer.next() : NULL;
   Diff *nextDiff = pointer.hasNext() ? pointer.next() : NULL;

   // Intentionally ignore the first and last element (don't need checking).
   while(nextDiff != NULL)
   {
      if (prevDiff->operation == DIFF_EQUAL && nextDiff->operation == DIFF_EQUAL)
      {
         // This is a single edit surrounded by equalities.
         if (thisDiff->text.endsWith(prevDiff->text))
         {
            // Shift the edit over the previous equality.
            String prefix = thisDiff->text.left(thisDiff->text.length() - prevDiff->text.length());
            thisDiff->text = prevDiff->text;
            thisDiff->text.append(prefix);
            String tmp = prevDiff->text;
            tmp.append(nextDiff->text);
            nextDiff->text = tmp;
            pointer.previous();  // Walk past nextDiff.
            pointer.previous();  // Walk past thisDiff.
            pointer.previous();  // Walk past prevDiff.
            pointer.remove();  // Delete prevDiff.
            pointer.next();  // Walk past thisDiff.
            thisDiff = pointer.next();  // Walk past nextDiff.
            nextDiff = pointer.hasNext() ? pointer.next() : NULL;
            changes = true;
         }
         else if (thisDiff->text.startsWith(nextDiff->text))
         {
            // Shift the edit over the next equality.
            prevDiff->text.append(nextDiff->text);
            thisDiff->text = safeMid(thisDiff->text, nextDiff->text.length());
            thisDiff->text.append(nextDiff->text);
            pointer.remove(); // Delete nextDiff.
            nextDiff = pointer.hasNext() ? pointer.next() : NULL;
            changes = true;
         }
      }
      prevDiff = thisDiff;
      thisDiff = nextDiff;
      nextDiff = pointer.hasNext() ? pointer.next() : NULL;
   }
   // If shifts were made, the diff needs reordering and another shift sweep.
   if (changes)
   {
      diff_cleanupMerge(diffs);
   }
}

int DiffEngine::diff_xIndex(const ObjectArray<Diff> &diffs, int loc)
{
   int chars1 = 0;
   int chars2 = 0;
   int last_chars1 = 0;
   int last_chars2 = 0;
   Diff *lastDiff;
   for(int i = 0; i < diffs.size(); i++)
   {
      Diff *aDiff = diffs.get(i);
      if (aDiff->operation != DIFF_INSERT)
      {
         // Equality or deletion.
         chars1 += (int)aDiff->text.length();
      }
      if (aDiff->operation != DIFF_DELETE)
      {
         // Equality or insertion.
         chars2 += (int)aDiff->text.length();
      }
      if (chars1 > loc)
      {
         // Overshot the location.
         lastDiff = aDiff;
         break;
      }
      last_chars1 = chars1;
      last_chars2 = chars2;
   }
   if (lastDiff->operation == DIFF_DELETE)
   {
      // The location was deleted.
      return last_chars2;
   }
   // Add the remaining character length.
   return last_chars2 + (loc - last_chars1);
}

inline void AppendLines(String& out, const String& text, TCHAR prefix)
{
   StringList *lines = text.split(_T("\n"));
   for(int i = 0; i < lines->size(); i++)
   {
      const TCHAR *l = lines->get(i);
      if (*l != 0)
      {
         out.append(prefix);
         out.append(l);
         out.append(_T('\n'));
      }
   }
   delete lines;
}

String DiffEngine::diff_generateLineDiff(ObjectArray<Diff> *diffs)
{
   String out;
   for(int i = 0; i < diffs->size(); i++)
   {
      Diff *aDiff = diffs->get(i);
      switch(aDiff->operation)
      {
         case DIFF_INSERT:
            AppendLines(out, aDiff->text, _T('+'));
            break;
         case DIFF_DELETE:
            AppendLines(out, aDiff->text, _T('-'));
            break;
         case DIFF_EQUAL:
            /* nothing to do - added to avoid compiler warning */
            break;
      }
   }
   return out;
}

String DiffEngine::diff_text1(const ObjectArray<Diff> &diffs)
{
   String text;
   for(int i = 0; i < diffs.size(); i++)
   {
      Diff *aDiff = diffs.get(i);
      if (aDiff->operation != DIFF_INSERT)
      {
         text.append(aDiff->text);
      }
   }
   return text;
}

String DiffEngine::diff_text2(const ObjectArray<Diff> &diffs)
{
   String text;
   for(int i = 0; i < diffs.size(); i++)
   {
      Diff *aDiff = diffs.get(i);
      if (aDiff->operation != DIFF_DELETE)
      {
         text += aDiff->text;
      }
   }
   return text;
}

int DiffEngine::diff_levenshtein(const ObjectArray<Diff> &diffs)
{
   int levenshtein = 0;
   int insertions = 0;
   int deletions = 0;
   for(int i = 0; i < diffs.size(); i++)
   {
      Diff *aDiff = diffs.get(i);
      switch(aDiff->operation)
      {
         case DIFF_INSERT:
         insertions += (int)aDiff->text.length();
         break;
         case DIFF_DELETE:
         deletions += (int)aDiff->text.length();
         break;
         case DIFF_EQUAL:
         // A deletion and an insertion is one substitution.
         levenshtein += std::max(insertions, deletions);
         insertions = 0;
         deletions = 0;
         break;
      }
   }
   levenshtein += std::max(insertions, deletions);
   return levenshtein;
}

String DiffEngine::diff_toDelta(const ObjectArray<Diff> &diffs)
{
   String text;
   for(int i = 0; i < diffs.size(); i++)
   {
      Diff *aDiff = diffs.get(i);
      switch(aDiff->operation)
      {
         case DIFF_INSERT:
            text.append(_T('+'));
            text.append(aDiff->text);
            text.append(_T('\t'));
            break;
         case DIFF_DELETE:
            text.appendFormattedString(_T("-%d\t"), aDiff->text.length());
            break;
         case DIFF_EQUAL:
            text.appendFormattedString(_T("=%d\t"), aDiff->text.length());
            break;
      }
   }
   if (!text.isEmpty())
   {
      // Strip off trailing tab character.
      text = text.left(text.length() - 1);
   }
   return text;
}

ObjectArray<Diff> *DiffEngine::diff_fromDelta(const String &text1, const String &delta)
{
   ObjectArray<Diff> *diffs = new ObjectArray<Diff>(64, 64, true);
   int pointer = 0;  // Cursor in text1
   StringList *tokens = delta.split(_T("\t"));
   for(int i = 0; i < tokens->size(); i++)
   {
      const TCHAR *token = tokens->get(i);
      if (*token == 0)
      {
         // Blank tokens are ok (from a trailing \t).
         continue;
      }
      // Each token begins with a one character parameter which specifies the
      // operation of this token (delete, insert, equality).
      String param = safeMid(token, 1);
      switch(token[0])
      {
         case '+':
            diffs->add(new Diff(DIFF_INSERT, param));
            break;
         case '-':
         // Fall through.
         case '=':
         {
            int n = _tcstol(param, NULL, 10);
            if (n < 0)
            {
               delete tokens;
               return diffs;
            }
            String text;
            text = safeMid(text1, pointer, n);
            pointer += n;
            if (token[0] == '=')
            {
               diffs->add(new Diff(DIFF_EQUAL, text));
            }
            else
            {
               diffs->add(new Diff(DIFF_DELETE, text));
            }
            break;
         }
         default:
            delete tokens;
            return diffs;
      }
   }
   delete tokens;
   return diffs;
}

/**
 * Produce diff between two strings line by line
 */
String LIBNETXMS_EXPORTABLE GenerateLineDiff(const String& left, const String& right)
{
   DiffEngine d;
   ObjectArray<Diff> *diffs = d.diff_main(left, right);
   String result = d.diff_generateLineDiff(diffs);
   delete diffs;
   return result;
}
