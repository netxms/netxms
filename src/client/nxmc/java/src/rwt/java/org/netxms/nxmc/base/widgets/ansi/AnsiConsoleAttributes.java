/**
 * Copyright 2012-2014 Mihai Nita
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.netxms.nxmc.base.widgets.ansi;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.base.widgets.helpers.StyleRange;
import org.netxms.nxmc.tools.ColorCache;

/**
 * ANSI console attributes
 */
public class AnsiConsoleAttributes implements Cloneable
{
   public final static int UNDERLINE_NONE = -1; // nothing in SWT, a bit of an abuse
   
   public Integer currentBgColor;
   public Integer currentFgColor;
   public int underline;
   public boolean bold;
   public boolean italic;
   public boolean invert;
   public boolean conceal;
   public boolean strike;
   public boolean framed;

   private ColorCache colorCache;
   private Control control; // owning control

   /**
    * Create new attribute object.
    *
    * @param control owning control
    */
   public AnsiConsoleAttributes(Control control)
   {
      this.control = control;
      colorCache = new ColorCache(control);
      reset();
   }

   /**
    * Reset state
    */
   public void reset()
   {
      currentBgColor = null;
      currentFgColor = null;
      underline = UNDERLINE_NONE;
      bold = false;
      italic = false;
      invert = false;
      conceal = false;
      strike = false;
      framed = false;
   }

   /**
    * @see java.lang.Object#clone()
    */
   @Override
   public AnsiConsoleAttributes clone()
   {
      AnsiConsoleAttributes result = new AnsiConsoleAttributes(control);
      result.currentBgColor = currentBgColor;
      result.currentFgColor = currentFgColor;
      result.underline = underline;
      result.bold = bold;
      result.italic = italic;
      result.invert = invert;
      result.conceal = conceal;
      result.strike = strike;
      result.framed = framed;
      return result;
   }

   /**
    * This function maps from the current attributes as "described" by escape sequences to real,
    * Eclipse console specific attributes (resolving color palette, default colors, etc.)
    * 
    * @param range
    * @param attribute
    */
   public void updateRangeStyle(StyleRange range)
   {
      if (currentFgColor != null)
         range.foreground = colorCache.create(AnsiConsoleColorPalette.getColor(currentFgColor));

      // Prepare the background color
      if (currentBgColor != null)
         range.background = colorCache.create(AnsiConsoleColorPalette.getColor(currentBgColor));

      // These two still mess with the foreground/background colors
      // We need to solve them before we use them for strike/underline/frame colors
      if (invert)
      {
         if (range.foreground == null)
            range.foreground = colorCache.create(control.getForeground().getRGB());
         if (range.background == null)
            range.background = colorCache.create(control.getBackground().getRGB());
         Color tmp = range.background;
         range.background = range.foreground;
         range.foreground = tmp;
      }

      if (conceal)
      {
         if (range.background == null)
            range.background = colorCache.create(control.getBackground().getRGB());
         range.foreground = range.background;
      }

      range.fontStyle = SWT.NORMAL;
      // Prepare the rest of the attributes
      if (bold)
         range.fontStyle |= SWT.BOLD;

      if (italic)
         range.fontStyle |= SWT.ITALIC;

      if (underline != UNDERLINE_NONE)
      {
         range.underline = true;
         range.underlineColor = range.foreground;
         range.underlineStyle = underline;
      }
      else
      {
         range.underline = false;
      }

      range.strikeout = strike;
      range.strikeoutColor = range.foreground;
   }
}
