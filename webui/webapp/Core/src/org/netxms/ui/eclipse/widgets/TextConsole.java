/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.widgets.ansi.AnsiCommands;
import org.netxms.ui.eclipse.widgets.ansi.AnsiConsoleAttributes;
import org.netxms.ui.eclipse.widgets.ansi.AnsiConsoleColorPalette;
import org.netxms.ui.eclipse.widgets.helpers.LineStyleEvent;
import org.netxms.ui.eclipse.widgets.helpers.LineStyleListener;
import org.netxms.ui.eclipse.widgets.helpers.StyleRange;

/**
 * Generic text console widget
 */
public class TextConsole extends Composite implements LineStyleListener
{
   private final static Pattern pattern = Pattern.compile("\\x1b\\[[^\\x40-\\x7e]*."); //$NON-NLS-1$
   private final static char ESCAPE_SGR = 'm';
   
   private StyledText console;
   private boolean autoScroll = true;
   private AnsiConsoleAttributes lastAttributes = new AnsiConsoleAttributes();
   private AnsiConsoleAttributes currentAttributes = new AnsiConsoleAttributes();
   private int lastRangeEnd = 0;
   
   /**
    * @param parent
    * @param style
    */
   public TextConsole(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      console = new StyledText(this, SWT.H_SCROLL | SWT.V_SCROLL);
      console.setFont(JFaceResources.getTextFont());
      console.setScrollOnAppend(autoScroll);
      console.addLineStyleListener(this);
   }

   /**
    * Add listener for console selection change
    * 
    * @param listener
    */
   public void addSelectionChangedListener(ISelectionChangedListener listener) 
   {
   }
   
   /**
    * Check if copy operation is allowed
    * 
    * @return
    */
   public boolean canCopy()
   {
      return false;
   }
   
   /**
    * Copy selection to clipboard
    */
   public void copy()
   {
   }
   
   /**
    * Clear console
    */
   public void clear()
   {
      console.setText("");
   }
   
   /**
    * Select all
    */
   public void selectAll()
   {
   }
   
   /**
    * @param autoScroll
    */
   public void setAutoScroll(boolean autoScroll)
   {
      this.autoScroll = autoScroll;
      console.setScrollOnAppend(autoScroll);
   }
   
   /**
    * Create new output stream attached to underlying console.
    * 
    * @return
    */
   public IOConsoleOutputStream newOutputStream()
   {
      return new IOConsoleOutputStream();
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.helpers.LineStyleListener#lineGetStyle(org.netxms.ui.eclipse.widgets.helpers.LineStyleEvent)
    */
   @Override
   public void lineGetStyle(LineStyleEvent event)
   {
      if (event == null || event.lineText == null || event.lineText.length() == 0)
         return;

      StyleRange defStyle;
      if (event.styles != null && event.styles.length > 0)
      {
         defStyle = new StyleRange(event.styles[0]);
         if (defStyle.background == null)
            defStyle.background = AnsiConsoleAttributes.getDefaultBackgroundColor();
      }
      else
      {
         defStyle = new StyleRange(1, lastRangeEnd, new Color(null, AnsiConsoleColorPalette.getColor(0)), new Color(null,
               AnsiConsoleColorPalette.getColor(15)), SWT.NORMAL);
      }

      lastRangeEnd = 0;
      List<StyleRange> ranges = new ArrayList<StyleRange>();
      String currentText = event.lineText;
      Matcher matcher = pattern.matcher(currentText);
      while(matcher.find())
      {
         int start = matcher.start();
         int end = matcher.end();

         String theEscape = currentText.substring(matcher.start() + 2, matcher.end() - 1);
         char code = currentText.charAt(matcher.end() - 1);

         if (code == ESCAPE_SGR)
         {
            // Select Graphic Rendition (SGR) escape sequence
            List<Integer> nCommands = new ArrayList<Integer>();
            for(String cmd : theEscape.split(";")) //$NON-NLS-1$
            {
               int nCmd = tryParseInteger(cmd);
               if (nCmd != -1)
                  nCommands.add(nCmd);
            }
            if (nCommands.isEmpty())
               nCommands.add(0);
            interpretCommand(nCommands);
         }

         if (lastRangeEnd != start)
            addRange(ranges, event.lineOffset + lastRangeEnd, start - lastRangeEnd, defStyle.foreground, false);
         lastAttributes = currentAttributes.clone();

         addRange(ranges, event.lineOffset + start, end - start, defStyle.foreground, true);
      }
      if (lastRangeEnd != currentText.length())
         addRange(ranges, event.lineOffset + lastRangeEnd, currentText.length() - lastRangeEnd, defStyle.foreground, false);
      lastAttributes = currentAttributes.clone();

      if (!ranges.isEmpty())
         event.styles = ranges.toArray(new StyleRange[ranges.size()]);
   }

   /**
    * @param nCommands
    * @return
    */
   private boolean interpretCommand(List<Integer> nCommands)
   {
      boolean result = false;

      Iterator<Integer> iter = nCommands.iterator();
      while(iter.hasNext())
      {
         int nCmd = iter.next();
         switch(nCmd)
         {
            case AnsiCommands.COMMAND_ATTR_RESET:
               currentAttributes.reset();
               break;
               
            case AnsiCommands.COMMAND_ATTR_INTENSITY_BRIGHT:
               currentAttributes.bold = true;
               break;
            case AnsiCommands.COMMAND_ATTR_INTENSITY_FAINT:
               currentAttributes.bold = false;
               break;
            case AnsiCommands.COMMAND_ATTR_INTENSITY_NORMAL:
               currentAttributes.bold = false;
               break;

            case AnsiCommands.COMMAND_ATTR_ITALIC:
               currentAttributes.italic = true;
               break;
            case AnsiCommands.COMMAND_ATTR_ITALIC_OFF:
               currentAttributes.italic = false;
               break;

            case AnsiCommands.COMMAND_ATTR_UNDERLINE:
               currentAttributes.underline = 1;
               break;
            case AnsiCommands.COMMAND_ATTR_UNDERLINE_DOUBLE:
               currentAttributes.underline = 2;
               break;
            case AnsiCommands.COMMAND_ATTR_UNDERLINE_OFF:
               currentAttributes.underline = AnsiConsoleAttributes.UNDERLINE_NONE;
               break;

            case AnsiCommands.COMMAND_ATTR_CROSSOUT_ON:
               currentAttributes.strike = true;
               break;
            case AnsiCommands.COMMAND_ATTR_CROSSOUT_OFF:
               currentAttributes.strike = false;
               break;

            case AnsiCommands.COMMAND_ATTR_NEGATIVE_ON:
               currentAttributes.invert = true;
               break;
            case AnsiCommands.COMMAND_ATTR_NEGATIVE_OFF:
               currentAttributes.invert = false;
               break;

            case AnsiCommands.COMMAND_ATTR_CONCEAL_ON:
               currentAttributes.conceal = true;
               break;
            case AnsiCommands.COMMAND_ATTR_CONCEAL_OFF:
               currentAttributes.conceal = false;
               break;

            case AnsiCommands.COMMAND_ATTR_FRAMED_ON:
               currentAttributes.framed = true;
               break;
            case AnsiCommands.COMMAND_ATTR_FRAMED_OFF:
               currentAttributes.framed = false;
               break;

            case AnsiCommands.COMMAND_COLOR_FOREGROUND_RESET:
               currentAttributes.currentFgColor = null;
               break;
            case AnsiCommands.COMMAND_COLOR_BACKGROUND_RESET:
               currentAttributes.currentBgColor = null;
               break;

            case AnsiCommands.COMMAND_256COLOR_FOREGROUND:
            case AnsiCommands.COMMAND_256COLOR_BACKGROUND: // {esc}[48;5;{color}m
               int nMustBe5 = iter.hasNext() ? iter.next() : -1;
               if (nMustBe5 == 5)
               { // 256 colors
                  int color = iter.hasNext() ? iter.next() : -1;
                  if (color >= 0 && color < 256)
                     if (nCmd == AnsiCommands.COMMAND_256COLOR_FOREGROUND)
                        currentAttributes.currentFgColor = color;
                     else
                        currentAttributes.currentBgColor = color;
               }
               break;

            case -1:
               break; // do nothing

            default:
               if (nCmd >= AnsiCommands.COMMAND_COLOR_FOREGROUND_FIRST && nCmd <= AnsiCommands.COMMAND_COLOR_FOREGROUND_LAST) // text color
                  currentAttributes.currentFgColor = nCmd - AnsiCommands.COMMAND_COLOR_FOREGROUND_FIRST;
               else if (nCmd >= AnsiCommands.COMMAND_COLOR_BACKGROUND_FIRST && nCmd <= AnsiCommands.COMMAND_COLOR_BACKGROUND_LAST) // background color
                  currentAttributes.currentBgColor = nCmd - AnsiCommands.COMMAND_COLOR_BACKGROUND_FIRST;
               else if (nCmd >= AnsiCommands.COMMAND_HICOLOR_FOREGROUND_FIRST && nCmd <= AnsiCommands.COMMAND_HICOLOR_FOREGROUND_LAST) // text color
                  currentAttributes.currentFgColor = nCmd - AnsiCommands.COMMAND_HICOLOR_FOREGROUND_FIRST + AnsiCommands.COMMAND_COLOR_INTENSITY_DELTA;
               else if (nCmd >= AnsiCommands.COMMAND_HICOLOR_BACKGROUND_FIRST && nCmd <= AnsiCommands.COMMAND_HICOLOR_BACKGROUND_LAST) // background color
                  currentAttributes.currentBgColor = nCmd - AnsiCommands.COMMAND_HICOLOR_BACKGROUND_FIRST + AnsiCommands.COMMAND_COLOR_INTENSITY_DELTA;
         }
      }

      return result;
   }

   /**
    * @param ranges
    * @param start
    * @param length
    * @param foreground
    * @param isCode
    */
   private void addRange(List<StyleRange> ranges, int start, int length, Color foreground, boolean isCode)
   {
      StyleRange range = new StyleRange(start, length, foreground, null);
      AnsiConsoleAttributes.updateRangeStyle(range, lastAttributes);
      if (isCode)
      {
         range.hidden = true;
      }
      ranges.add(range);
      lastRangeEnd = lastRangeEnd + range.length;
   }
   
   /**
    * @param text
    * @return
    */
   private static int tryParseInteger(String text)
   {
      if ("".equals(text)) //$NON-NLS-1$
         return -1;

      try
      {
         return Integer.parseInt(text);
      }
      catch(NumberFormatException e)
      {
         return -1;
      }
   }

   /**
    * Console output stream implementation
    */
   public class IOConsoleOutputStream extends OutputStream
   {
      /* (non-Javadoc)
       * @see java.io.OutputStream#write(int)
       */
      @Override
      public void write(int b) throws IOException
      {
         write(new byte[] {(byte)b}, 0, 1);
      }
      
      /* (non-Javadoc)
       * @see java.io.OutputStream#write(byte[])
       */
      @Override
      public void write(byte[] b) throws IOException 
      {
         write(b, 0, b.length);
      }
      
      /* (non-Javadoc)
       * @see java.io.OutputStream#write(byte[], int, int)
       */
      @Override
      public void write(byte[] b, int off, int len) throws IOException 
      {
         write(new String(b, off, len));
      }
      
      /**
       * Write string to console
       * 
       * @param s
       * @throws IOException
       */
      public synchronized void write(final String s) throws IOException 
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               console.append(s);
            }
         });
      }
   }
}
