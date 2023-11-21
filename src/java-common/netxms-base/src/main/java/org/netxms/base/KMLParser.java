/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
package org.netxms.base;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
 * KML file format parser
 */
public final class KMLParser
{
   private static Logger logger = LoggerFactory.getLogger(KMLParser.class);

   /**
    * KML polygon parser
    * 
    * @param file path to the file with KML definition
    * @return map with polygon name and polygon coordinates
    */
   public static Map<String, List<GeoLocation>> importPolygons(File file)
   {
      if (!file.exists())
         return null;

      DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
      Document doc = null;
      try
      {
         factory.setFeature("http://apache.org/xml/features/disallow-doctype-decl", true);
         DocumentBuilder builder = factory.newDocumentBuilder();
         doc = builder.parse(file);
      }
      catch(Exception e)
      {
         logger.warn("Cannot load or parse KML file " + file.getPath(), e);
      }
      return (doc != null) ? importPolygons(doc) : null;
   }

   /**
    * KML polygon parser
    * 
    * @param content KML content
    * @return map with polygon name and polygon coordinates
    */
   public static Map<String, List<GeoLocation>> importPolygons(String content)
   {
      DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
      Document doc = null;
      try
      {
         factory.setFeature("http://apache.org/xml/features/disallow-doctype-decl", true);
         DocumentBuilder builder = factory.newDocumentBuilder();
         ByteArrayInputStream in = new ByteArrayInputStream(content.getBytes("UTF-8"));
         doc = builder.parse(in);
      }
      catch(Exception e)
      {
         logger.warn("Cannot load or parse KML from memory", e);
      }
      return (doc != null) ? importPolygons(doc) : null;
   }

   /**
    * KML polygon parser
    * 
    * @param doc KML file as XML document
    * @return map with polygon name and polygon coordinates
    */
   private static Map<String, List<GeoLocation>> importPolygons(Document doc)
   {
      Map<String, List<GeoLocation>> map = new HashMap<String, List<GeoLocation>>();
      NodeList nList = doc.getElementsByTagName("Placemark");
      for(int i = 0; i < nList.getLength(); i++)
      {
         Node nNode = nList.item(i);

         // Should parse only if it contains polygon inside
         if (nNode.getNodeType() == Node.ELEMENT_NODE)
         {
            NodeList polygon = ((Element)nNode).getElementsByTagName("Polygon");
            if (polygon.getLength() < 1)
               continue;

            NodeList nameElement = ((Element)nNode).getElementsByTagName("name");
            String name = "Unknown";
            if (nameElement.getLength() >= 1)
               name = ((Element)nameElement.item(0)).getTextContent().trim();

            NodeList coordinatesElement = ((Element)polygon.item(0)).getElementsByTagName("coordinates");
            String coodinates = "";
            if (coordinatesElement.getLength() >= 1)
               coodinates = ((Element)coordinatesElement.item(0)).getTextContent();

            List<GeoLocation> border = new ArrayList<GeoLocation>();

            String[] lines = coodinates.split("\\r?\\n");
            for(String line : lines)
            {
               String[] items = line.split(",");
               if (items.length >= 2)
               {
                  try
                  {
                     border.add(new GeoLocation(Double.parseDouble(items[1]), Double.parseDouble(items[0])));
                  }
                  catch(NumberFormatException e)
                  {
                     logger.debug("Error parsing polygon vertex in KML file");
                  }
               }
            }

            if (border.size() > 0)
               map.put(name, border);
         }
      }
      return map;
   }
}
