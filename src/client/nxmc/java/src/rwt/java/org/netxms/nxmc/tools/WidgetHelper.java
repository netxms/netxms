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
package org.netxms.nxmc.tools;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.BreakIterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.scripting.ClientListener;
import org.eclipse.rap.rwt.widgets.WidgetUtil;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.TextTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontMetrics;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Scrollable;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.base.widgets.helpers.MsgProxyWidget;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ThemeEngine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Utility class for simplified creation of widgets
 */
public class WidgetHelper
{
   private static final Logger logger = LoggerFactory.getLogger(WidgetHelper.class);

	public static final int INNER_SPACING = 2;
	public static final int OUTER_SPACING = 4;
	public static final int DIALOG_WIDTH_MARGIN = 10;
	public static final int DIALOG_HEIGHT_MARGIN = 10;
	public static final int DIALOG_SPACING = 5;
	public static final int BUTTON_WIDTH_HINT = 90;
	public static final int WIDE_BUTTON_WIDTH_HINT = 120;
	public static final String DEFAULT_LAYOUT_DATA = "WidgetHelper::default_layout_data"; //$NON-NLS-1$

	private static final Pattern patternOnlyCharNum = Pattern.compile("[a-zA-Z0-9]+");
	private static final Pattern patternAllDotsAtEnd = Pattern.compile("[.]*$");
	private static final Pattern patternCharsAndNumbersAtEnd = Pattern.compile("[a-zA-Z0-9]*$");
	private static final Pattern patternCharsAndNumbersAtStart = Pattern.compile("^[a-zA-Z0-9]*");

   /**
    * Get character(s) to represent new line in text.
    *
    * @return character(s) to represent new line in text
    */
   public static String getNewLineCharacters()
   {
      return SystemUtils.IS_OS_WINDOWS ? "\r\n" : "\n";
   }

	/**
    * Create pair of label and input field, with label above
	 * 
	 * @param parent Parent composite
	 * @param flags Flags for Text creation
	 * @param widthHint Width hint for text control
	 * @param labelText Label's text
	 * @param initialText Initial text for input field (may be null)
	 * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
	 * @return Created Text object
	 */
	public static Text createLabeledText(final Composite parent, int flags, int widthHint, final String labelText,
	                                     final String initialText, Object layoutData)
	{
		Composite group = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = INNER_SPACING;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		group.setLayout(layout);

		if (layoutData != DEFAULT_LAYOUT_DATA)
		{
			group.setLayoutData(layoutData);
		}
		else
		{
			GridData gridData = new GridData();
			gridData.horizontalAlignment = GridData.FILL;
			gridData.grabExcessHorizontalSpace = true;
			group.setLayoutData(gridData);
		}
		
		Label label = new Label(group, SWT.NONE);
		label.setText(labelText);

		Text text = new Text(group, flags);
		if (initialText != null)
			text.setText(initialText);
      text.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		return text;
	}

	/**
    * Create pair of label and StyledText widget, with label above
	 * 
	 * @param parent Parent composite
	 * @param flags Flags for Text creation
	 * @param labelText Label's text
	 * @param initialText Initial text for input field (may be null)
	 * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
	 * @return Created Text object
	 */
	public static StyledText createLabeledStyledText(final Composite parent, int flags, final String labelText,
	                                                 final String initialText, Object layoutData)
	{
		Composite group = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = INNER_SPACING;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		group.setLayout(layout);

		if (layoutData != DEFAULT_LAYOUT_DATA)
		{
			group.setLayoutData(layoutData);
		}
		else
		{
			GridData gridData = new GridData();
			gridData.horizontalAlignment = GridData.FILL;
			gridData.grabExcessHorizontalSpace = true;
			group.setLayoutData(gridData);
		}
		
		Label label = new Label(group, SWT.NONE);
		label.setText(labelText);

		StyledText text = new StyledText(group, flags);
		if (initialText != null)
			text.setText(initialText);
      text.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		
		return text;
	}

	/**
    * Create pair of label and combo box, with label above
	 * 
	 * @param parent Parent composite
	 * @param flags Flags for Text creation
	 * @param labelText Label's text
	 * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
	 * @return Created Combo object
	 */
	public static Combo createLabeledCombo(final Composite parent, int flags, final String labelText, Object layoutData)
	{
      return createLabeledCombo(parent, flags, labelText, layoutData, null);
	}

	/**
    * Create pair of label and combo box, with label above
    * 
    * @param parent Parent composite
    * @param flags Flags for Text creation
    * @param labelText Label's text
    * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
    * @param backgroundColor background color for surrounding composite and label (null for default)
    * @return Created Combo object
    */
   public static Combo createLabeledCombo(final Composite parent, int flags, final String labelText, Object layoutData, Color backgroundColor)
   {
      Composite group = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = INNER_SPACING;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		group.setLayout(layout);

      if (backgroundColor != null)
         group.setBackground(backgroundColor);

		if (layoutData != DEFAULT_LAYOUT_DATA)
		{
			group.setLayoutData(layoutData);
		}
		else
		{
			GridData gridData = new GridData();
			gridData.horizontalAlignment = GridData.FILL;
			gridData.grabExcessHorizontalSpace = true;
			group.setLayoutData(gridData);
		}
		
      Label label = new Label(group, SWT.NONE);
      label.setText(labelText);
      if (backgroundColor != null)
         label.setBackground(backgroundColor);

		Combo combo = new Combo(group, flags);
		GridData gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		combo.setLayoutData(gridData);

		return combo;
	}

	/**
    * Create pair of label and spinner, with label above
	 * 
	 * @param parent Parent composite
	 * @param flags Flags for Text creation
	 * @param labelText Label's text
	 * @param minVal minimal spinner value
	 * @param maxVal maximum spinner value
	 * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
	 * @return Created Spinner object
	 */
	public static Spinner createLabeledSpinner(final Composite parent, int flags, final String labelText, int minVal, int maxVal, Object layoutData)
	{
		Composite group = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = INNER_SPACING;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		group.setLayout(layout);

		if (layoutData != DEFAULT_LAYOUT_DATA)
		{
			group.setLayoutData(layoutData);
		}
		else
		{
			GridData gridData = new GridData();
			gridData.horizontalAlignment = GridData.FILL;
			gridData.grabExcessHorizontalSpace = true;
			group.setLayoutData(gridData);
		}
		
		Label label = new Label(group, SWT.NONE);
		label.setText(labelText);

		Spinner spinner = new Spinner(group, flags);
		GridData gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		spinner.setLayoutData(gridData);
		
		spinner.setMinimum(minVal);
		spinner.setMaximum(maxVal);
		
		return spinner;
	}

	/**
    * Create pair of label and color selector, with label above
	 * 
	 * @param parent Parent composite
	 * @param labelText Label's text
	 * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
	 * @return Created Text object
	 */
	public static ColorSelector createLabeledColorSelector(final Composite parent, final String labelText, Object layoutData)
	{
		Composite group = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = INNER_SPACING;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		group.setLayout(layout);

		if (layoutData != DEFAULT_LAYOUT_DATA)
		{
			group.setLayoutData(layoutData);
		}
		else
		{
			GridData gridData = new GridData();
			gridData.horizontalAlignment = GridData.FILL;
			group.setLayoutData(gridData);
		}
		
		Label label = new Label(group, SWT.NONE);
		label.setText(labelText);

		ColorSelector cs = new ColorSelector(group);
		GridData gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		cs.getButton().setLayoutData(gridData);		

		return cs;
	}

	/**
	 * Create labeled control using factory.
	 * 
	 * @param parent parent composite
	 * @param flags flags for control being created
	 * @param factory control factory
	 * @param labelText Label's text
	 * @param layoutData Layout data for label/input pair. If null, default GridData will be assigned.
	 * @return created control
	 */
	public static Control createLabeledControl(Composite parent, int flags, WidgetFactory factory, String labelText, Object layoutData)
	{
		Composite group = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = INNER_SPACING;
		layout.horizontalSpacing = 0;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		group.setLayout(layout);

		if (layoutData != DEFAULT_LAYOUT_DATA)
		{
			group.setLayoutData(layoutData);
		}
		else
		{
			GridData gridData = new GridData();
			gridData.horizontalAlignment = GridData.FILL;
			gridData.grabExcessHorizontalSpace = true;
			group.setLayoutData(gridData);
		}

		Label label = new Label(group, SWT.NONE);
		label.setText(labelText);

		final Control widget = factory.createControl(group, flags);
      widget.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		return widget;
	}

	/**
	 * Save settings of table viewer columns
	 * 
	 * @param table Table control
	 * @param prefix Prefix for properties
	 */
   public static void saveColumnSettings(Table table, String prefix)
	{
      PreferenceStore settings = PreferenceStore.getInstance();
		TableColumn[] columns = table.getColumns();
		for(int i = 0; i < columns.length; i++)
		{
         Object id = columns[i].getData("ID");
         if ((id == null) || !(id instanceof Integer))
            id = Integer.valueOf(i);
         int width = columns[i].getWidth();
         if (((Integer)id == columns.length - 1) && SystemUtils.IS_OS_LINUX)
         {
            // Attempt workaround for Linux issue when last table column grows by few pixels on each open
            try
            {
               int oldWidth = settings.getAsInteger(prefix + "." + id + ".width", 0);
               ScrollBar sb = table.getVerticalBar();
               if ((sb != null) && (oldWidth < width) && (width - oldWidth <= sb.getSize().y))
               {
                  // assume that last column grows because of a bug
                  width = oldWidth;
               }
            }
            catch(NumberFormatException e)
            {
            }
         }
         settings.set(prefix + "." + id + ".width", width); //$NON-NLS-1$ //$NON-NLS-2$
		}
	}

	/**
    * Restore settings of table viewer columns previously saved by call to WidgetHelper.saveColumnSettings
    *
    * @param table Table control
    * @param prefix Prefix for properties
    */
   public static void restoreColumnSettings(Table table, String prefix)
	{
      PreferenceStore settings = PreferenceStore.getInstance();
		TableColumn[] columns = table.getColumns();
		for(int i = 0; i < columns.length; i++)
		{
			try
			{
			   Object id = columns[i].getData("ID");
			   if ((id == null) || !(id instanceof Integer))
			      id = Integer.valueOf(i);
            int w = settings.getAsInteger(prefix + "." + id + ".width", 0); //$NON-NLS-1$ //$NON-NLS-2$
            if (w > 0)
               columns[i].setWidth(w);
			}
			catch(NumberFormatException e)
			{
			}
		}
	}

	/**
	 * Save settings of tree viewer columns
	 * 
	 * @param table Table control
	 * @param prefix Prefix for properties
	 */
   public static void saveColumnSettings(Tree tree, String prefix)
	{
      PreferenceStore settings = PreferenceStore.getInstance();
		TreeColumn[] columns = tree.getColumns();
		for(int i = 0; i < columns.length; i++)
		{
         Object id = columns[i].getData("ID");
         if ((id == null) || !(id instanceof Integer))
            id = Integer.valueOf(i);
         settings.set(prefix + "." + id + ".width", columns[i].getWidth()); //$NON-NLS-1$ //$NON-NLS-2$
		}
	}
	
	/**
	 * Restore settings of tree viewer columns previously saved by call to WidgetHelper.saveColumnSettings
	 * 
	 * @param table Table control
	 * @param prefix Prefix for properties
	 */
   public static void restoreColumnSettings(Tree tree, String prefix)
	{
      PreferenceStore settings = PreferenceStore.getInstance();
		TreeColumn[] columns = tree.getColumns();
		for(int i = 0; i < columns.length; i++)
		{
			try
			{
	         Object id = columns[i].getData("ID");
	         if ((id == null) || !(id instanceof Integer))
	            id = Integer.valueOf(i);
            int w = settings.getAsInteger(prefix + "." + id + ".width", 0); //$NON-NLS-1$ //$NON-NLS-2$
            if (w > 0)
               columns[i].setWidth(w);
			}
			catch(NumberFormatException e)
			{
			}
		}
	}
	
	/**
	 * Save settings for sortable table viewer
	 * @param viewer Viewer
	 * @param prefix Prefix for properties
	 */
   public static void saveTableViewerSettings(SortableTableViewer viewer, String prefix)
	{
		final Table table = viewer.getTable();
      saveColumnSettings(table, prefix);
		TableColumn column = table.getSortColumn();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(prefix + ".sortColumn", (column != null) ? (Integer)column.getData("ID") : -1); //$NON-NLS-1$ //$NON-NLS-2$
      settings.set(prefix + ".sortDirection", table.getSortDirection()); //$NON-NLS-1$
	}
	
	/**
	 * Restore settings for sortable table viewer
	 * @param viewer Viewer
	 * @param prefix Prefix for properties
	 */
   public static void restoreTableViewerSettings(SortableTableViewer viewer, String prefix)
	{
		final Table table = viewer.getTable();
      restoreColumnSettings(table, prefix);
      PreferenceStore settings = PreferenceStore.getInstance();
		try
		{
         table.setSortDirection(settings.getAsInteger(prefix + ".sortDirection", SWT.UP)); //$NON-NLS-1$
         int column = settings.getAsInteger(prefix + ".sortColumn", 0); //$NON-NLS-1$
			if (column >= 0)
			{
				table.setSortColumn(viewer.getColumnById(column));
			}
		}
		catch(NumberFormatException e)
		{
		}
	}
	
	/**
	 * Save settings for sortable tree viewer
	 * @param viewer Viewer
	 * @param prefix Prefix for properties
	 */
   public static void saveTreeViewerSettings(SortableTreeViewer viewer, String prefix)
	{
		final Tree tree = viewer.getTree();
      saveColumnSettings(tree, prefix);
		TreeColumn column = tree.getSortColumn();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(prefix + ".sortColumn", (column != null) ? (Integer)column.getData("ID") : -1); //$NON-NLS-1$ //$NON-NLS-2$
      settings.set(prefix + ".sortDirection", tree.getSortDirection()); //$NON-NLS-1$
	}
	
	/**
	 * Restore settings for sortable table viewer
	 * @param viewer Viewer
	 * @param prefix Prefix for properties
	 */
   public static void restoreTreeViewerSettings(SortableTreeViewer viewer, String prefix)
	{
		final Tree tree = viewer.getTree();
      restoreColumnSettings(tree, prefix);
      PreferenceStore settings = PreferenceStore.getInstance();
      tree.setSortDirection(settings.getAsInteger(prefix + ".sortDirection", SWT.UP)); //$NON-NLS-1$
      int column = settings.getAsInteger(prefix + ".sortColumn", 0); //$NON-NLS-1$
      if (column >= 0)
		{
         tree.setSortColumn(viewer.getColumnById(column));
		}
	}

	/**
    * Wrapper for saveTableViewerSettings/saveTreeViewerSettings
    * 
    * @param viewer Viewer
    * @param prefix Prefix for properties
    */
   public static void saveColumnViewerSettings(ColumnViewer viewer, String prefix)
	{
		if (viewer instanceof SortableTableViewer)
		{
         saveTableViewerSettings((SortableTableViewer)viewer, prefix);
		}
		else if (viewer instanceof SortableTreeViewer)
		{
         saveTreeViewerSettings((SortableTreeViewer)viewer, prefix);
		}
	}
	
	/**
    * Wrapper for restoreTableViewerSettings/restoreTreeViewerSettings
    * 
    * @param viewer table or tree viewer
    * @param prefix Prefix for properties
    */
   public static void restoreColumnViewerSettings(ColumnViewer viewer, String prefix)
	{
		if (viewer instanceof SortableTableViewer)
		{
         restoreTableViewerSettings((SortableTableViewer)viewer, prefix);
		}
		else if (viewer instanceof SortableTreeViewer)
		{
         restoreTreeViewerSettings((SortableTreeViewer)viewer, prefix);
		}
	}
	
	/**
	 * Copy given text to clipboard
	 * 
	 * @param text 
	 */
	public static void copyToClipboard(final String text)
	{
		final Clipboard cb = new Clipboard(Display.getCurrent());
      Transfer transfer = TextTransfer.getInstance();
      cb.setContents(new Object[] { (text != null) ? text : "" }, new Transfer[] { transfer }); //$NON-NLS-1$
      cb.dispose();
   }
	
	/**
	 * @param manager
	 * @param control
	 * @param readOnly
	 */
	public static void addStyledTextEditorActions(final IMenuManager manager, final StyledText control, boolean readOnly)
	{
	}

   /**
    * Get best fitting font from given font list for given string and bounding rectangle.
    * String should fit using multiline.
    * Fonts in the list must be ordered from smaller to larger.
    * 
    * @param gc GC
    * @param fonts list of available fonts
    * @param text text to fit
    * @param width width of bounding rectangle
    * @param height height of bounding rectangle
    * @param maxLineCount maximum line count that should be used
    * @return best font by position in array
    */
   public static int getBestFittingFontMultiline(GC gc, Font[] fonts, String text, int width, int height, int maxLineCount)
   {
      int first = 0;
      int last = fonts.length - 1;
      int curr = last / 2;
      int font = 0;
      while(last > first)
      {
         gc.setFont(fonts[curr]);
         if (fitToRect(gc, text, width, height, maxLineCount))
         {
            font = curr;
            first = curr + 1;
            curr = first + (last - first) / 2;
         }
         else
         {
            last = curr - 1;
            curr = first + (last - first) / 2;
         }
      }
      
      return font;
   }
   
   /**
    * Checks if string fits to given rectangle using font set already set in GC
    * 
    * @param gc GC
    * @param text text to fit
    * @param width width of bounding rectangle
    * @param height height of bounding rectangle
    * @param maxLineCount maximum line count that should be used
    * @return if string fits in the field
    */
   public static boolean fitToRect(GC gc, String text, int width, int height, int maxLineCount)
   {
      Point ext = gc.textExtent(text);
      if (ext.y > height)
         return false;
      if (ext.x <= width)
         return true;
      
      FittedString newString = fitStringToSector(gc, text, width, maxLineCount > 0 ? Math.min(maxLineCount, (int)(height / ext.y)) : (int)(height / ext.y));
      return newString.isCutted(); 
   }
   
   /**
    * Calculate substring for string to fit in the sector
    * 
    * @param gc gc object
    * @param text object name
    * @param maxLineCount number of lines that can be used to display object name
    * @return  formated string
    */
   public static FittedString fitStringToSector(GC gc, String text, int width, int maxLineCount)
   {
      StringBuilder name = new StringBuilder("");
      int start = 0;
      boolean fit = true;
      for(int i = 0; start < text.length(); i++)
      {         
         if(i >= maxLineCount)
         {
            fit = false;
            break;
         }
         
         String substr = text.substring(start);
         int nameL = gc.textExtent(substr).x;
         int numOfCharToLeave = (int)((width - 6)/(nameL/substr.length())); //make best guess
         if(numOfCharToLeave >= substr.length())
            numOfCharToLeave = substr.length();
         String tmp = substr;
         
         while(gc.textExtent(tmp).x > width)
         {
            numOfCharToLeave--;
            tmp = substr.substring(0, numOfCharToLeave);
            Matcher matcher = patternOnlyCharNum.matcher(tmp);
            if(matcher.matches() || (i+1 == maxLineCount && numOfCharToLeave != substr.length()))
            {               
               Matcher matcherReplaceDot = patternAllDotsAtEnd.matcher(tmp);
               tmp = matcherReplaceDot.replaceAll("");
               tmp += "...";     
               fit = false;
            }
            else
            {
               Matcher matcherRemoveCharsAfterSeparator = patternCharsAndNumbersAtEnd.matcher(tmp);
               tmp = matcherRemoveCharsAfterSeparator.replaceAll("");
               numOfCharToLeave = tmp.length();
            }               
         } 
         
         name.append(tmp);
         if(i+1 < maxLineCount && numOfCharToLeave != substr.length())
         {
            name.append("\n");
         }
         
         Matcher matcherRemoveLineEnd = patternCharsAndNumbersAtStart.matcher(substr.substring(numOfCharToLeave-1));
         numOfCharToLeave = substr.length() - matcherRemoveLineEnd.replaceAll("").length(); //remove if something left after last word
         start = start+numOfCharToLeave+1;
      }           
      
      return new FittedString(name.toString(), fit);    
   }
		
	/**
	 * Get best fitting font from given font list for given string and bounding rectangle.
	 * Fonts in the list must be ordered from smaller to larger.
	 * 
	 * @param gc GC
	 * @param fonts list of available fonts
	 * @param text text to fit
	 * @param width width of bounding rectangle
	 * @param height height of bounding rectangle
	 * @return best font
	 */
	public static Font getBestFittingFont(GC gc, Font[] fonts, String text, int width, int height)
	{
		int first = 0;
		int last = fonts.length - 1;
		int curr = last / 2;
		Font font = null;
		while(last > first)
		{
			gc.setFont(fonts[curr]);
			Point ext = gc.textExtent(text);
			if ((ext.x <= width) && (ext.y <= height))
			{
				font = fonts[curr];
				first = curr + 1;
				curr = first + (last - first) / 2;
			}
			else
			{
				last = curr - 1;
				curr = first + (last - first) / 2;
			}
		}
		
		// Use smallest font if no one fit
		if (font == null)
			font = fonts[0];
		return font;
	}
	
	/**
	 * Find font with matching size in font array.
	 * 
	 * @param fonts fonts to select from
	 * @param sourceFont font to match
	 * @return matching font or null
	 */
	public static Font getMatchingSizeFont(Font[] fonts, Font sourceFont)
	{
      float h = sourceFont.getFontData()[0].getHeight();
		for(int i = 0; i < fonts.length; i++)
         if (fonts[i].getFontData()[0].getHeight() == h)
				return fonts[i];
		return null;
	}

	/**
	 * Validate text input
	 * 
	 * @param text text control
	 * @param validator validator
	 * @return true if text is valid
	 */
	private static boolean validateTextInputInternal(Control control, String text, String label, TextFieldValidator validator, PreferencePage page)
	{
		if (!control.isEnabled())
			return true;	// Ignore validation for disabled controls
		
		boolean ok = validator.validate(text);
      control.setBackground(ok ? null : ThemeEngine.getBackgroundColor("TextInput.Error"));
		if (ok)
		{
			if (page != null)
				page.setErrorMessage(null);
		}
		else
		{
			if (page != null)
				page.setErrorMessage(validator.getErrorMessage(text, label));
			else	
            MessageDialogHelper.openError(control.getShell(),
                  LocalizationHelper.getI18n(WidgetHelper.class).tr("Input Validation Error"),
                  validator.getErrorMessage(text, label));
		}
		return ok;
	}

	/**
	 * Validate text input
	 * 
	 * @param text text control
	 * @param validator validator
	 * @return true if text is valid
	 */
   public static boolean validateTextInput(Text text, String label, TextFieldValidator validator, PreferencePage page)
	{
		return validateTextInputInternal(text, text.getText(), label, validator, page);
	}
	
	/**
	 * Validate text input
	 * 
	 * @param text text control
	 * @param validator validator
	 * @return true if text is valid
	 */
   public static boolean validateTextInput(LabeledText text, TextFieldValidator validator, PreferencePage page)
	{
		return validateTextInputInternal(text.getTextControl(), text.getText(), text.getLabel(), validator, page);
	}
	
	/**
	 * Convert font size in pixels to platform-dependent (DPI dependent actually) points
	 * @param device
	 * @param px
	 * @return
	 */
	public static int fontPixelsToPoints(Display device, int px)
	{
		return (int)Math.round(px * 72.0 / device.getDPI().y);
	}

   /**
    * Scale text points relative to "basic" 96 DPI.
    * 
    * @param device
    * @param pt
    * @return
    */
   public static int scaleTextPoints(Display device, int pt)
   {
      return (int)Math.round(pt * (device.getDPI().y / 96.0));
   }
	
   /**
    * Get width of given text in pixels using settings from given control
    * 
    * @param control
    * @param text
    * @return
    */
   public static int getTextWidth(Control control, String text)
   {
      return getTextExtent(control, text).x;
   }

   /**
    * Get width and height of given text in pixels using settings from given control
    * 
    * @param control
    * @param text
    * @return
    */
   public static Point getTextExtent(Control control, String text)
   {
      GC gc = new GC(control);
      gc.setFont(control.getFont());
      Point e = gc.textExtent(text);
      gc.dispose();
      return e;
   }
   
   /**
    *  Get column index by column ID
    *  
    * @param table table control
    * @param id the id index to be found by
    * @return index of the column
    */
   public static int getColumnIndexById(Table table, int id)
   {
      int index = -1;
      TableColumn[] columns = table.getColumns();
      for(int i = 0; i < columns.length; i++)
      {
         if (!columns[i].isDisposed() && ((Integer)columns[i].getData("ID") == id)) //$NON-NLS-1$
         {
            index = i;
            break;
         }
      }
      
      return index;
   }

   /**
    *  Get column index by column ID
    *  
    * @param tree tree control
    * @param id the id index to be found by
    * @return index of the column
    */
   public static int getColumnIndexById(Tree tree, int id)
   {
      int index = -1;
      TreeColumn[] columns = tree.getColumns();
      for(int i = 0; i < columns.length; i++)
      {
         if (!columns[i].isDisposed() && ((Integer)columns[i].getData("ID") == id)) //$NON-NLS-1$
         {
            index = i;
            break;
         }
      }
      
      return index;
   }

   /**
    * Compute wrap size for given text. Copied from Eclipse forms plugin.
    *
    * @param gc
    * @param text
    * @param wHint
    * @return
    */
   public static Point computeWrapSize(GC gc, String text, int wHint)
   {
      BreakIterator wb = BreakIterator.getWordInstance();
      wb.setText(text);
      FontMetrics fm = gc.getFontMetrics();
      int lineHeight = fm.getHeight();

      int saved = 0;
      int last = 0;
      int height = lineHeight;
      int maxWidth = 0;
      for(int loc = wb.first(); loc != BreakIterator.DONE; loc = wb.next())
      {
         String word = text.substring(saved, loc);
         Point extent = gc.textExtent(word);
         if (extent.x > wHint)
         {
            // overflow
            saved = last;
            height += extent.y;
            // switch to current word so maxWidth will accommodate very long single words
            word = text.substring(last, loc);
            extent = gc.textExtent(word);
         }
         maxWidth = Math.max(maxWidth, extent.x);
         last = loc;
      }
      /*
       * Correct the height attribute in case it was calculated wrong due to wHint being less than maxWidth. The recursive call
       * proved to be the only thing that worked in all cases. Some attempts can be made to estimate the height, but the algorithm
       * needs to be run again to be sure.
       */
      if (maxWidth > wHint)
         return computeWrapSize(gc, text, maxWidth);
      return new Point(maxWidth, height);
   }

   /**
    * Paint wrapped text. Copied from Eclipse forms plugin.
    *
    * @param gc
    * @param text
    * @param bounds
    */
   public static void paintWrapText(GC gc, String text, Rectangle bounds)
   {
      paintWrapText(gc, text, bounds, false);
   }

   /**
    * Paint wrapped text. Copied from Eclipse forms plugin.
    *
    * @param gc
    * @param text
    * @param bounds
    * @param underline
    */
   public static void paintWrapText(GC gc, String text, Rectangle bounds, boolean underline)
   {
      BreakIterator wb = BreakIterator.getWordInstance();
      wb.setText(text);
      FontMetrics fm = gc.getFontMetrics();
      int lineHeight = fm.getHeight();
      int descent = 2; // FIXME: how to find correct value fm.getDescent();

      int saved = 0;
      int last = 0;
      int y = bounds.y;
      int width = bounds.width;

      for(int loc = wb.first(); loc != BreakIterator.DONE; loc = wb.next())
      {
         String line = text.substring(saved, loc);
         Point extent = gc.textExtent(line);

         if (extent.x > width)
         {
            // overflow
            String prevLine = text.substring(saved, last);
            gc.drawText(prevLine, bounds.x, y, true);
            if (underline)
            {
               Point prevExtent = gc.textExtent(prevLine);
               int lineY = y + lineHeight - descent + 1;
               gc.drawLine(bounds.x, lineY, bounds.x + prevExtent.x, lineY);
            }

            saved = last;
            y += lineHeight;
         }
         last = loc;
      }
      // paint the last line
      String lastLine = text.substring(saved, last);
      gc.drawText(lastLine, bounds.x, y, true);
      if (underline)
      {
         int lineY = y + lineHeight - descent + 1;
         Point lastExtent = gc.textExtent(lastLine);
         gc.drawLine(bounds.x, lineY, bounds.x + lastExtent.x, lineY);
      }
   }

   /**
    * Escape text for HTML
    * 
    * @param text text to escape
    * @param convertNl if true will convert new line character to <br>
    *           tag
    * @param jsString if true will escape ' and \ with \
    * @return
    */
   public static String escapeText(final String text, boolean convertNl, boolean jsString)
   {
      StringBuffer sb = new StringBuffer();
      int textLength = text.length();
      for(int i = 0; i < textLength; i++)
      {
         char ch = text.charAt(i);
         switch(ch)
         {
            case '&':
               sb.append("&amp;");
               break;
            case '<':
               sb.append("&lt;");
               break;
            case '>':
               sb.append("&gt;");
               break;
            case '"':
               sb.append("&quot;");
               break;
            case '\r':
               if (!convertNl)
                  sb.append(ch);
               break;
            case '\n':
               sb.append(convertNl ? "<br>" : ch);
               break;
            case '\'':
               sb.append(jsString ? "\\'" : ch);
               break;
            case '\\':
               sb.append(jsString ? "\\\\" : ch);
               break;
            default:
               sb.append(ch);
               break;
         }
      }
      return sb.toString();
   }

   /**
    * Attach mouse track listener to composite. This listener is only suitable for hover detection. Only one listener can be
    * attached.
    * 
    * @param control control to attach listener to
    * @param listener mouse track listener
    */
   public static void attachMouseTrackListener(Composite control, MouseTrackListener listener)
   {
      WidgetUtil.registerDataKeys("msgProxyWidget");

      MsgProxyWidget proxy = (MsgProxyWidget)control.getData("msgProxyWidgetInternal");
      if (proxy == null)
      {
         proxy = new MsgProxyWidget(control);
         control.setData("msgProxyWidgetInternal", proxy);
         control.setData("msgProxyWidget", proxy.getRemoteObjectId());
      }
      proxy.setMouseTrackListener(listener);

      ClientListener clientListener = (ClientListener)Registry.getProperty("MouseHoverListener");
      if (clientListener == null)
      {
         String script = loadResourceAsText("js/hover.js");
         if (script == null)
            return;
         clientListener = new ClientListener(script);
         Registry.getInstance().setProperty("MouseHoverListener", clientListener);
      }
      control.addListener(SWT.MouseEnter, clientListener);
      control.addListener(SWT.MouseMove, clientListener);
      control.addListener(SWT.MouseExit, clientListener);
   }

   /**
    * Load resource file as text
    * 
    * @param resource
    * @return
    */
   private static String loadResourceAsText(String resource)
   {
      InputStream input = WidgetHelper.class.getClassLoader().getResourceAsStream(resource);
      if (input == null)
      {
         logger.error("Resource " + resource + " not found");
         return null;
      }

      BufferedReader reader = null;
      try
      {
         reader = new BufferedReader(new InputStreamReader(input, "UTF-8"));
         StringBuilder builder = new StringBuilder();
         String line = reader.readLine();
         while(line != null)
         {
            builder.append(line);
            builder.append('\n');
            line = reader.readLine();
         }
         return builder.toString();
      }
      catch(Exception e)
      {
         logger.error("Exception while loading resource " + resource, e);
         return null;
      }
      finally
      {
         try
         {
            if (reader != null)
               reader.close();
            else
               input.close();
         }
         catch(IOException e)
         {
         }
      }
   }

   /**
    * Helper method to set scroll bar increment. Do nothing in web version.
    *
    * @param scrollable scrollable to configure scrollbar for
    * @param direction scrollbar direction (<code>SWT.HORIZONTAL</code> or <code>SWT.VERTICAL</code>)
    * @param increment increment value
    */
   public static void setScrollBarIncrement(Scrollable scrollable, int direction, int increment)
   {
   }

   /**
    * Wrapper for <code>GC.setInterpolation(SWT.HIGH)</code> (compatibility layer for RAP).
    *
    * @param gc GC to set high interpolation mode for
    */
   public static void setHighInterpolation(GC gc)
   {
   }

   /**
    * Helper method to set file dialog filter extensions (compatibility layer for RAP).
    *
    * @param fd file dialog to set extensions for
    * @param extensions file extensions
    */
   public static void setFileDialogFilterExtensions(FileDialog fd, String[] extensions)
   {
   }

   /**
    * Helper method to set file dialog filter extension names (compatibility layer for RAP).
    *
    * @param fd file dialog to set extensions for
    * @param names extension names
    */
   public static void setFileDialogFilterNames(FileDialog fd, String[] names)
   {
   }

   /**
    * Set custom type for control (compatibility layer for RAP, has no effect in desktop build).
    *
    * @param control control to set type for
    * @param type custom type
    */
   public static void setControlCustomType(Control control, String type)
   {
      control.setData(RWT.CUSTOM_VARIANT, type);
   }

   /**
    * Get clipboard available types (compatibility layer for RAP, has no effect in desktop build)
    * 
    * @param cb
    * @return
    */
   public static TransferData[] getAvailableTypes(Clipboard cb)
   {
      //TODO: implement types for web
      return new TransferData[] {};
   }
}