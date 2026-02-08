/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package com.netxms.mcp.docbook;

import com.netxms.mcp.resources.ServerResource;
import io.modelcontextprotocol.spec.McpSchema.ReadResourceRequest;

/**
 * Represents a DocBook article that can be served as an MCP resource
 */
public class DocBookArticle extends ServerResource
{
   private final String id;
   private final String title;
   private final String content;
   private final String fileName;

   /**
    * Constructor
    *
    * @param id article ID
    * @param title article title
    * @param content article content
    * @param fileName source file name
    */
   public DocBookArticle(String id, String title, String content, String fileName)
   {
      this.id = id;
      this.title = title;
      this.content = content;
      this.fileName = fileName;
   }

   /**
    * Get article ID
    *
    * @return article ID
    */
   public String getId()
   {
      return id;
   }

   /**
    * Get article title
    *
    * @return article title
    */
   public String getTitle()
   {
      return title;
   }

   /**
    * Get article content
    *
    * @return article content
    */
   public String getContent()
   {
      return content;
   }

   /**
    * Get source file name
    *
    * @return source file name
    */
   public String getFileName()
   {
      return fileName;
   }

   @Override
   public String getUri()
   {
      return "docbook://article/" + id;
   }

   @Override
   public String getName()
   {
      return title;
   }

   @Override
   public String getDescription()
   {
      return "DocBook article: " + title + " (from " + fileName + ")";
   }

   @Override
   public String getMimeType()
   {
      return "text/plain";
   }

   @Override
   public String execute(ReadResourceRequest request) throws Exception
   {
      return "Title: " + title + "\n" +
             "ID: " + id + "\n" +
             "Source: " + fileName + "\n\n" +
             content;
   }

   /**
    * Check if article matches search query using fuzzy matching
    *
    * @param query search query
    * @return true if article matches the query
    */
   public boolean matches(String query)
   {
      if (query == null || query.trim().isEmpty())
         return true;
      
      String lowerQuery = query.toLowerCase().trim();
      String lowerTitle = title.toLowerCase();
      String lowerContent = content.toLowerCase();
      String lowerId = id.toLowerCase();
      
      // Direct substring match (highest priority)
      if (lowerTitle.contains(lowerQuery) || lowerContent.contains(lowerQuery) || lowerId.contains(lowerQuery))
         return true;
      
      // Split query into words for partial matching
      String[] queryWords = lowerQuery.split("\\s+");
      if (queryWords.length == 1)
      {
         // Single word fuzzy matching
         return fuzzyMatchSingleWord(queryWords[0], lowerTitle, lowerContent, lowerId);
      }
      else
      {
         // Multiple words - check if all words match somewhere
         return fuzzyMatchMultipleWords(queryWords, lowerTitle, lowerContent, lowerId);
      }
   }
   
   /**
    * Fuzzy match for single word queries
    */
   private boolean fuzzyMatchSingleWord(String word, String title, String content, String id)
   {
      // Exact word match
      if (containsWord(title, word) || containsWord(content, word) || containsWord(id, word))
         return true;
      
      // Partial word match (word contains the query or query contains significant part of word)
      if (word.length() >= 3)
      {
         if (containsPartialWord(title, word) || containsPartialWord(content, word) || containsPartialWord(id, word))
            return true;
      }
      
      // Simple edit distance for typos (only for words >= 4 characters)
      if (word.length() >= 4)
      {
         return containsFuzzyWord(title, word) || containsFuzzyWord(content, word) || containsFuzzyWord(id, word);
      }
      
      return false;
   }
   
   /**
    * Fuzzy match for multiple word queries
    */
   private boolean fuzzyMatchMultipleWords(String[] queryWords, String title, String content, String id)
   {
      int matchedWords = 0;
      
      for (String word : queryWords)
      {
         if (word.length() < 2) continue; // Skip very short words
         
         if (fuzzyMatchSingleWord(word, title, content, id))
         {
            matchedWords++;
         }
      }
      
      // Require at least 70% of words to match for multi-word queries
      double matchRatio = (double) matchedWords / queryWords.length;
      return matchRatio >= 0.7;
   }
   
   /**
    * Check if text contains the word as a complete word
    */
   private boolean containsWord(String text, String word)
   {
      return text.matches(".*\\b" + java.util.regex.Pattern.quote(word) + "\\b.*");
   }
   
   /**
    * Check if text contains partial word matches
    */
   private boolean containsPartialWord(String text, String word)
   {
      String[] textWords = text.split("\\s+");
      for (String textWord : textWords)
      {
         // Remove punctuation
         textWord = textWord.replaceAll("[^a-zA-Z0-9]", "");
         if (textWord.length() < 3) continue;
         
         // Check if query word is contained in text word or vice versa
         if (textWord.contains(word) || word.contains(textWord))
         {
            // Ensure significant overlap
            int minLength = Math.min(textWord.length(), word.length());
            int maxLength = Math.max(textWord.length(), word.length());
            if (minLength >= 3 && (double) minLength / maxLength >= 0.6)
               return true;
         }
      }
      return false;
   }
   
   /**
    * Check if text contains words with small edit distance (typo tolerance)
    */
   private boolean containsFuzzyWord(String text, String word)
   {
      String[] textWords = text.split("\\s+");
      for (String textWord : textWords)
      {
         textWord = textWord.replaceAll("[^a-zA-Z0-9]", "");
         if (textWord.length() < 3) continue;
         
         // Only check words of similar length
         if (Math.abs(textWord.length() - word.length()) <= 2)
         {
            int distance = editDistance(textWord, word);
            int maxAllowedDistance = Math.max(1, word.length() / 4); // Allow 25% character differences
            if (distance <= maxAllowedDistance)
               return true;
         }
      }
      return false;
   }
   
   /**
    * Calculate edit distance between two strings (Levenshtein distance)
    */
   private int editDistance(String s1, String s2)
   {
      int[][] dp = new int[s1.length() + 1][s2.length() + 1];
      
      for (int i = 0; i <= s1.length(); i++)
         dp[i][0] = i;
      for (int j = 0; j <= s2.length(); j++)
         dp[0][j] = j;
      
      for (int i = 1; i <= s1.length(); i++)
      {
         for (int j = 1; j <= s2.length(); j++)
         {
            if (s1.charAt(i - 1) == s2.charAt(j - 1))
            {
               dp[i][j] = dp[i - 1][j - 1];
            }
            else
            {
               dp[i][j] = 1 + Math.min(dp[i - 1][j], Math.min(dp[i][j - 1], dp[i - 1][j - 1]));
            }
         }
      }
      
      return dp[s1.length()][s2.length()];
   }
}
