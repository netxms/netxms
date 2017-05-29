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

#ifndef DIFF_H
#define DIFF_H

/**
 * String map template for holding objects as values
 */
template<typename T> class StringIntMap : public StringMapBase
{
private:
   static void destructor(void *object) { }

public:
   StringIntMap() : StringMapBase(false) { m_objectDestructor = destructor; }

   void set(const TCHAR *key, T value) { setObject((TCHAR *)key, CAST_TO_POINTER(value, void*), false); }
   T get(const TCHAR *key) const { return CAST_FROM_POINTER(getObject(key), T); }
};

/*
 * Functions for diff, match and patch.
 * Computes the difference between two texts to create a patch.
 * Applies the patch onto another text, allowing for errors.
 *
 * @author fraser@google.com (Neil Fraser)
 *
 * Qt/C++ port by mikeslemmer@gmail.com (Mike Slemmer):
 *
 * Code known to compile and run with Qt 4.3 through Qt 4.7.
 *
 * Here is a trivial sample program which works properly when linked with this
 * library:
 *

 #include <QtCore>
 #include <String>
 #include <QList>
 #include <QMap>
 #include <QVariant>
 #include "diff_match_patch.h"
 int main(int argc, char **argv) {
   diff_match_patch dmp;
   String str1 = String("First string in diff");
   String str2 = String("Second string in diff");

   String strPatch = dmp.patch_toText(dmp.patch_make(str1, str2));
   QPair<String, QVector<bool> > out
       = dmp.patch_apply(dmp.patch_fromText(strPatch), str1);
   String strResult = out.first;

   // here, strResult will equal str2 above.
   return 0;
 }

 */

/**-
* The data structure representing a diff is a Linked list of Diff objects:
* {Diff(Operation.DELETE, "Hello"), Diff(Operation.INSERT, "Goodbye"),
*  Diff(Operation.EQUAL, " world.")}
* which means: delete "Hello", add "Goodbye" and keep " world."
*/
enum DiffOperation 
{
   DIFF_DELETE, 
   DIFF_INSERT, 
   DIFF_EQUAL
};

/**
* Class representing one diff operation.
*/
class Diff 
{
public:
  DiffOperation operation;
  // One of: INSERT, DELETE or EQUAL.
  String text;
  // The text associated with this diff operation.

  /**
   * Constructor.  Initializes the diff with the provided values.
   * @param operation One of INSERT, DELETE or EQUAL.
   * @param text The text being applied.
   */
  Diff(DiffOperation _operation, const String &_text);
  Diff(const Diff *src);
  Diff(const Diff &src);
  Diff();
  inline bool isNull() const;
  String toString() const;
  bool operator==(const Diff &d) const;
  bool operator!=(const Diff &d) const;

  static String strOperation(DiffOperation op);
};

/**
 * Class containing the diff, match and patch methods.
 * Also contains the behaviour settings.
 */
class DiffEngine
{
 public:
   // Defaults.
   // Set these on your diff_match_patch instance to override the defaults.

   // Number of milliseconds to map a diff before giving up (0 for infinity).
   int Diff_Timeout;
   
   // Cost of an empty edit operation in terms of edit characters.
   short Diff_EditCost;

public:
   DiffEngine();

  //  DIFF FUNCTIONS


  /**
   * Find the differences between two texts.
   * Run a faster slightly less optimal diff.
   * This method allows the 'checklines' of diff_main() to be optional.
   * Most of the time checklines is wanted, so default to true.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @return Linked List of Diff objects.
   */
  ObjectArray<Diff> *diff_main(const String &text1, const String &text2);

  /**
   * Find the differences between two texts.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * @return Linked List of Diff objects.
   */
  ObjectArray<Diff> *diff_main(const String &text1, const String &text2, bool checklines);

  /**
   * Find the differences between two texts.  Simplifies the problem by
   * stripping any common prefix or suffix off the texts before diffing.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * @param deadline Time when the diff should be complete by.  Used
   *     internally for recursive calls.  Users should set DiffTimeout instead.
   * @return Linked List of Diff objects.
   */
 private:
  ObjectArray<Diff> *diff_main(const String &text1, const String &text2, bool checklines, INT64 deadline);

  /**
   * Find the differences between two texts.  Assumes that the texts do not
   * have any common prefix or suffix.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * @param deadline Time when the diff should be complete by.
   * @return Linked List of Diff objects.
   */
 private:
  ObjectArray<Diff> *diff_compute(String text1, String text2, bool checklines, INT64 deadline);

  /**
   * Do a quick line-level diff on both strings, then rediff the parts for
   * greater accuracy.
   * This speedup can produce non-minimal diffs.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param deadline Time when the diff should be complete by.
   * @return Linked List of Diff objects.
   */
 private:
  ObjectArray<Diff> *diff_lineMode(const String& text1, const String& text2, INT64 deadline);

  /**
   * Find the 'middle snake' of a diff, split the problem in two
   * and return the recursively constructed diff.
   * See Myers 1986 paper: An O(ND) Difference Algorithm and Its Variations.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @return Linked List of Diff objects.
   */
 protected:
  ObjectArray<Diff> *diff_bisect(const String &text1, const String &text2, INT64 deadline);

  /**
   * Given the location of the 'middle snake', split the diff in two parts
   * and recurse.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param x Index of split point in text1.
   * @param y Index of split point in text2.
   * @param deadline Time at which to bail if not yet complete.
   * @return LinkedList of Diff objects.
   */
 private:
  ObjectArray<Diff> *diff_bisectSplit(const String &text1, const String &text2, int x, int y, INT64 deadline);

  /**
   * Split two texts into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * @param text1 First string.
   * @param text2 Second string.
   * @return Three element Object array, containing the encoded text1, the
   *     encoded text2 and the List of unique strings.  The zeroth element
   *     of the List of unique strings is intentionally blank.
   */
 protected:
  Array *diff_linesToChars(const String &text1, const String &text2); // return elems 0 and 1 are String, elem 2 is StringList

  /**
   * Split a text into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * @param text String to encode.
   * @param lineArray List of unique strings.
   * @param lineHash Map of strings to indices.
   * @return Encoded string.
   */
 private:
  String diff_linesToCharsMunge(const String &text, StringList &lineArray, StringIntMap<int>& lineHash);

  /**
   * Rehydrate the text in a diff from a string of line hashes to real lines of
   * text.
   * @param diffs LinkedList of Diff objects.
   * @param lineArray List of unique strings.
   */
 private:
  void diff_charsToLines(ObjectArray<Diff> *diffs, const StringList &lineArray);

  /**
   * Determine the common prefix of two strings.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the start of each string.
   */
 public:
  int diff_commonPrefix(const String &text1, const String &text2);

  /**
   * Determine the common suffix of two strings.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the end of each string.
   */
 public:
  int diff_commonSuffix(const String &text1, const String &text2);

  /**
   * Determine if the suffix of one string is the prefix of another.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the end of the first
   *     string and the start of the second string.
   */
 protected:
  size_t diff_commonOverlap(const String &text1, const String &text2);

  /**
   * Do the two texts share a substring which is at least half the length of
   * the longer text?
   * This speedup can produce non-minimal diffs.
   * @param text1 First string.
   * @param text2 Second string.
   * @return Five element String array, containing the prefix of text1, the
   *     suffix of text1, the prefix of text2, the suffix of text2 and the
   *     common middle.  Or null if there was no match.
   */
 protected:
  StringList *diff_halfMatch(const String &text1, const String &text2);

  /**
   * Does a substring of shorttext exist within longtext such that the
   * substring is at least half the length of longtext?
   * @param longtext Longer string.
   * @param shorttext Shorter string.
   * @param i Start index of quarter length substring within longtext.
   * @return Five element String array, containing the prefix of longtext, the
   *     suffix of longtext, the prefix of shorttext, the suffix of shorttext
   *     and the common middle.  Or null if there was no match.
   */
 private:
  StringList *diff_halfMatchI(const String &longtext, const String &shorttext, int i);

  /**
   * Reduce the number of edits by eliminating semantically trivial equalities.
   * @param diffs LinkedList of Diff objects.
   */
 public:
  void diff_cleanupSemantic(ObjectArray<Diff> &diffs);

  /**
   * Look for single edits surrounded on both sides by equalities
   * which can be shifted sideways to align the edit to a word boundary.
   * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
   * @param diffs LinkedList of Diff objects.
   */
 public:
  void diff_cleanupSemanticLossless(ObjectArray<Diff> &diffs);

  /**
   * Given two strings, compute a score representing whether the internal
   * boundary falls on logical boundaries.
   * Scores range from 6 (best) to 0 (worst).
   * @param one First string.
   * @param two Second string.
   * @return The score.
   */
 private:
  int diff_cleanupSemanticScore(const String &one, const String &two);

  /**
   * Reduce the number of edits by eliminating operationally trivial equalities.
   * @param diffs LinkedList of Diff objects.
   */
 public:
  void diff_cleanupEfficiency(ObjectArray<Diff> &diffs);

  /**
   * Reorder and merge like edit sections.  Merge equalities.
   * Any edit section can move as long as it doesn't cross an equality.
   * @param diffs LinkedList of Diff objects.
   */
 public:
  void diff_cleanupMerge(ObjectArray<Diff> &diffs);

  /**
   * loc is a location in text1, compute and return the equivalent location in
   * text2.
   * e.g. "The cat" vs "The big cat", 1->1, 5->8
   * @param diffs LinkedList of Diff objects.
   * @param loc Location within text1.
   * @return Location within text2.
   */
 public:
  int diff_xIndex(const ObjectArray<Diff> &diffs, int loc);

  /**
   * Convert a Diff list into a text report.
   * @param diffs LinkedList of Diff objects.
   * @return text representation.
   */
 public:
  String diff_generateLineDiff(ObjectArray<Diff> *diffs);

  /**
   * Compute and return the source text (all equalities and deletions).
   * @param diffs LinkedList of Diff objects.
   * @return Source text.
   */
 public:
  String diff_text1(const ObjectArray<Diff> &diffs);

  /**
   * Compute and return the destination text (all equalities and insertions).
   * @param diffs LinkedList of Diff objects.
   * @return Destination text.
   */
 public:
  String diff_text2(const ObjectArray<Diff> &diffs);

  /**
   * Compute the Levenshtein distance; the number of inserted, deleted or
   * substituted characters.
   * @param diffs LinkedList of Diff objects.
   * @return Number of changes.
   */
 public:
  int diff_levenshtein(const ObjectArray<Diff> &diffs);

  /**
   * Crush the diff into an encoded string which describes the operations
   * required to transform text1 into text2.
   * E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
   * Operations are tab-separated.  Inserted text is escaped using %xx notation.
   * @param diffs Array of diff tuples.
   * @return Delta text.
   */
 public:
  String diff_toDelta(const ObjectArray<Diff> &diffs);

  /**
   * Given the original text1, and an encoded string which describes the
   * operations required to transform text1 into text2, compute the full diff.
   * @param text1 Source string for the diff.
   * @param delta Delta text.
   * @return Array of diff tuples or null if invalid.
   * @throws String If invalid input.
   */
 public:
  ObjectArray<Diff> *diff_fromDelta(const String &text1, const String &delta);

  /**
   * A safer version of String.mid(pos).  This one returns "" instead of
   * null when the postion equals the string length.
   * @param str String to take a substring from.
   * @param pos Position to start the substring from.
   * @return Substring.
   */
 private:
  static inline String safeMid(const String &str, size_t pos)
  {
    return str.substring(pos, -1);
  }

  /**
   * A safer version of String.mid(pos, len).  This one returns "" instead of
   * null when the postion equals the string length.
   * @param str String to take a substring from.
   * @param pos Position to start the substring from.
   * @param len Length of substring.
   * @return Substring.
   */
 private:
  static inline String safeMid(const String &str, size_t pos, int len)
  {
    return str.substring(pos, len);
  }
};

#endif // DIFF_H
