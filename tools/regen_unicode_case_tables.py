#!/usr/bin/env python3
"""
Regenerate the Unicode simple case mapping tables in
src/libnetxms/unicode_case.cpp from the Unicode Character Database.

Fields 12 (simple uppercase mapping) and 13 (simple lowercase mapping) of
UnicodeData.txt are read and compressed into (start, end, step, delta) ranges,
so that codepoints start, start + step, start + 2 * step, ... end all map to
codepoint + delta. Full case mappings from SpecialCasing.txt are deliberately
ignored: they are either locale specific (Turkish, Azeri, Lithuanian) or expand
to more than one codepoint, and neither fits a per-character mapping function.

UnicodeData.txt is downloaded from unicode.org unless --ucd-file is given.

Run from repository root:
    python3 tools/regen_unicode_case_tables.py
"""

import argparse
import re
import sys
import urllib.request

UNICODE_VERSION = "17.0.0"
UCD_URL = f"https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/UnicodeData.txt"
TARGET = "src/libnetxms/unicode_case.cpp"

BEGIN_MARKER = "/* BEGIN GENERATED TABLES */"
END_MARKER = "/* END GENERATED TABLES */"


def load_mappings(text):
   """Extract simple uppercase and lowercase mappings from UnicodeData.txt."""
   upper, lower = {}, {}
   for line in text.splitlines():
      f = line.split(';')
      if len(f) < 15:
         continue
      # First>/Last> entries define ranges of CJK ideographs and similar
      # blocks; none of them carry case mappings
      if 'First>' in f[1] or 'Last>' in f[1]:
         if f[12] or f[13]:
            raise ValueError(f"range entry {f[0]} carries a case mapping")
         continue
      cp = int(f[0], 16)
      if f[12]:
         upper[cp] = int(f[12], 16)
      if f[13]:
         lower[cp] = int(f[13], 16)
   return upper, lower


def compress(mapping):
   """Compress codepoint -> codepoint mapping into (start, end, step, delta) ranges."""
   items = sorted(mapping.items())
   ranges = []
   i = 0
   while i < len(items):
      cp, target = items[i]
      delta = target - cp
      best_end, best_step, best_count = cp, 1, 1
      # Step 1 covers contiguous runs (Cyrillic, Greek), step 2 covers the
      # alternating upper/lower pairs typical for Latin Extended
      for step in (1, 2):
         end, count, j = cp, 1, i + 1
         while j < len(items) and items[j][0] == end + step and items[j][1] - items[j][0] == delta:
            end, count, j = items[j][0], count + 1, j + 1
         if count > best_count:
            best_end, best_step, best_count = end, step, count
      ranges.append((cp, best_end, best_step, delta))
      i += best_count
   return ranges


def verify(mapping, ranges):
   """Check that the compressed ranges reproduce the mapping over the whole codepoint space."""
   lookup = {}
   for start, end, step, delta in ranges:
      for cp in range(start, end + 1, step):
         if cp in lookup:
            raise ValueError(f"overlapping ranges at U+{cp:04X}")
         lookup[cp] = cp + delta
   if lookup != mapping:
      raise ValueError("compressed ranges do not reproduce source mapping")
   for i in range(1, len(ranges)):
      if ranges[i][0] <= ranges[i - 1][1]:
         raise ValueError(f"ranges not sorted/disjoint at U+{ranges[i][0]:04X}")


def format_table(name, comment, ranges):
   lines = [f"/**", f" * {comment}", f" */", f"static const CaseMappingRange {name}[] =", "{"]
   for start, end, step, delta in ranges:
      lines.append(f"   {{ 0x{start:05X}, 0x{end:05X}, {step}, {delta} }},")
   lines.append("};")
   return "\n".join(lines)


def main():
   parser = argparse.ArgumentParser(description=__doc__)
   parser.add_argument("--ucd-file", help="local copy of UnicodeData.txt (downloaded if not given)")
   parser.add_argument("--target", default=TARGET, help=f"file to update (default: {TARGET})")
   args = parser.parse_args()

   if args.ucd_file:
      with open(args.ucd_file, encoding="utf-8") as f:
         text = f.read()
   else:
      print(f"Downloading {UCD_URL}")
      with urllib.request.urlopen(UCD_URL) as response:
         text = response.read().decode("utf-8")

   upper, lower = load_mappings(text)
   upper_ranges, lower_ranges = compress(upper), compress(lower)
   verify(upper, upper_ranges)
   verify(lower, lower_ranges)

   print(f"uppercase: {len(upper)} mappings -> {len(upper_ranges)} ranges")
   print(f"lowercase: {len(lower)} mappings -> {len(lower_ranges)} ranges")

   block = "\n\n".join([
      format_table("s_upperCaseMapping",
         f"Simple uppercase mappings from Unicode {UNICODE_VERSION}", upper_ranges),
      format_table("s_lowerCaseMapping",
         f"Simple lowercase mappings from Unicode {UNICODE_VERSION}", lower_ranges)])

   with open(args.target, encoding="utf-8") as f:
      source = f.read()
   pattern = re.compile(
      re.escape(BEGIN_MARKER) + ".*?" + re.escape(END_MARKER), re.DOTALL)
   if not pattern.search(source):
      sys.exit(f"markers not found in {args.target}")
   source = pattern.sub(f"{BEGIN_MARKER}\n\n{block}\n\n{END_MARKER}", source)
   with open(args.target, "w", encoding="utf-8") as f:
      f.write(source)
   print(f"Updated {args.target}")


if __name__ == "__main__":
   main()
