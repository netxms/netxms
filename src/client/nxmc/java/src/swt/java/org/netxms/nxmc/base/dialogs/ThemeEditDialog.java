/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.dialogs;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.resources.Theme;
import org.netxms.nxmc.resources.ThemeElement;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Theme editing dialog
 */
public class ThemeEditDialog extends Dialog
{
   public static final int COLUMN_TAG = 0;
   public static final int COLUMN_FOREGROUND = 1;
   public static final int COLUMN_BACKGROUND = 2;
   public static final int COLUMN_FONT = 3;

   private Theme theme;
   private LabeledText name;
   private TableViewer viewer;
   private ColorCache colorCache;
   private Map<String, ThemeElement> changedElements = new HashMap<String, ThemeElement>();

   /**
    * @param parentShell
    */
   public ThemeEditDialog(Shell parentShell, Theme theme)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.theme = theme;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Edit Theme");
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger("ThemeEditDialog.cx", 670), settings.getAsInteger("ThemeEditDialog.cy", 600));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      colorCache = new ColorCache(dialogArea);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel("Name");
      name.setText(theme.getName());
      name.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Theme elements");
      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      label.setLayoutData(gd);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      setupViewer();
      viewer.setInput(theme.getTags());
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.getTable().addListener(SWT.PaintItem, new Listener() {
         @Override
         public void handleEvent(Event event)
         {
            if ((event.index == COLUMN_FOREGROUND) || (event.index == COLUMN_BACKGROUND))
               drawColorCell(event, event.index);
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editElement();
         }
      });

      return dialogArea;
   }

   /**
    * Setup viewer
    */
   private void setupViewer()
   {
      TableColumn column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("Tag");
      column.setWidth(400);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("FG");
      column.setWidth(50);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("BG");
      column.setWidth(50);

      column = new TableColumn(viewer.getTable(), SWT.LEFT);
      column.setText("Font");
      column.setWidth(150);

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ColorListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((String)e1).compareToIgnoreCase((String)e2);
         }
      });

      viewer.getTable().setLinesVisible(true);
      viewer.getTable().setHeaderVisible(true);
   }

   /**
    * Draw color cell
    *
    * @param event
    * @param column
    */
   private void drawColorCell(Event event, int column)
   {
      TableItem item = (TableItem)event.item;
      String tag = (String)item.getData();

      ThemeElement e = changedElements.containsKey(tag) ? changedElements.get(tag) : theme.getElement((String)tag);
      if (e == null)
         return;

      RGB rgb = (column == COLUMN_FOREGROUND) ? e.foreground : e.background;
      if (rgb == null)
         return;

      int width = viewer.getTable().getColumn(column).getWidth();
      Color color = colorCache.create(rgb);
      event.gc.setForeground(colorCache.create(0, 0, 0));
      event.gc.setBackground(color);
      event.gc.setLineWidth(1);
      event.gc.fillRectangle(event.x + 3, event.y + 2, width - 7, event.height - 5);
      event.gc.drawRectangle(event.x + 3, event.y + 2, width - 7, event.height - 5);
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set("ThemeEditDialog.cx", size.x);
      settings.set("ThemeEditDialog.cy", size.y);
   }

   /**
    * Edit selected element
    */
   private void editElement()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      String tag = (String)selection.getFirstElement();
      ThemeElement element = new ThemeElement(changedElements.containsKey(tag) ? changedElements.get(tag) : theme.getElement(tag));
      ThemeElementEditDialog dlg = new ThemeElementEditDialog(getShell(), element);
      if (dlg.open() == Window.OK)
      {
         changedElements.put(tag, element);
         viewer.update(tag, null);
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      saveSettings();
      theme.setName(name.getText().trim());
      for(Entry<String, ThemeElement> e : changedElements.entrySet())
         theme.setElement(e.getKey(), e.getValue());
      super.okPressed();
   }

   /**
    * Label provider for color list
    */
   private class ColorListLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         switch(columnIndex)
         {
            case COLUMN_TAG:
               return (String)element;
            case COLUMN_FONT:
               ThemeElement te = changedElements.containsKey(element) ? changedElements.get(element) : theme.getElement((String)element);
               if ((te.fontName == null) || te.fontName.isEmpty())
                  return "";
               return te.fontName + " " + Integer.toString(te.fontHeight) + "pt";
            default:
               return "";
         }
      }
   }
}
