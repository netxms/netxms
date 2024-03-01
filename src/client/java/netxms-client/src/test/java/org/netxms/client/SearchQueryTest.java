/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertTrue;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertFalse;
import org.netxms.client.search.SearchAttributeProvider;
import org.netxms.client.search.SearchQuery;

/**
 * Tests for class <code>SearchQuery</code>.
 */
public class SearchQueryTest
{
   @Test
   public void testTextMatch() throws Exception
   {
      SearchQuery q = new SearchQuery("Hello world");
      System.out.println(q);
      assertTrue(q.match("Hello, world!"));
      assertTrue(q.match("Say hello to the world of wonders"));
      assertFalse(q.match("Hi world!"));

      q = new SearchQuery("Hello world not wonders");
      System.out.println(q);
      assertTrue(q.match("Hello, world!"));
      assertFalse(q.match("Say hello to the world of wonders"));
      assertFalse(q.match("Hi world!"));

      q = new SearchQuery("\"Hello world\"");
      System.out.println(q);
      assertFalse(q.match("Hello, world!"));
      assertFalse(q.match("Say hello to the world of wonders"));
      assertTrue(q.match("Hello world!"));
   }

   @Test
   public void testAttributeMatch() throws Exception
   {
      SearchQuery q = new SearchQuery("heavy vendor:CAT");
      System.out.println(q);
      assertTrue(q.match(new TestObject("FirstCat", "C-200", "CAT", "Heavy duty truck")));
      assertFalse(q.match(new TestObject("Demo-4", "X-35", "ACME", "Heavy duty truck")));

      q = new SearchQuery("heavy -vendor:CAT");
      System.out.println(q);
      assertFalse(q.match(new TestObject("FirstCat", "C-200", "CAT", "Heavy duty truck")));
      assertTrue(q.match(new TestObject("Demo-4", "X-35", "ACME", "Heavy duty truck")));

      q = new SearchQuery("type:C-200,X-35,X-36 -vendor:CAT");
      System.out.println(q);
      assertFalse(q.match(new TestObject("FirstCat", "C-200", "CAT", "Heavy duty truck")));
      assertTrue(q.match(new TestObject("Demo-4", "X-35", "ACME", "Heavy duty truck")));
      assertTrue(q.match(new TestObject("Demo-5", "X-36", "ACME", "Heavy duty truck")));
      assertTrue(q.match(new TestObject("OEMCAT", "C-200", "BFCorp", "Heavy duty truck")));
   }

   private static class TestObject implements SearchAttributeProvider
   {
      private String name;
      private String type;
      private String vendor;
      private String description;

      public TestObject(String name, String type, String vendor, String description)
      {
         this.name = name;
         this.type = type;
         this.vendor = vendor;
         this.description = description;
      }

      @Override
      public String getText()
      {
         return description;
      }

      @Override
      public String getAttribute(String a)
      {
         if (a.equalsIgnoreCase("name"))
            return name;
         if (a.equalsIgnoreCase("type"))
            return type;
         if (a.equalsIgnoreCase("vendor"))
            return vendor;
         return null;
      }
   }
}
