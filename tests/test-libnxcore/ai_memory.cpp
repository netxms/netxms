/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: ai_memory.cpp
**
** Unit tests for AI memory prompt block formatting: ordering, size limit, omission note.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nxai.h>
#include <testtools.h>

/**
 * Create test entry
 */
static shared_ptr<AIMemoryEntry> MakeEntry(uint32_t id, const char *title, const char *content, time_t updatedAt, bool byModel = true)
{
   return make_shared<AIMemoryEntry>(id, AIMemoryScope::ENVIRONMENT, 0, title, content, byModel, 1, byModel ? 'C' : AI_MEMORY_SOURCE_HUMAN, 0, updatedAt);
}

/**
 * Empty list produces empty block
 */
static void TestEmptyList()
{
   StartTest(_T("Empty entry list"));
   std::vector<shared_ptr<AIMemoryEntry>> entries;
   AssertTrue(FormatAIMemoryPromptBlock(entries, 0, "environment_memory", "intro").empty());
   EndTest();
}

/**
 * Block structure and ordering: most recently updated entries first
 */
static void TestOrdering()
{
   StartTest(_T("Entry ordering"));
   std::vector<shared_ptr<AIMemoryEntry>> entries;
   entries.push_back(MakeEntry(1, "Oldest", "first content", 1000));
   entries.push_back(MakeEntry(2, "Newest", "second content", 3000, false));
   entries.push_back(MakeEntry(3, "Middle", "third content", 2000));

   std::string block = FormatAIMemoryPromptBlock(entries, 0, "environment_memory", "Intro text.");
   AssertTrue(block.compare(0, 20, "<environment_memory>") == 0);
   AssertTrue(block.find("Intro text.\n") != std::string::npos);
   AssertTrue(block.compare(block.length() - 21, 21, "</environment_memory>") == 0);

   size_t newest = block.find("[#2, ");
   size_t middle = block.find("[#3, ");
   size_t oldest = block.find("[#1, ");
   AssertTrue(newest != std::string::npos);
   AssertTrue(middle != std::string::npos);
   AssertTrue(oldest != std::string::npos);
   AssertTrue(newest < middle);
   AssertTrue(middle < oldest);

   AssertTrue(block.find("recorded by administrator] Newest: second content") != std::string::npos);
   AssertTrue(block.find("recorded by assistant] Oldest: first content") != std::string::npos);
   AssertTrue(block.find("omitted") == std::string::npos);
   EndTest();
}

/**
 * Entries with equal update time are ordered by ID, newest first
 */
static void TestTieBreak()
{
   StartTest(_T("Ordering tie break by ID"));
   std::vector<shared_ptr<AIMemoryEntry>> entries;
   entries.push_back(MakeEntry(10, "A", "a", 5000));
   entries.push_back(MakeEntry(20, "B", "b", 5000));
   std::string block = FormatAIMemoryPromptBlock(entries, 0, "user_memory", "intro");
   AssertTrue(block.find("[#20, ") < block.find("[#10, "));
   EndTest();
}

/**
 * Size limit: older entries are omitted, block stays within limit, omission note is present
 */
static void TestSizeLimit()
{
   StartTest(_T("Size limit"));
   std::vector<shared_ptr<AIMemoryEntry>> entries;
   std::string longContent(300, 'x');
   for(uint32_t i = 1; i <= 5; i++)
      entries.push_back(MakeEntry(i, ("Entry " + std::to_string(i)).c_str(), longContent.c_str(), 1000 * i));

   size_t limit = 900;
   std::string block = FormatAIMemoryPromptBlock(entries, limit, "environment_memory", "intro");
   AssertTrue(block.length() <= limit);
   AssertTrue(block.find("[#5, ") != std::string::npos);   // newest always first candidate
   AssertTrue(block.find("[#1, ") == std::string::npos);   // oldest cannot fit
   AssertTrue(block.find("more entries omitted due to size limit; use recall-memory") != std::string::npos);
   AssertTrue(block.compare(block.length() - 21, 21, "</environment_memory>") == 0);

   // Unlimited size keeps everything
   block = FormatAIMemoryPromptBlock(entries, 0, "environment_memory", "intro");
   for(uint32_t i = 1; i <= 5; i++)
      AssertTrue(block.find("[#" + std::to_string(i) + ", ") != std::string::npos);
   AssertTrue(block.find("omitted") == std::string::npos);
   EndTest();
}

/**
 * Limit too small for any entry still yields a well-formed block with the note
 */
static void TestLimitBelowSingleEntry()
{
   StartTest(_T("Limit below single entry"));
   std::vector<shared_ptr<AIMemoryEntry>> entries;
   entries.push_back(MakeEntry(1, "Big", std::string(2000, 'y').c_str(), 1000));
   std::string block = FormatAIMemoryPromptBlock(entries, 400, "object_memory", "intro");
   AssertTrue(block.length() <= 400);
   AssertTrue(block.find("[#1, ") == std::string::npos);
   AssertTrue(block.find("1 more entries omitted") != std::string::npos);
   AssertTrue(block.compare(block.length() - 16, 16, "</object_memory>") == 0);
   EndTest();
}

/**
 * Test AI memory prompt block formatting
 */
void TestAIMemoryPromptBlock()
{
   TestEmptyList();
   TestOrdering();
   TestTieBreak();
   TestSizeLimit();
   TestLimitBelowSingleEntry();
}
