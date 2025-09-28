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

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
 * Loader for DocBook XML files
 */
public class DocBookLoader
{
   private static final Logger logger = LoggerFactory.getLogger(DocBookLoader.class);

   /**
    * Load DocBook articles from a directory containing XML files
    *
    * @param directory directory path
    * @return list of DocBook articles
    * @throws IOException if directory cannot be read
    */
   public static List<DocBookArticle> loadFromDirectory(Path directory) throws IOException
   {
      List<DocBookArticle> articles = new ArrayList<>();
      
      try (Stream<Path> paths = Files.walk(directory))
      {
         paths.filter(Files::isRegularFile)
              .filter(path -> path.toString().toLowerCase().endsWith(".xml"))
              .forEach(path -> {
                 try
                 {
                    List<DocBookArticle> fileArticles = load(path);
                    articles.addAll(fileArticles);
                 }
                 catch (Exception e)
                 {
                    logger.warn("Failed to load DocBook file {}: {}", path, e.getMessage());
                 }
              });
      }
      
      return articles;
   }

   /**
    * Load DocBook articles from a single XML file
    *
    * @param filePath file path
    * @return list of DocBook articles
    * @throws Exception if file cannot be parsed
    */
   public static List<DocBookArticle> load(Path filePath) throws Exception
   {
      List<DocBookArticle> articles = new ArrayList<>();
      
      DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
      factory.setNamespaceAware(true);
      DocumentBuilder builder = factory.newDocumentBuilder();
      Document doc = builder.parse(filePath.toFile());
      
      String fileName = filePath.getFileName().toString();
      
      // Parse different DocBook structures
      Element root = doc.getDocumentElement();
      
      if ("article".equals(root.getTagName()) || "chapter".equals(root.getTagName()) || 
          "section".equals(root.getTagName()) || "book".equals(root.getTagName()))
      {
         // Single article/chapter/section document
         DocBookArticle article = parseElement(root, fileName);
         if (article != null)
            articles.add(article);
      }
      
      // Look for nested articles, chapters, sections
      articles.addAll(parseNestedElements(root, fileName));
      
      return articles;
   }

   /**
    * Parse nested DocBook elements (articles, chapters, sections)
    *
    * @param parent parent element
    * @param fileName source file name
    * @return list of articles found
    */
   private static List<DocBookArticle> parseNestedElements(Element parent, String fileName)
   {
      List<DocBookArticle> articles = new ArrayList<>();
      
      NodeList children = parent.getChildNodes();
      for (int i = 0; i < children.getLength(); i++)
      {
         Node node = children.item(i);
         if (node instanceof Element)
         {
            Element element = (Element) node;
            String tagName = element.getTagName();
            
            if ("article".equals(tagName) || "chapter".equals(tagName) || 
                "section".equals(tagName) || "sect1".equals(tagName) ||
                "sect2".equals(tagName) || "sect3".equals(tagName))
            {
               DocBookArticle article = parseElement(element, fileName);
               if (article != null)
                  articles.add(article);
            }
            
            // Recursively search for more nested elements
            articles.addAll(parseNestedElements(element, fileName));
         }
      }
      
      return articles;
   }

   /**
    * Parse a DocBook element into an article
    *
    * @param element XML element
    * @param fileName source file name
    * @return DocBook article or null if parsing failed
    */
   private static DocBookArticle parseElement(Element element, String fileName)
   {
      try
      {
         String id = element.getAttribute("id");
         if (id == null || id.trim().isEmpty())
         {
            // Generate ID from tag name and position if not present
            id = element.getTagName() + "_" + System.identityHashCode(element);
         }
         
         String title = extractTitle(element);
         if (title == null || title.trim().isEmpty())
         {
            title = "Untitled " + element.getTagName();
         }
         
         String content = extractTextContent(element);
         
         return new DocBookArticle(id, title, content, fileName);
      }
      catch (Exception e)
      {
         logger.warn("Failed to parse DocBook element in {}: {}", fileName, e.getMessage());
         return null;
      }
   }

   /**
    * Extract title from DocBook element
    *
    * @param element XML element
    * @return title text or null if not found
    */
   private static String extractTitle(Element element)
   {
      // Try different title elements
      String[] titleTags = {"title", "info/title", "articleinfo/title", "chapterinfo/title", "sectioninfo/title"};
      
      for (String titleTag : titleTags)
      {
         NodeList titleNodes = element.getElementsByTagName(titleTag);
         if (titleNodes.getLength() > 0)
         {
            String title = titleNodes.item(0).getTextContent();
            if (title != null && !title.trim().isEmpty())
               return title.trim();
         }
      }
      
      // Try to find any title element
      NodeList titleNodes = element.getElementsByTagName("title");
      if (titleNodes.getLength() > 0)
      {
         return titleNodes.item(0).getTextContent().trim();
      }
      
      return null;
   }

   /**
    * Extract text content from DocBook element, removing XML tags
    *
    * @param element XML element
    * @return clean text content
    */
   private static String extractTextContent(Element element)
   {
      StringBuilder content = new StringBuilder();
      extractTextRecursive(element, content);
      return content.toString().trim();
   }

   /**
    * Recursively extract text content from element and its children
    *
    * @param node XML node
    * @param content string builder to append text to
    */
   private static void extractTextRecursive(Node node, StringBuilder content)
   {
      if (node.getNodeType() == Node.TEXT_NODE)
      {
         String text = node.getTextContent();
         if (text != null && !text.trim().isEmpty())
         {
            content.append(text.trim()).append(" ");
         }
      }
      else if (node.getNodeType() == Node.ELEMENT_NODE)
      {
         Element element = (Element) node;
         String tagName = element.getTagName();
         
         // Add some formatting for certain elements
         if ("para".equals(tagName) || "section".equals(tagName) || 
             "chapter".equals(tagName) || "article".equals(tagName))
         {
            content.append("\n");
         }
         else if ("title".equals(tagName))
         {
            content.append("\n=== ");
         }
         
         NodeList children = node.getChildNodes();
         for (int i = 0; i < children.getLength(); i++)
         {
            extractTextRecursive(children.item(i), content);
         }
         
         if ("para".equals(tagName))
         {
            content.append("\n");
         }
         else if ("title".equals(tagName))
         {
            content.append(" ===\n");
         }
      }
   }
}