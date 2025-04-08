/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.BreakIterator;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.function.Consumer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.ST;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.TextTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontMetrics;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageDataProvider;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Scrollable;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledControl;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
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
      return SystemUtils.IS_OS_WINDOWS ? "\n" : "\n";
   }

   /**
    * Mask password. Will not mask encrypted passwords.
    *
    * @param password password to mask
    * @return masked password
    */
   public static String maskPassword(String password)
   {
      if (password.length() == 44 && password.endsWith("="))
         return password;

      char[] content = password.toCharArray();
      for(int i = 1; i < content.length - 1; i++)
         content[i] = '*';
      return new String(content);
   }

   /**
    * Redo window layout and resize it to accommodate possible layout changes.
    * 
    * @param window window to resize
    */
   public static void adjustWindowSize(Window window)
   {
      Shell shell = window.getShell();
      shell.layout(true, true);
      shell.pack();
   }

   /**
    * Redo property dialog layout and resize it to accommodate possible layout changes.
    * 
    * @param window window to resize
    */
   public static void adjustWindowSize(PreferencePage page)
   {
      Shell shell = page.getShell();
      shell.layout(true, true);
      shell.pack();
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
   public static Text createLabeledText(final Composite parent, int flags, int widthHint, final String labelText, final String initialText, Object layoutData)
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
		GridData gridData = new GridData();
		gridData.horizontalAlignment = GridData.FILL;
		gridData.grabExcessHorizontalSpace = true;
		gridData.verticalAlignment = GridData.FILL;
		gridData.grabExcessVerticalSpace = true;
		gridData.widthHint = widthHint;
		text.setLayoutData(gridData);		
		
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
		label.setBackground(parent.getBackground());

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
      settings.set(prefix + ".sortColumn", (column != null) ? (Integer)column.getData("ID") : -1);
      settings.set(prefix + ".sortDirection", table.getSortDirection());
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
         table.setSortDirection(settings.getAsInteger(prefix + ".sortDirection", SWT.UP));
         int column = settings.getAsInteger(prefix + ".sortColumn", 0);
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
      settings.set(prefix + ".sortColumn", (column != null) ? (Integer)column.getData("ID") : -1);
      settings.set(prefix + ".sortDirection", tree.getSortDirection());
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
      cb.setContents(new Object[] { (text != null) ? text : "" }, new Transfer[] { transfer });
      cb.dispose();
   }

   /**
    * Copy given image to clipboard
    * 
    * @param text
    */
   public static void copyToClipboard(final Image image)
   {
      final Clipboard cb = new Clipboard(Display.getCurrent());
      ImageTransfer imageTransfer = ImageTransfer.getInstance();
      cb.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
      cb.dispose();
   }

	/**
	 * @param manager
	 * @param control
	 * @param readOnly
	 */
	public static void addStyledTextEditorActions(final IMenuManager manager, final StyledText control, boolean readOnly)
	{
		if (!readOnly)
		{
         final Action cut = new Action("Cut") {
				@Override
				public void run()
				{
					control.cut();
				}
			};
			cut.setImageDescriptor(SharedIcons.CUT);
			manager.add(cut);
		}
		
      final Action copy = new Action("&Copy") {
			@Override
			public void run()
			{
				control.copy();
			}
		};
		copy.setImageDescriptor(SharedIcons.COPY);
		manager.add(copy);

		if (!readOnly)
		{
         final Action paste = new Action("&Paste") {
				@Override
				public void run()
				{
					control.paste();
				}
			};
			paste.setImageDescriptor(SharedIcons.PASTE);
			manager.add(paste);
			
         final Action delete = new Action("&Delete") {
				@Override
				public void run()
				{
					control.invokeAction(ST.DELETE_NEXT);
				}
			};
			manager.add(delete);

			manager.add(new Separator());
		}
		
      final Action selectAll = new Action("Select &all") {
			@Override
			public void run()
			{
				control.selectAll();
			}
		};
		manager.add(selectAll);
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
      Font originalFont = gc.getFont();

      int first = 0;
      int last = fonts.length - 1;
      int curr = last / 2;
      int font = 0;
      while(last > first)
      {
         gc.setFont(fonts[curr]);
         if (fitStringToRect(gc, text, width, height, maxLineCount))
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

      gc.setFont(originalFont);
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
    * @return true if string was cut
    */
   public static boolean fitStringToRect(GC gc, String text, int width, int height, int maxLineCount)
   {
      Point ext = gc.textExtent(text);
      if (ext.y > height)
         return false;
      if (ext.x <= width)
         return true;

      FittedString newString = fitStringToArea(gc, text, width, maxLineCount > 0 ? Math.min(maxLineCount, (int)(height / ext.y)) : (int)(height / ext.y));
      return newString.isCutted(); 
   }

   /**
    * Calculate substring for string to fit into the given area defined by width in pixels and number of lines of text
    * 
    * @param gc gc object
    * @param text object name
    * @param maxLineCount number of lines that can be used to display text
    * @return formated string
    */
   public static FittedString fitStringToArea(GC gc, String text, int width, int maxLineCount)
   {
      StringBuilder name = new StringBuilder("");
      int start = 0;
      boolean fit = true;
      for(int i = 0; start < text.length(); i++)
      {
         if (i >= maxLineCount)
         {
            fit = false;
            break;
         }

         String substr = text.substring(start);
         int nameL = gc.textExtent(substr).x;
         int numOfCharToLeave = (int)((width - 6) / (nameL / substr.length())); // make best guess
         if (numOfCharToLeave >= substr.length())
            numOfCharToLeave = substr.length();
         String tmp = substr;

         while(gc.textExtent(tmp).x > width)
         {
            numOfCharToLeave--;
            tmp = substr.substring(0, numOfCharToLeave);
            Matcher matcher = patternOnlyCharNum.matcher(tmp);
            if (matcher.matches() || (i + 1 == maxLineCount && numOfCharToLeave != substr.length()))
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
         if (i + 1 < maxLineCount && numOfCharToLeave != substr.length())
         {
            name.append("\n");
         }

         Matcher matcherRemoveLineEnd = patternCharsAndNumbersAtStart.matcher(substr.substring(numOfCharToLeave - 1));
         numOfCharToLeave = substr.length() - matcherRemoveLineEnd.replaceAll("").length(); // remove if something left after last word
         start = start + numOfCharToLeave + 1;
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
      Font originalFont = gc.getFont();

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

      gc.setFont(originalFont);
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
		float h = sourceFont.getFontData()[0].height;
		for(int i = 0; i < fonts.length; i++)
			if (fonts[i].getFontData()[0].height == h)
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
   private static boolean validateTextInputInternal(LabeledControl parentControl, Control control, String text, TextFieldValidator validator)
	{
		if (!control.isEnabled())
			return true;	// Ignore validation for disabled controls

		boolean ok = validator.validate(text);
      control.setBackground(ok ? null : ThemeEngine.getBackgroundColor("TextInput.Error"));
		if (ok)
		{
         if (parentControl != null)
            parentControl.setErrorMessage(null);
		}
		else
		{
         if (parentControl != null)
            parentControl.setErrorMessage(validator.getErrorMessage(text));
			else	
            MessageDialogHelper.openError(control.getShell(),
                  LocalizationHelper.getI18n(WidgetHelper.class).tr("Input Validation Error"),
                  validator.getErrorMessage(text));
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
   public static boolean validateTextInput(Text text, TextFieldValidator validator)
	{
      return validateTextInputInternal(null, text, text.getText(), validator);
	}

	/**
	 * Validate text input
	 * 
	 * @param text text control
	 * @param validator validator
	 * @return true if text is valid
	 */
   public static boolean validateTextInput(LabeledText text, TextFieldValidator validator)
	{
      return validateTextInputInternal(text, text.getTextControl(), text.getText(), validator);
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
    * Get width and height of given text in pixels using given control and font
    * 
    * @param control
    * @param font
    * @param text
    * @return
    */
   public static Point getTextExtent(Control control, Font font, String text)
   {
      GC gc = new GC(control);
      gc.setFont(font);
      Point e = gc.textExtent(text);
      gc.dispose();
      return e;
   }

   /**
    * Get column index by column ID
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
      int descent = fm.getDescent();

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
    * Attach mouse track listener to composite (compatibility layer for RAP).
    * 
    * @param control control to attach listener to
    * @param listener mouse track listener
    */
   public static void attachMouseTrackListener(Composite control, MouseTrackListener listener)
   {
      control.addMouseTrackListener(listener);
   }

   /**
    * Attach mouse move listener to composite (compatibility layer for RAP).
    * 
    * @param control control to attach listener to
    * @param listener mouse track listener
    */
   public static void attachMouseMoveListener(Composite control, MouseMoveListener listener)
   {
      control.addMouseMoveListener(listener);
   }

   /**
    * Helper method to set scroll bar increment (compatibility layer for RAP).
    *
    * @param scrollable scrollable to configure scrollbar for
    * @param direction scrollbar direction (<code>SWT.HORIZONTAL</code> or <code>SWT.VERTICAL</code>)
    * @param increment increment value
    */
   public static void setScrollBarIncrement(Scrollable scrollable, int direction, int increment)
   {
      ScrollBar bar = (direction == SWT.HORIZONTAL) ? scrollable.getHorizontalBar() : scrollable.getVerticalBar();
      if (bar != null)
         bar.setIncrement(increment);
   }

   /**
    * Wrapper for <code>GC.setInterpolation(SWT.HIGH)</code> (compatibility layer for RAP).
    *
    * @param gc GC to set high interpolation mode for
    */
   public static void setHighInterpolation(GC gc)
   {
      gc.setInterpolation(SWT.HIGH);
   }

   /**
    * Set control redraw flag. Should be used when redraw flag should be changed in desktop version but not in web version. Do
    * nothing for web client.
    * 
    * @param control control to change redraw flag on
    * @param redraw new redraw flag
    */
   public static void setRedraw(Control control, boolean redraw)
   {
      control.setRedraw(redraw);
   }

   /**
    * Helper method to set file dialog filter extensions (compatibility layer for RAP).
    *
    * @param fd file dialog to set extensions for
    * @param extensions file extensions
    */
   public static void setFileDialogFilterExtensions(FileDialog fd, String[] extensions)
   {
      fd.setFilterExtensions(extensions);
   }

   /**
    * Helper method to set file dialog filter extension names (compatibility layer for RAP).
    *
    * @param fd file dialog to set extensions for
    * @param names extension names
    */
   public static void setFileDialogFilterNames(FileDialog fd, String[] names)
   {
      fd.setFilterNames(names);
   }

   /**
    * Helper method to get file list from file dialog (compatibility layer for RAP).
    *
    * @param fd file dialog to set extensions for
    * @param names extension names
    */
   public static void getFileDialogFileList(FileDialog fd, List<File> fileList)
   {
      String files[] = fd.getFileNames();
      if(files.length > 0)
      {
         fileList.clear();
         for(int i = 0; i < files.length; i++)
         {
            fileList.add(new File(fd.getFilterPath(), files[i]));
         }
      }
   }

   /**
    * Set custom type for control (compatibility layer for RAP, has no effect in desktop build).
    *
    * @param control control to set type for
    * @param type custom type
    */
   public static void setControlCustomType(Control control, String type)
   {
   }

   /**
    * Disable selection bar on tab folder (compatibility layer for RAP, has no effect in web build).
    *
    * @param tabFolder tab folder to disable selection bar in
    */
   public static void disableTabFolderSelectionBar(CTabFolder tabFolder)
   {
      tabFolder.setSelectionBarThickness(0);
   }

   /**
    * Get clipboard available types (compatibility layer for RAP, has no effect in desktop build)
    * 
    * @param cb
    * @return
    */
   public static TransferData[] getAvailableTypes(Clipboard cb)
   {
      return cb.getAvailableTypes();
   }

   /**
    * Create image from input stream.
    *
    * @param display owning display
    * @param stream input stream
    * @return image object or null on error
    */
   public static Image createImageFromStream(Display display, InputStream stream)
   {
      return new Image(display, stream);
   }

   /**
    * Create image from image data (intended for use in non-UI threads).
    *
    * @param display owning display
    * @param imageData image data
    * @return image object or null on error
    */
   public static Image createImageFromImageData(Display display, ImageData imageData)
   {
      return new Image(display, imageData);
   }

   /**
    * Create image descriptor from image data
    *
    * @param imageData image data
    * @return image descriptor object
    */
   public static ImageDescriptor createImageDescriptor(ImageData imageData)
   {
      return ImageDescriptor.createFromImageDataProvider(new ImageDataProvider() {
            @Override
            public ImageData getImageData(int zoom)
            {
               return imageData;
            }
         });
   }

   /**
    * Save given image to file (will show file save dialog in desktop client and initiate download in web client).
    * 
    * @param view parent view (can be null)
    * @param fileNameHint hint for the file name (can be null)
    * @param image image to save
    */
   public static void saveImageToFile(View view, String fileNameHint, Image image)
   {
      final ImageData[] data = new ImageData[] { image.getImageData() };
      exportFile(view, fileNameHint, new String[] { "*.png", "*.*" }, new String[] { "PNG files", "All files" }, "application/octet-stream", (fileName) -> {
         ImageLoader saver = new ImageLoader();
         saver.data = data;
         saver.save(fileName, SWT.IMAGE_PNG);
      });
   }

   /**
    * Save given text to file (will show file save dialog in desktop client and initiate download in web client).
    * 
    * @param view parent view (can be null)
    * @param fileNameHint hint for the file name (can be null)
    * @param text text to save
    */
   public static void saveTextToFile(View view, String fileNameHint, String text)
   {
      saveTextToFile(view, fileNameHint, new String[] { "*.*" }, new String[] { "All files" }, text);
   }

   /**
    * Save given text to file (will show file save dialog in desktop client and initiate download in web client).
    * 
    * @param view parent view (can be null)
    * @param fileNameHint hint for the file name (can be null)
    * @param fileExtensions file filter extensions (can be null)
    * @param fileExtensionNames file filter extension names (can be null)
    * @param text text to save
    */
   public static void saveTextToFile(View view, String fileNameHint, String[] fileExtensions, String[] fileExtensionNames, String text)
   {
      exportFile(view, fileNameHint, fileExtensions, fileExtensionNames, "text/plain", (fileName) -> {
         try (OutputStream out = new FileOutputStream(new File(fileName)))
         {
            out.write(text.getBytes("utf-8"));
         }
      });
   }

   /**
    * Save temporary file to location provided by user.
    * 
    * @param view parent view (can be null)
    * @param fileNameHint hint for the file name (can be null)
    * @param fileExtensions file filter extensions (can be null)
    * @param fileExtensionNames file filter extension names (can be null)
    * @param file temporary file to save
    * @param mimeType file MIME type (ignored for desktop)
    */
   public static void saveTemporaryFile(View view, String fileNameHint, String[] fileExtensions, String[] fileExtensionNames, File file, String mimeType)
   {
      exportFile(view, fileNameHint, fileExtensions, fileExtensionNames, mimeType, (fileName) -> {
         try (InputStream in = new FileInputStream(file); OutputStream out = new FileOutputStream(new File(fileName)))
         {
            IOUtils.copy(in, out);
         }
      });
   }

   /**
    * Export data to file (will show file save dialog in desktop client and initiate download in web client).
    * 
    * @param view parent view (can be null)
    * @param fileNameHint hint for the file name (can be null)
    * @param fileExtensions file filter extensions (can be null)
    * @param fileExtensionNames file filter extension names (can be null)
    * @param mimeType file MIME type (ignored for desktop)
    * @param processor export data processor
    */
   public static void exportFile(View view, String fileNameHint, String[] fileExtensions, String[] fileExtensionNames, String mimeType, ExportDataProcessor processor)
   {
      FileDialog dlg = new FileDialog((view != null) ? view.getWindow().getShell() : Display.getCurrent().getActiveShell(), SWT.SAVE);
      if (fileNameHint != null)
         dlg.setFileName(fileNameHint);
      dlg.setFilterExtensions(fileExtensions);
      dlg.setFilterNames(fileExtensionNames);
      dlg.setOverwrite(true);
      final String fileName = dlg.open();
      if (fileName == null)
         return;

      Job job = new Job("Exporting data", view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            processor.export(fileName);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot export data to file";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Check if system theme is a dark one.
    *
    * @return true if system theme is a dark one
    */
   public static boolean isSystemDarkTheme()
   {
      return isSystemDarkTheme(Display.getCurrent());
   }

   /**
    * Check if system theme is a dark one.
    *
    * @param display display to use
    * @return true if system theme is a dark one
    */
   public static boolean isSystemDarkTheme(Display display)
   {
      boolean darkThemeDetected = ColorConverter.isDarkColor(display.getSystemColor(SWT.COLOR_WIDGET_BACKGROUND).getRGB());
      return darkThemeDetected; // Do not use Display.isSystemDarkTheme() as may report true on Windows whily all controls are still using light theme
   }

   /**
    * Create popup calendar and call provided callback on selection.
    *
    * @param shell parent shell
    * @param anchor anchor control for positioning popup
    * @param initialDate initial date for calendar (can be null)
    * @param callback callback to be called when user selects date
    */
   public static void createPopupCalendar(Shell shell, Control anchor, Date initialDate, Consumer<Calendar> callback)
   {
      final Shell popup = new Shell(shell, SWT.NO_TRIM | SWT.ON_TOP);
      final DateTime dateSelector = new DateTime(popup, SWT.CALENDAR | SWT.SHORT);

      Point size = dateSelector.computeSize(SWT.DEFAULT, SWT.DEFAULT);
      popup.setSize(size);
      dateSelector.setSize(size);

      Calendar c = Calendar.getInstance();
      if (initialDate != null)
         c.setTime(initialDate);
      dateSelector.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));

      dateSelector.addFocusListener(new FocusListener() {
         @Override
         public void focusLost(FocusEvent e)
         {
            popup.close();
         }

         @Override
         public void focusGained(FocusEvent e)
         {
         }
      });

      dateSelector.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            Calendar c = Calendar.getInstance();
            c.set(dateSelector.getYear(), dateSelector.getMonth(), dateSelector.getDay(), dateSelector.getHours(), dateSelector.getMinutes(), dateSelector.getSeconds());
            callback.accept(c);
            popup.close();
         }
      });

      Rectangle rect = anchor.getDisplay().map(anchor.getParent(), null, anchor.getBounds());
      popup.setLocation(rect.x, rect.y + rect.height);
      popup.open();
   }

   /**
    * Create browser widget and optionally set initial location. Returns null if browser cannot be created.
    *
    * @param parent parent composite
    * @param style browser widget style
    * @param url initial location (can be null)
    * @return browser widget or null if browser cannot be instantiated
    */
   public static Browser createBrowser(Composite parent, int style, String url)
   {
      try
      {
         Browser browser = new Browser(parent, style | SWT.EDGE);
         if (url != null)
            browser.setUrl(url);
         return browser;
      }
      catch(Throwable t)
      {
         logger.error("Cannot create browser widget", t);
         return null;
      }
   }
}
