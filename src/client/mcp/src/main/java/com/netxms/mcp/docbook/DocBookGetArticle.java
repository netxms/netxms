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
import java.util.Optional;
import com.netxms.mcp.tools.ServerTool;

/**
 * Tool for retrieving full DocBook article content by ID
 */
public class DocBookGetArticle extends ServerTool
{
   private final List<DocBookArticle> articles;
   private final String knowledgeBase;
   private final String description;

   /**
    * Constructor
    *
    * @param articles list of DocBook articles
    * @param knowledgeBase knowledge base identifier
    * @param description knowledge base description
    */
   public DocBookGetArticle(List<DocBookArticle> articles, String knowledgeBase, String description)
   {
      this.articles = articles;
      this.knowledgeBase = knowledgeBase;
      this.description = description;
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "docbook-get-article-" + knowledgeBase;
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Retrieve full content of a specific article from " + description + " knowledge base by article ID.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addStringArgument("article_id", "ID of the article to retrieve (use docbook-search-" + knowledgeBase + " to find article IDs)", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> arguments) throws Exception
   {
      String articleId = arguments.containsKey("article_id") ? (String) arguments.get("article_id") : "";

      if (articleId.trim().isEmpty())
      {
         return "Error: Article ID is required";
      }

      Optional<DocBookArticle> foundArticle = articles.stream()
         .filter(article -> article.getId().equals(articleId))
         .findFirst();

      if (!foundArticle.isPresent())
      {
         // Try fuzzy matching by partial ID or title
         foundArticle = articles.stream()
            .filter(article -> article.getId().contains(articleId) || 
                              article.getTitle().toLowerCase().contains(articleId.toLowerCase()))
            .findFirst();
      }

      if (!foundArticle.isPresent())
      {
         StringBuilder availableIds = new StringBuilder();
         availableIds.append("Article not found with ID: ").append(articleId).append("\n\n");
         availableIds.append("Available articles:\n");
         
         for (DocBookArticle article : articles)
         {
            availableIds.append("- ID: ").append(article.getId())
                       .append(", Title: ").append(article.getTitle()).append("\n");
         }
         
         return availableIds.toString();
      }

      DocBookArticle article = foundArticle.get();
      
      StringBuilder result = new StringBuilder();
      result.append("=".repeat(60)).append("\n");
      result.append("ARTICLE: ").append(article.getTitle()).append("\n");
      result.append("=".repeat(60)).append("\n");
      result.append("ID: ").append(article.getId()).append("\n");
      result.append("Source File: ").append(article.getFileName()).append("\n");
      result.append("Knowledge Base: ").append(description).append("\n");
      result.append("URI: ").append(article.getUri()).append("\n");
      result.append("=".repeat(60)).append("\n\n");
      
      result.append(article.getContent());
      
      result.append("\n\n").append("=".repeat(60));
      result.append("\nEnd of article: ").append(article.getTitle());
      result.append("\n").append("=".repeat(60));

      return result.toString();
   }
}