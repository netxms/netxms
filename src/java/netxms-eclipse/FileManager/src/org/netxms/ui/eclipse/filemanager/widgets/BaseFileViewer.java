/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.filemanager.widgets;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.LineStyleEvent;
import org.eclipse.swt.custom.LineStyleListener;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.IViewPart;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.console.resources.ThemeEngine;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;

/**
 * Base file viewer widget
 */
public class BaseFileViewer extends Composite
{
   public static final int INFORMATION = 0;
   public static final int WARNING = 1;
   public static final int ERROR = 2;

   public static final long MAX_FILE_SIZE = 67108864; // 64MB

   protected IViewPart viewPart;
   protected StyledText text;
   protected Composite messageBar;
   protected CLabel messageBarLabel;
   protected Label messageCloseButton;
   protected Composite searchBar;
   protected Text searchBarText;
   protected Label searchCloseButton;
   protected boolean scrollLock = false;
   private int lineCountLimit = 0;
   protected StringBuilder content = new StringBuilder();
   protected LineStyler lineStyler = null;
   protected Pattern appendFilter = null;
   protected String lineRemainder = null;

   /**
    * Create file viewer
    * 
    * @param parent
    * @param style
    */
   public BaseFileViewer(Composite parent, int style, IViewPart viewPart)
   {
      super(parent, style);
      this.viewPart = viewPart;

      setLayout(new FormLayout());
      
      /*** Message bar ***/
      messageBar = new Composite(this, SWT.NONE);
      messageBar.setBackground(ThemeEngine.getBackgroundColor("MessageBar"));
      messageBar.setForeground(ThemeEngine.getForegroundColor("MessageBar"));
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = 2;
      messageBar.setLayout(layout);
      messageBar.setVisible(false);
      
      messageBarLabel = new CLabel(messageBar, SWT.NONE);
      messageBarLabel.setBackground(messageBar.getBackground());
      messageBarLabel.setForeground(messageBar.getForeground());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      messageBarLabel.setLayoutData(gd);
      
      messageCloseButton = new Label(messageBar, SWT.NONE);
      messageCloseButton.setBackground(messageBar.getBackground());
      messageCloseButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      messageCloseButton.setImage(SharedIcons.IMG_CLOSE);
      messageCloseButton.setToolTipText(Messages.get().BaseFileViewer_HideMessage);
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      messageCloseButton.setLayoutData(gd);
      messageCloseButton.addMouseListener(new MouseListener() {
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
               hideMessage();
         }
      });
      
      Label separator = new Label(messageBar, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      separator.setLayoutData(gd);
      
      FormData fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      messageBar.setLayoutData(fd);

      /*** Text area ***/
      text = new StyledText(this, SWT.H_SCROLL | SWT.V_SCROLL);
      text.setEditable(false);
      text.setFont(JFaceResources.getTextFont());
      fd = new FormData();
      fd.top = new FormAttachment(0, 0);
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      text.setLayoutData(fd);

      text.addLineStyleListener(new LineStyleListener() {
         @Override
         public void lineGetStyle(LineStyleEvent event)
         {
            try
            {
               event.styles = styleLine(event.lineText);
               if (event.styles != null)
               {
                  for(StyleRange r : event.styles)
                     r.start += event.lineOffset;
               }
            }
            catch(Exception e)
            {
               Activator.logError("Exception in line style listener", e);
            }
         }
      });

      /*** Search bar ***/
      searchBar = new Composite(this, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 3;
      layout.marginBottom = 3;
      layout.numColumns = 3;
      searchBar.setLayout(layout);
      searchBar.setVisible(false);

      separator = new Label(searchBar, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      separator.setLayoutData(gd);

      Label searchBarLabel = new Label(searchBar, SWT.LEFT);
      searchBarLabel.setText(Messages.get().BaseFileViewer_Find);
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
      searchBarText.setMessage(Messages.get().BaseFileViewer_FindInFile);
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
      searchCloseButton.setToolTipText(Messages.get().BaseFileViewer_Close);
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

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (lineStyler != null)
               lineStyler.dispose();
         }
      });
   }

   /**
    * Show local file in viewer
    *
    * @param file file to show
    * @param scrollToEnd if true, scroll to end of file
    */
   public void showFile(final File file, final boolean scrollToEnd)
   {
      ConsoleJob job = new ConsoleJob(Messages.get().BaseFileViewer_LoadJobName, viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final String content = loadFile(file);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  setContent(content);
                  if (scrollToEnd)
                     text.setTopIndex(text.getLineCount() - 1);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().BaseFileViewer_LoadJobError, file.getAbsolutePath());
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Show message in message bar
    * 
    * @param severity
    * @param text
    */
   public void showMessage(int severity, String messageText)
   {
      switch(severity)
      {
         case WARNING:
            messageBarLabel.setImage(StatusDisplayInfo.getStatusImage(Severity.WARNING));
            break;
         case ERROR:
            messageBarLabel.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
            break;
         default:
            messageBarLabel.setImage(SharedIcons.IMG_INFORMATION);
            break;
      }
      messageBarLabel.setText(messageText);
      messageBar.setVisible(true);
      ((FormData)text.getLayoutData()).top = new FormAttachment(messageBar, 0, SWT.BOTTOM);
      layout(true, true);
   }
   
   /**
    * Hide message bar
    */
   public void hideMessage()
   {
      messageBar.setVisible(false);
      ((FormData)text.getLayoutData()).top = new FormAttachment(0, 0);
      layout(true, true);
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
      text.selectAll();
      text.notifyListeners(SWT.Selection, new Event());
   }

   /**
    * Copy selection to clipboard
    */
   public void copy()
   {
      text.copy();
   }

   /**
    * Check if copy can be performed
    * 
    * @return
    */
   public boolean canCopy()
   {
      return text.getSelectionCount() > 0;
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
            Activator.logError("Syntax error in file viewer append filter (regex=\"" + regex + "\")", e);
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
      text.addSelectionListener(listener);
   }

   /**
    * Remove selection listener
    * 
    * @param listener
    */
   public void removeSelectionListener(SelectionListener listener)
   {
      text.removeSelectionListener(listener);
   }

   /**
    * @param s
    */
   protected void setContent(String s)
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
    * @param s
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

      if (!scrollLock)
      {
         text.setTopIndex(text.getLineCount() - 1);
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
    * Style line. Default implementation calls registered line styler if any.
    * 
    * @param line line text
    * @return array of style ranges or null
    */
   protected StyleRange[] styleLine(String line)
   {
      return (lineStyler != null) ? lineStyler.styleLine(line) : null;
   }

   /**
    * @return the lineStyler
    */
   public LineStyler getLineStyler()
   {
      return lineStyler;
   }

   /**
    * @param lineStyler the lineStyler to set
    */
   public void setLineStyler(LineStyler lineStyler)
   {
      if (this.lineStyler == lineStyler)
         return;

      if (this.lineStyler != null)
         this.lineStyler.dispose();

      this.lineStyler = lineStyler;
   }

   /**
    * Do search
    */
   private void doSearch(boolean typing)
   {
      String searchString = searchBarText.getText().toLowerCase();
      if (searchString.length() == 0)
         return;
      
      int searchPosition = text.getCaretOffset();
      if (typing && (text.getSelectionCount() > 0))
      {
         Point p = text.getSelectionRange();
         searchPosition = p.x;
      }
      
      if (content.length() - searchPosition < searchString.length())
         return;
      
      int index = content.indexOf(searchString, searchPosition);
      if (index > -1)
      {
         text.setSelection(index, index + searchString.length());
         searchBarText.setBackground(null);
      }
      else
      {
         searchBarText.setBackground(ThemeEngine.getBackgroundColor("TextInput.Error"));
      }
   }
   
   /**
    * Do search backwards
    */
   private void doReverseSearch()
   {
      String searchString = searchBarText.getText().toLowerCase();
      if (searchString.length() == 0)
         return;
      
      int searchPosition = text.getCaretOffset();
      if ((text.getSelectionCount() > 0) && text.getSelectionText().toLowerCase().equals(searchString))
      {
         Point p = text.getSelectionRange();
         searchPosition = p.x - 1;
      }

      if (searchPosition < searchString.length())
         return;
      
      int index = content.lastIndexOf(searchString, searchPosition);
      if (index > -1)
      {
         text.setSelection(index, index + searchString.length());
         searchBarText.setBackground(null);
      }
      else
      {
         searchBarText.setBackground(ThemeEngine.getBackgroundColor("TextInput.Error"));
      }
   }
   
   /**
    * Remove escape sequences from input string
    * 
    * @param s
    * @return
    */
   protected static String removeEscapeSequences(String s)
   {
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
         e.printStackTrace();
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
    * Line styler interface
    */
   public interface LineStyler
   {
      /**
       * Style given line.
       *
       * @param line text line
       * @return styling for the line
       */
      public StyleRange[] styleLine(String line);

      /**
       * Called by file viewer when line styler is no longer needed. Implementing classes can use this method to dispose resources.
       */
      public void dispose();
   }
}
