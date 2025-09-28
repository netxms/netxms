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

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import com.netxms.mcp.tools.ServerTool;

/**
 * Tool for searching DocBook articles
 */
public class DocBookSearch extends ServerTool
{
   private final List<DocBookArticle> articles;
   private final String knowledgeBase;
   private final String description;

   /**
    * Constructor
    *
    * @param articles list of DocBook articles to search
    * @param knowledgeBase knowledge base identifier
    * @param description knowledge base description
    */
   public DocBookSearch(List<DocBookArticle> articles, String knowledgeBase, String description)
   {
      this.articles = articles;
      this.knowledgeBase = knowledgeBase;
      this.description = description;
   }

   @Override
   public String getName()
   {
      return "docbook-search-" + knowledgeBase;
   }

   @Override
   public String getDescription()
   {
      return "Search for articles in " + description + " knowledge base. Returns matching article titles and IDs.";
   }

   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addStringArgument("query", "Search query to find relevant articles (searches in titles and content)", true)
         .addIntegerArgument("max_results", "Maximum number of results to return (default: 10)", false)
         .build();
   }

   @Override
   public String execute(Map<String, Object> arguments) throws Exception
   {
      String query = arguments.containsKey("query") ? (String) arguments.get("query") : "";
      int maxResults = arguments.containsKey("max_results") ? 
         ((Number) arguments.get("max_results")).intValue() : 10;

      if (query.trim().isEmpty())
      {
         return "Error: Search query cannot be empty";
      }

      // Create a list of articles with their relevance scores
      List<ArticleMatch> matches = articles.stream()
         .map(article -> new ArticleMatch(article, calculateRelevanceScore(article, query)))
         .filter(match -> match.score > 0) // Only include articles that match
         .sorted((a, b) -> Double.compare(b.score, a.score)) // Sort by relevance (highest first)
         .limit(maxResults)
         .collect(Collectors.toList());

      if (matches.isEmpty())
      {
         return "No articles found matching query: " + query;
      }

      StringBuilder result = new StringBuilder();
      result.append("Found ").append(matches.size()).append(" article(s) matching \"").append(query).append("\":\n\n");

      for (int i = 0; i < matches.size(); i++)
      {
         ArticleMatch match = matches.get(i);
         DocBookArticle article = match.article;
         result.append(i + 1).append(". ");
         result.append("Title: ").append(article.getTitle());
         
         // Add relevance indicator for fuzzy matches
         if (match.score < 100)
         {
            result.append(" (").append(String.format("%.0f", match.score)).append("% match)");
         }
         
         result.append("\n");
         result.append("   ID: ").append(article.getId()).append("\n");
         result.append("   Source: ").append(article.getFileName()).append("\n");
         result.append("   URI: ").append(article.getUri()).append("\n");
         
         // Add a snippet of matching content
         String content = article.getContent();
         String snippet = findBestMatchingSnippet(content, query);
         if (snippet != null)
         {
            result.append("   Snippet: ").append(snippet).append("\n");
         }
         
         result.append("\n");
      }

      result.append("Use docbook-get-article-").append(knowledgeBase).append(" tool to retrieve the full content of any article.");
      return result.toString();
   }
   
   /**
    * Calculate relevance score for an article based on the search query
    */
   private double calculateRelevanceScore(DocBookArticle article, String query)
   {
      if (!article.matches(query))
         return 0;
      
      String lowerQuery = query.toLowerCase().trim();
      String lowerTitle = article.getTitle().toLowerCase();
      String lowerContent = article.getContent().toLowerCase();
      String lowerId = article.getId().toLowerCase();
      
      double score = 0;
      
      // Exact matches get highest scores
      if (lowerTitle.equals(lowerQuery))
         score += 100;
      else if (lowerTitle.contains(lowerQuery))
         score += 80;
      else if (lowerId.contains(lowerQuery))
         score += 70;
      
      // Content matches get lower scores
      if (lowerContent.contains(lowerQuery))
         score += 30;
      
      // Word-based scoring for partial matches
      String[] queryWords = lowerQuery.split("\\s+");
      for (String word : queryWords)
      {
         if (word.length() < 2) continue;
         
         // Title word matches
         if (containsExactWord(lowerTitle, word))
            score += 20;
         else if (containsPartialWordMatch(lowerTitle, word))
            score += 10;
         
         // Content word matches
         if (containsExactWord(lowerContent, word))
            score += 5;
         else if (containsPartialWordMatch(lowerContent, word))
            score += 2;
      }
      
      // Bonus for shorter titles (more specific matches)
      if (score > 0 && article.getTitle().length() < 50)
         score += 5;
      
      return Math.min(score, 100); // Cap at 100%
   }
   
   /**
    * Check if text contains exact word match
    */
   private boolean containsExactWord(String text, String word)
   {
      return text.matches(".*\\b" + java.util.regex.Pattern.quote(word) + "\\b.*");
   }
   
   /**
    * Check if text contains partial word match
    */
   private boolean containsPartialWordMatch(String text, String word)
   {
      String[] textWords = text.split("\\s+");
      for (String textWord : textWords)
      {
         textWord = textWord.replaceAll("[^a-zA-Z0-9]", "");
         if (textWord.length() >= 3 && word.length() >= 3)
         {
            if (textWord.contains(word) || word.contains(textWord))
            {
               int minLength = Math.min(textWord.length(), word.length());
               int maxLength = Math.max(textWord.length(), word.length());
               if ((double) minLength / maxLength >= 0.6)
                  return true;
            }
         }
      }
      return false;
   }
   
   /**
    * Find the best matching snippet from content
    */
   private String findBestMatchingSnippet(String content, String query)
   {
      String lowerContent = content.toLowerCase();
      String lowerQuery = query.toLowerCase();
      
      // Try exact query match first
      int index = lowerContent.indexOf(lowerQuery);
      if (index >= 0)
      {
         return extractSnippet(content, index, query.length());
      }
      
      // Try individual words
      String[] queryWords = lowerQuery.split("\\s+");
      for (String word : queryWords)
      {
         if (word.length() >= 3)
         {
            index = lowerContent.indexOf(word);
            if (index >= 0)
            {
               return extractSnippet(content, index, word.length());
            }
         }
      }
      
      return null;
   }
   
   /**
    * Extract a snippet around the matching text
    */
   private String extractSnippet(String content, int matchIndex, int matchLength)
   {
      int start = Math.max(0, matchIndex - 50);
      int end = Math.min(content.length(), matchIndex + matchLength + 50);
      String snippet = content.substring(start, end);
      
      if (start > 0) snippet = "..." + snippet;
      if (end < content.length()) snippet = snippet + "...";
      
      return snippet.replaceAll("\\s+", " ").trim();
   }
   
   /**
    * Helper class to store article with its relevance score
    */
   private static class ArticleMatch
   {
      final DocBookArticle article;
      final double score;
      
      ArticleMatch(DocBookArticle article, double score)
      {
         this.article = article;
         this.score = score;
      }
   }
}