/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.nxmc.base.widgets.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;

/**
 * Style range helper
 */
public class StyleRange
{
   public int start;
   public int length;
   public Color foreground;
   public Color background;
   public int fontStyle = SWT.NORMAL;
   public boolean underline = false;
   public Color underlineColor = null;
   public int underlineStyle = 0;
   public boolean strikeout = false;
   public Color strikeoutColor = null;
   public boolean hidden = false;

   /**
    * New style range
    */
   public StyleRange()
   {
      this.start = 0;
      this.length = 0;
      this.foreground = null;
      this.background = null;
   }
   
   /**
    * Copy constructor
    * 
    * @param src
    */
   public StyleRange(StyleRange src)
   {
      this.start = src.start;
      this.length = src.length;
      this.foreground = src.foreground;
      this.background = src.background;
      this.fontStyle = src.fontStyle;
      this.underline = src.underline;
      this.underlineColor = src.underlineColor;
      this.underlineStyle = src.underlineStyle;
      this.strikeout = src.strikeout;
      this.strikeoutColor = src.strikeoutColor;
      this.hidden = src.hidden;
   }

   /** 
    * Create a new style range.
    *
    * @param start start offset of the style
    * @param length length of the style 
    * @param foreground foreground color of the style, null if none 
    * @param background background color of the style, null if none
    */
   public StyleRange(int start, int length, Color foreground, Color background) 
   {
      this.start = start;
      this.length = length;
      this.foreground = foreground;
      this.background = background;
   }

   /** 
    * Create a new style range.
    *
    * @param start start offset of the style
    * @param length length of the style 
    * @param foreground foreground color of the style, null if none 
    * @param background background color of the style, null if none
    * @param fontStyle font style of the style, may be SWT.NORMAL, SWT.ITALIC or SWT.BOLD
    */
   public StyleRange(int start, int length, Color foreground, Color background, int fontStyle) 
   {
      this(start, length, foreground, background);
      this.fontStyle = fontStyle;
   }

   /**
    * @see java.lang.Object#clone()
    */
   @Override
   public Object clone() throws CloneNotSupportedException
   {
      StyleRange r = new StyleRange(start, length, foreground, background, fontStyle);
      r.underline = underline;
      r.underlineColor = underlineColor;
      r.underlineStyle = underlineStyle;
      r.strikeout = strikeout;
      r.strikeoutColor = strikeoutColor;
      return r;
   }
}
