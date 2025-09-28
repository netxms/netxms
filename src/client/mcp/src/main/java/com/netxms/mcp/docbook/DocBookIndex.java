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
import com.netxms.mcp.resources.ServerResource;
import io.modelcontextprotocol.spec.McpSchema.ReadResourceRequest;

/**
 * Resource providing an index of all DocBook articles
 */
public class DocBookIndex extends ServerResource
{
   private final List<DocBookArticle> articles;

   /**
    * Constructor
    *
    * @param articles list of DocBook articles
    */
   public DocBookIndex(List<DocBookArticle> articles)
   {
      this.articles = articles;
   }

   @Override
   public String getUri()
   {
      return "docbook://index";
   }

   @Override
   public String getName()
   {
      return "DocBook Article Index";
   }

   @Override
   public String getDescription()
   {
      return "Index of all available DocBook articles in the knowledge base";
   }

   @Override
   public String getMimeType()
   {
      return "text/plain";
   }

   @Override
   public String execute(ReadResourceRequest request) throws Exception
   {
      StringBuilder index = new StringBuilder();
      
      index.append("DocBook Knowledge Base Index\n");
      index.append("=".repeat(40)).append("\n\n");
      index.append("Total articles: ").append(articles.size()).append("\n\n");
      
      // Group articles by source file
      java.util.Map<String, java.util.List<DocBookArticle>> articlesByFile = articles.stream()
         .collect(java.util.stream.Collectors.groupingBy(DocBookArticle::getFileName));
      
      for (java.util.Map.Entry<String, java.util.List<DocBookArticle>> entry : articlesByFile.entrySet())
      {
         String fileName = entry.getKey();
         java.util.List<DocBookArticle> fileArticles = entry.getValue();
         
         index.append("Source File: ").append(fileName).append("\n");
         index.append("-".repeat(fileName.length() + 13)).append("\n");
         
         for (DocBookArticle article : fileArticles)
         {
            index.append("• ").append(article.getTitle()).append("\n");
            index.append("  ID: ").append(article.getId()).append("\n");
            index.append("  URI: ").append(article.getUri()).append("\n");
            
            // Add a brief content preview (first 100 characters)
            String content = article.getContent();
            if (content.length() > 100)
            {
               content = content.substring(0, 97) + "...";
            }
            content = content.replaceAll("\\s+", " ").trim();
            index.append("  Preview: ").append(content).append("\n");
            index.append("\n");
         }
         
         index.append("\n");
      }
      
      index.append("=".repeat(40)).append("\n");
      index.append("Use docbook_search_* tools to search for specific content\n");
      index.append("Use docbook_get_article_* tools to retrieve full article content");
      
      return index.toString();
   }
}