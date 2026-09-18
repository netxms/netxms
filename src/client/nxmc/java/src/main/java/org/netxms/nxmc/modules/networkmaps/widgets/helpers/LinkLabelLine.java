/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.maps.LinkDataDirection;

/**
 * One line of a network map link label: sequence of text segments, each optionally marked with data direction (drawn as an
 * arrow before the text).
 */
public final class LinkLabelLine
{
   public static final String SEGMENT_SEPARATOR = "  ";

   /**
    * Line segment
    */
   public static final class Segment
   {
      public final LinkDataDirection direction;
      public final String text;

      private Segment(LinkDataDirection direction, String text)
      {
         this.direction = direction;
         this.text = text;
      }
   }

   private final List<Segment> segments = new ArrayList<>();

   /**
    * Create empty line
    */
   public LinkLabelLine()
   {
   }

   /**
    * Create line with single segment without direction
    *
    * @param text segment text
    */
   public LinkLabelLine(String text)
   {
      add(LinkDataDirection.NONE, text);
   }

   /**
    * Add segment
    *
    * @param direction data direction
    * @param text segment text
    */
   public void add(LinkDataDirection direction, String text)
   {
      segments.add(new Segment(direction, text));
   }

   /**
    * @return line segments
    */
   public List<Segment> getSegments()
   {
      return segments;
   }

   /**
    * Get line text without direction marks
    *
    * @return line text
    */
   public String getText()
   {
      StringBuilder sb = new StringBuilder();
      for(Segment s : segments)
      {
         if (sb.length() > 0)
            sb.append(SEGMENT_SEPARATOR);
         sb.append(s.text);
      }
      return sb.toString();
   }

   /**
    * Get text of given lines without direction marks
    *
    * @param lines label lines
    * @return text with one row per line
    */
   public static String join(List<LinkLabelLine> lines)
   {
      StringBuilder sb = new StringBuilder();
      for(LinkLabelLine line : lines)
      {
         if (sb.length() > 0)
            sb.append('\n');
         sb.append(line.getText());
      }
      return sb.toString();
   }
}
