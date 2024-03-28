/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.filemanager.widgets;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.base.widgets.helpers.LineStyler;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Base file viewer widget
 */
public class BaseFileViewer extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(BaseFileViewer.class);
   private final I18n i18n = LocalizationHelper.getI18n(BaseFileViewer.class);

   public static final int INFORMATION = 0;
   public static final int WARNING = 1;
   public static final int ERROR = 2;

   public static final long MAX_FILE_SIZE = 67108864; // 64MB

   protected View view;
   protected StyledText text;
   protected Composite searchBar;
   protected Text searchBarText;
   protected Label searchCloseButton;
   protected boolean scrollLock = false;
   private int lineCountLimit = 0;
   protected StringBuilder content = new StringBuilder();
   protected Pattern appendFilter = null;
   protected String lineRemainder = null;

   /**
    * Create file viewer
    * 
    * @param parent
    * @param style
    */
   public BaseFileViewer(Composite parent, int style, View view)
   {
      super(parent, style);
      this.view = view;

      setLayout(new FormLayout());

      /*** Text area ***/
      text = new StyledText(this, SWT.H_SCROLL | SWT.V_SCROLL);
      text.setFont(JFaceResources.getTextFont());
      FormData fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      text.setLayoutData(fd);

      /*** Search bar ***/
      searchBar = new Composite(this, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 3;
      layout.marginBottom = 3;
      layout.numColumns = 3;
      searchBar.setLayout(layout);
      searchBar.setVisible(false);

      Label separator = new Label(searchBar, SWT.SEPARATOR | SWT.HORIZONTAL);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      separator.setLayoutData(gd);

      Label searchBarLabel = new Label(searchBar, SWT.LEFT);
      searchBarLabel.setText(i18n.tr("Find:"));
      searchBarLabel.setBackground(searchBar.getBackground());
      searchBarLabel.setForeground(searchBar.getForeground());
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalIndent = 5;
      searchBarLabel.setLayoutData(gd);

      Composite searchBarTextContainer = new Composite(searchBar, SWT.BORDER);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 3;
      layout.horizontalSpacing = 0;
      searchBarTextContainer.setLayout(layout);
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.LEFT;
      gd.widthHint = 400;
      searchBarTextContainer.setLayoutData(gd);

      searchBarText = new Text(searchBarTextContainer, SWT.NONE);
      searchBarText.setMessage(i18n.tr("Find in file"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.CENTER;
      gd.grabExcessHorizontalSpace = true;
      searchBarText.setLayoutData(gd);
      searchBarText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            doSearch(true);
         }
      });
      searchBarText.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            doSearch(false);
         }
      });

      searchBarTextContainer.setBackground(searchBarText.getBackground());

      ToolBar searchButtons = new ToolBar(searchBarTextContainer, SWT.FLAT);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalAlignment = SWT.LEFT;
      searchButtons.setLayoutData(gd);

      ToolItem item = new ToolItem(searchButtons, SWT.PUSH);
      item.setImage(SharedIcons.IMG_UP);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            doReverseSearch();
         }
      });

      item = new ToolItem(searchButtons, SWT.PUSH);
      item.setImage(SharedIcons.IMG_DOWN);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            doSearch(false);
         }
      });

      searchCloseButton = new Label(searchBar, SWT.NONE);
      searchCloseButton.setBackground(searchBar.getBackground());
      searchCloseButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      searchCloseButton.setImage(SharedIcons.IMG_CLOSE);
      searchCloseButton.setToolTipText(i18n.tr("Close"));
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 20;
      searchCloseButton.setLayoutData(gd);
      searchCloseButton.addMouseListener(new MouseListener() {
         private boolean doAction = false;

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               doAction = false;
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               doAction = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && doAction)
               hideSearchBar();
         }
      });

      fd = new FormData();
      fd.bottom = new FormAttachment(100, 0);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      searchBar.setLayoutData(fd);
   }

   /**
    * Show local file in viewer
    *
    * @param file file to show
    * @param scrollToEnd if true, scroll to end of file
    */
   public void showFile(final File file, final boolean scrollToEnd)
   {
      Job job = new Job(i18n.tr("Load file into viewer"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String content = loadFile(file);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  boolean scrollOnAppend = text.isScrollOnAppend();
                  if (scrollToEnd)
                     text.setScrollOnAppend(true);
                  setContent(content);
                  if (scrollToEnd && !scrollOnAppend)
                     text.setScrollOnAppend(false);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot load file %s"), file.getAbsolutePath());
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Show search bar
    */
   public void showSearchBar()
   {
      searchBarText.setText(""); //$NON-NLS-1$
      searchBar.setVisible(true);
      ((FormData)text.getLayoutData()).bottom = new FormAttachment(searchBar, 0, SWT.TOP);
      layout(true, true);
      searchBarText.setFocus();
   }

   /**
    * Hide message bar
    */
   public void hideSearchBar()
   {
      searchBar.setVisible(false);
      ((FormData)text.getLayoutData()).bottom = new FormAttachment(100, 0);
      layout(true, true);
   }

   /**
    * Clear viewer
    */
   public void clear()
   {
      content = new StringBuilder();
      text.setText(""); //$NON-NLS-1$
   }
   
   /**
    * Select all
    */
   public void selectAll()
   {
   }

   /**
    * Copy selection to clipboard
    */
   public void copy()
   {
   }

   /**
    * Check if copy can be performed
    * 
    * @return
    */
   public boolean canCopy()
   {
      return false;
   }

   /**
    * @return the scrollLock
    */
   public boolean isScrollLock()
   {
      return scrollLock;
   }

   /**
    * @param scrollLock the scrollLock to set
    */
   public void setScrollLock(boolean scrollLock)
   {
      this.scrollLock = scrollLock;
      text.setScrollOnAppend(!scrollLock);
   }

   /**
    * Get limit on number of lines to keep in viewer.
    *
    * @return limit on number of lines to keep in viewer (0 means "unlimited")
    */
   public int getLineCountLimit()
   {
      return lineCountLimit;
   }

   /**
    * Set limit on number of lines to keep in viewer (0 for unlimited).
    *
    * @param lineCountLimit new limit
    */
   public void setLineCountLimit(int lineCountLimit)
   {
      this.lineCountLimit = lineCountLimit;
   }

   /**
    * Set append filter.
    *
    * @param regex regular expression that define matching lines
    */
   public void setAppendFilter(String regex)
   {
      if ((regex != null) && !regex.isEmpty())
      {
         try
         {
            appendFilter = Pattern.compile(regex, Pattern.CASE_INSENSITIVE);
         }
         catch(PatternSyntaxException e)
         {
            logger.warn("Syntax error in file viewer append filter (regex=\"" + regex + "\")", e);
            appendFilter = null;
         }
      }
      else
      {
         appendFilter = null;
      }
   }

   /**
    * @return
    */
   public Control getTextControl()
   {
      return text;
   }

   /**
    * Add text selection listener
    * 
    * @param listener
    */
   public void addSelectionListener(SelectionListener listener)
   {
   }

   /**
    * Remove selection listener
    * 
    * @param listener
    */
   public void removeSelectionListener(SelectionListener listener)
   {
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      super.setBackground(color);
      text.setBackground(color);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setForeground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setForeground(Color color)
   {
      super.setForeground(color);
      text.setForeground(color);
   }

   /**
    * Get viewer content.
    *
    * @return viewer content
    */
   public String getContent()
   {
      return text.getText();
   }

   /**
    * Replace content (including all styling) with one from given source file viewer
    *
    * @param source source file viewer
    * @param scrollToEnd true to scroll new content to end of text
    */
   public void replaceContent(BaseFileViewer source, boolean scrollToEnd)
   {
      text.replaceContent(source.text);
      if (scrollToEnd)
         setTextTopIndex();
   }

   /**
    * Set content of a viewer.
    *
    * @param s new content as a string
    */
   public void setContent(String s)
   {
      lineRemainder = null;
      String ps = filterText(removeEscapeSequences(s));
      text.setText(ps);
      content = new StringBuilder();
      content.append(ps.toLowerCase());
      if (lineCountLimit > 0)
      {
         int lineCount = text.getLineCount();
         if ((lineCountLimit > 0) && (lineCount > lineCountLimit))
         {
            int length = text.getOffsetAtLine(lineCount - lineCountLimit);
            text.replaceTextRange(0, length, "");
            content.replace(0, length, "");
         }
      }
   }

   /**
    * Append to existing content.
    *
    * @param s addition for viewer content as a string
    */
   protected void append(String s)
   {
      String ps = filterText(removeEscapeSequences(s));
      content.append(ps.toLowerCase());
      text.append(ps);

      if (lineCountLimit > 0)
      {
         int lineCount = text.getLineCount();
         if ((lineCountLimit > 0) && (lineCount > lineCountLimit))
         {
            int length = text.getOffsetAtLine(lineCount - lineCountLimit);
            text.replaceTextRange(0, length, "");
            content.replace(0, length, "");
         }
      }
   }

   /**
    * Filter text using append filter
    * 
    * @param text text to filter
    * @return filtered text
    */
   private String filterText(String text)
   {
      if (appendFilter == null)
         return text;

      StringBuilder sb = new StringBuilder();
      if (lineRemainder != null)
      {
         sb.append(lineRemainder);
         lineRemainder = null;
      }
      sb.append(text);

      StringBuilder output = new StringBuilder();
      int offset = 0;
      while(offset < sb.length())
      {
         int nextOffset = sb.indexOf("\n", offset);
         if (nextOffset == -1)
         {
            lineRemainder = sb.substring(offset);
            break;
         }
         String line = sb.substring(offset, nextOffset + 1);
         if (appendFilter.matcher(line).find())
            output.append(line);
         offset = nextOffset + 1;
      }
      return output.toString();
   }

   /**
    * @return the lineStyler
    */
   public LineStyler getLineStyler()
   {
      return text.getLineStyler();
   }

   /**
    * @param lineStyler the lineStyler to set
    */
   public void setLineStyler(LineStyler lineStyler)
   {
      text.setLineStyler(lineStyler);
   }

   /**
    * Do search
    */
   private void doSearch(boolean typing)
   {
   }
   
   /**
    * Do search backwards
    */
   private void doReverseSearch()
   {
   }
   
   /**
    * Remove escape sequences from input string
    * 
    * @param s
    * @return
    */
   protected static String removeEscapeSequences(String s)
   {
      // Convert to correct new line symbol
      s = s.replaceAll("\r(?!\n)", "\n");

      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < s.length(); i++)
      {
         char ch = s.charAt(i);
         if (ch == 27)
         {
            i++;
            ch = s.charAt(i);
            if (ch == '[')
            {
               for(; i < s.length(); i++)
               {
                  ch = s.charAt(i);
                  if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')))
                     break;
               }
            }
            else if ((ch == '(') || (ch == ')'))
            {
               i++;
            }
         }
         else if ((ch >= 32) || (ch == '\r') || (ch == '\n') || (ch == '\t'))
         {
            sb.append(ch);
         }
      }
      return sb.toString();
   }

   /**
    * Load file content into string
    * 
    * @param file
    * @return
    */
   protected static String loadFile(final File file)
   {
      StringBuilder content = new StringBuilder();
      FileReader reader = null;
      char[] buffer = new char[32768];
      try
      {
         reader = new FileReader(file);
         int size = 0;
         while(size < MAX_FILE_SIZE)
         {
            int count = reader.read(buffer);
            if (count == -1)
               break;
            if (count == buffer.length)
            {
               content.append(buffer);
            }
            else
            {
               content.append(Arrays.copyOf(buffer, count));
            }
            size += count;
         }
      }
      catch(IOException e)
      {
         logger.error("Cannot load file content to string", e);
      }
      finally
      {
         if (reader != null)
         {
            try
            {
               reader.close();
            }
            catch(IOException e)
            {
            }
         }
      }
      return content.toString();
   }

   /**
    * Set text top index (compatibility layer for RAP)
    */
   protected void setTextTopIndex()
   {      
   }   

   /**
    * Set scroll behavior on append (compatibility layer for RAP)
    */
   protected void setScrollOnAppend(boolean scrollLock)
   {
      text.setScrollOnAppend(!scrollLock);
   }
}
