/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.search;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

/**
 * Search query.
 * 
 * Query can be constructed using the following elements:
 * <p>
 * word given word should be present in object's text
 * <p>
 * not word given word should not be present in object's text
 * <p>
 * "quoted text" words in quotes should be present in object's text
 * <p>
 * not "quoted text" words in quotes should not be present in object's text
 * <p>
 * attribute:value1,value2 object should have given attribute, and attribute value should be one of those listed
 * <p>
 * -attribute:value1,value2 object should not have given attribute, or attribute value should not be one of those listed
 *
 */
public class SearchQuery
{
   private Set<String> includedTexts = null;
   private Set<String> excludedTexts = null;
   private Map<String, Set<String>> includedAttributes = null;
   private Map<String, Set<String>> excludedAttributes = null;

   /**
    * Construct new search query from source string
    * 
    * @param source query source
    */
   public SearchQuery(String source)
   {
      char[] input = source.toCharArray();
      int position = 0, start = -1;
      int state = 0;
      boolean negateNext = false;
      Set<String> attributeValues = null;
      while(position <= input.length)
      {
         int ch = (position < input.length) ? input[position] : ' ';
         switch(state)
         {
            case 0: // whitespace
               if (ch == '"')
               {
                  state = 3;
                  start = position + 1;
               }
               else if (!Character.isWhitespace(ch))
               {
                  state = 1;
                  start = position;
               }
               break;
            case 1: // text started
               if (Character.isWhitespace(ch))
               {
                  String text = new String(input, start, position - start);
                  if (text.equalsIgnoreCase("not"))
                  {
                     negateNext = true;
                  }
                  else
                  {
                     if (negateNext)
                     {
                        if (excludedTexts == null)
                           excludedTexts = new HashSet<String>();
                        excludedTexts.add(text.toLowerCase());
                        negateNext = false;
                     }
                     else
                     {
                        if (includedTexts == null)
                           includedTexts = new HashSet<String>();
                        includedTexts.add(text.toLowerCase());
                     }
                  }
                  state = 0;
               }
               else if (ch == ':') // Start of attribute
               {
                  Map<String, Set<String>> attributes;
                  if ((input[start] == '-') || negateNext)
                  {
                     if (excludedAttributes == null)
                        excludedAttributes = new HashMap<String, Set<String>>();
                     attributes = excludedAttributes;
                     if (input[start] == '-')
                        start++;
                     negateNext = false;
                  }
                  else
                  {
                     if (includedAttributes == null)
                        includedAttributes = new HashMap<String, Set<String>>();
                     attributes = includedAttributes;
                  }

                  String name = new String(input, start, position - start).toLowerCase();
                  attributeValues = attributes.get(name);
                  if (attributeValues == null)
                  {
                     attributeValues = new HashSet<String>();
                     attributes.put(name, attributeValues);
                  }

                  start = position + 1;
                  state = 2;
               }
               break;
            case 2: // attribute values
               if (Character.isWhitespace(ch) || (ch == ','))
               {
                  String v = new String(input, start, position - start).toLowerCase();
                  if (!v.isEmpty())
                     attributeValues.add(v);
                  if (ch == ',')
                  {
                     start = position + 1;
                  }
                  else
                  {
                     state = 0; // Whitespace, end of current element
                  }
               }
               break;
            case 3: // quoted text
               if (ch == '"')
               {
                  if (negateNext)
                  {
                     if (excludedTexts == null)
                        excludedTexts = new HashSet<String>();
                     excludedTexts.add(new String(input, start, position - start).toLowerCase());
                     negateNext = false;
                  }
                  else
                  {
                     if (includedTexts == null)
                        includedTexts = new HashSet<String>();
                     includedTexts.add(new String(input, start, position - start).toLowerCase());
                  }
                  state = 0;
               }
               break;
         }
         position++;
      }
   }

   /**
    * Match given object against this query. If object implements <code>SearchAttributeProvider</code> interface then text for
    * matching will be retrieved from <code>getText</code> method, and attribute matching will take place. Otherwise, text will be
    * retrieved from <code>toString</code> method.
    * 
    * @param object object to test
    * @return true if this search query matches given object
    */
   public boolean match(Object object)
   {
      String text = (object instanceof SearchAttributeProvider) ? ((SearchAttributeProvider)object).getText().toLowerCase() : object.toString().toLowerCase();
      if (excludedTexts != null)
      {
         for(String s : excludedTexts)
            if (text.contains(s))
               return false;
      }
      if (includedTexts != null)
      {
         for(String s : includedTexts)
            if (!text.contains(s))
               return false;
      }

      if (object instanceof SearchAttributeProvider)
      {
         if (excludedAttributes != null)
         {
            for(Entry<String, Set<String>> e : excludedAttributes.entrySet())
            {
               String value = ((SearchAttributeProvider)object).getAttribute(e.getKey());
               if ((value != null) && e.getValue().contains(value.toLowerCase()))
                  return false;
            }
         }
         if (includedAttributes != null)
         {
            for(Entry<String, Set<String>> e : includedAttributes.entrySet())
            {
               String value = ((SearchAttributeProvider)object).getAttribute(e.getKey());
               if ((value == null) || !e.getValue().contains(value.toLowerCase()))
                  return false;
            }
         }
      }

      return true;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "SearchQuery [includedTexts=" + includedTexts + ", excludedTexts=" + excludedTexts + ", includedAttributes=" + includedAttributes + ", excludedAttributes=" + excludedAttributes + "]";
   }
}
