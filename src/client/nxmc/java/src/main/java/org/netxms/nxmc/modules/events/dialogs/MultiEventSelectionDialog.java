/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.events.dialogs;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.jface.viewers.ICheckStateProvider;
import org.eclipse.jface.viewers.ITableFontProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting multiple events with checkbox support and initial selection.
 */
public class MultiEventSelectionDialog extends Dialog
{
   private static final String CONFIG_PREFIX = "MultiEventSelectionDialog";

   private final I18n i18n = LocalizationHelper.getI18n(MultiEventSelectionDialog.class);

   private NXCSession session;
   private FilterText filterTextWidget;
   private CheckboxTableViewer viewer;
   private List<SelectableEvent> events = new ArrayList<>();
   private Set<Integer> initialSelection;
   private int[] selectedEventCodes;
   private String filterText = "";

   /**
    * Create multi-event selection dialog.
    *
    * @param parentShell parent shell
    * @param initialSelection array of initially selected event codes (can be null)
    */
   public MultiEventSelectionDialog(Shell parentShell, int[] initialSelection)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      session = Registry.getSession();

      this.initialSelection = new HashSet<>();
      if (initialSelection != null)
      {
         for(int code : initialSelection)
            this.initialSelection.add(code);
      }
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Events"));
      PreferenceStore settings = PreferenceStore.getInstance();
      int cx = settings.getAsInteger(CONFIG_PREFIX + ".cx", 0);
      int cy = settings.getAsInteger(CONFIG_PREFIX + ".cy", 0);
      if ((cx > 0) && (cy > 0))
      {
         newShell.setSize(cx, cy);
      }
      else
      {
         newShell.setSize(600, 500);
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      filterTextWidget = new FilterText(dialogArea, SWT.NONE, null, false, false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filterTextWidget.setLayoutData(gd);
      filterTextWidget.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filterText = filterTextWidget.getText().toLowerCase();
            viewer.refresh();
         }
      });

      viewer = CheckboxTableViewer.newCheckList(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.V_SCROLL | SWT.H_SCROLL);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      viewer.getTable().setLayoutData(gd);
      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);

      TableColumn colCode = new TableColumn(viewer.getTable(), SWT.LEFT);
      colCode.setText(i18n.tr("Code"));
      colCode.setWidth(100);

      TableColumn colName = new TableColumn(viewer.getTable(), SWT.LEFT);
      colName.setText(i18n.tr("Name"));
      colName.setWidth(250);

      TableColumn colTags = new TableColumn(viewer.getTable(), SWT.LEFT);
      colTags.setText(i18n.tr("Tags"));
      colTags.setWidth(200);

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new SelectableEventLabelProvider());
      viewer.setCheckStateProvider(new ICheckStateProvider() {
         @Override
         public boolean isGrayed(Object element)
         {
            return false;
         }

         @Override
         public boolean isChecked(Object element)
         {
            return ((SelectableEvent)element).selected;
         }
      });
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer v, Object e1, Object e2)
         {
            return ((SelectableEvent)e1).event.getName().compareToIgnoreCase(((SelectableEvent)e2).event.getName());
         }
      });
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer v, Object parentElement, Object element)
         {
            if (filterText.isEmpty())
               return true;
            SelectableEvent se = (SelectableEvent)element;
            return se.event.getName().toLowerCase().contains(filterText) ||
                   String.valueOf(se.event.getCode()).contains(filterText) ||
                   se.event.getTagList().toLowerCase().contains(filterText);
         }
      });
      viewer.addCheckStateListener(new ICheckStateListener() {
         @Override
         public void checkStateChanged(CheckStateChangedEvent event)
         {
            ((SelectableEvent)event.getElement()).selected = event.getChecked();
            viewer.update(event.getElement(), null); // Refresh to update bold font
         }
      });
      viewer.addDoubleClickListener(event -> {
         Object selection = ((org.eclipse.jface.viewers.IStructuredSelection)event.getSelection()).getFirstElement();
         if (selection != null)
         {
            boolean currentState = ((SelectableEvent)selection).selected;
            ((SelectableEvent)selection).selected = !currentState;
            viewer.setChecked(selection, !currentState);
            viewer.update(selection, null); // Refresh to update bold font
         }
      });

      viewer.getTable().addDisposeListener((e) -> saveSettings());

      loadEventTemplates();

      return dialogArea;
   }

   /**
    * Load event templates from server.
    */
   private void loadEventTemplates()
   {
      new Job(i18n.tr("Loading event templates"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<EventTemplate> templates = session.getEventTemplates();
            runInUIThread(() -> {
               if (!viewer.getTable().isDisposed())
               {
                  events.clear();
                  for(EventTemplate t : templates)
                  {
                     events.add(new SelectableEvent(t, initialSelection.contains(t.getCode())));
                  }
                  viewer.setInput(events);
                  packColumns();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load event templates");
         }
      }.start();
   }

   /**
    * Pack viewer columns
    */
   private void packColumns()
   {
      Table table = viewer.getTable();
      int count = table.getColumnCount();
      for(int i = 0; i < count; i++)
      {
         TableColumn c = table.getColumn(i);
         if (c.getResizable())
            c.pack();
      }
      if (!Registry.IS_WEB_CLIENT)
         viewer.getControl().redraw(); // Fixes display glitch on Windows
   }

   /**
    * Save dialog settings.
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x);
      settings.set(CONFIG_PREFIX + ".cy", size.y);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      List<Integer> selected = new ArrayList<>();
      for(SelectableEvent se : events)
      {
         if (se.selected)
            selected.add(se.event.getCode());
      }

      selectedEventCodes = new int[selected.size()];
      for(int i = 0; i < selected.size(); i++)
         selectedEventCodes[i] = selected.get(i);

      super.okPressed();
   }

   /**
    * Get selected event codes.
    *
    * @return array of selected event codes
    */
   public int[] getSelectedEventCodes()
   {
      return selectedEventCodes;
   }

   /**
    * Get selected event templates.
    *
    * @return array of selected event templates
    */
   public EventTemplate[] getSelectedEvents()
   {
      List<EventTemplate> selected = new ArrayList<>();
      for(SelectableEvent se : events)
      {
         if (se.selected)
            selected.add(se.event);
      }
      return selected.toArray(new EventTemplate[0]);
   }

   /**
    * Wrapper class to track selection state of event templates.
    */
   private static class SelectableEvent
   {
      EventTemplate event;
      boolean selected;

      SelectableEvent(EventTemplate event, boolean selected)
      {
         this.event = event;
         this.selected = selected;
      }
   }

   /**
    * Label provider for selectable event list.
    */
   private static class SelectableEventLabelProvider extends LabelProvider implements ITableLabelProvider, ITableFontProvider
   {
      private Font boldFont;

      /**
       * Create label provider.
       */
      SelectableEventLabelProvider()
      {
         FontData fd = JFaceResources.getDefaultFont().getFontData()[0];
         fd.setStyle(SWT.BOLD);
         boldFont = new Font(Display.getDefault(), fd);
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         if (columnIndex != 0)
            return null;

         EventTemplate event = ((SelectableEvent)element).event;
         return StatusDisplayInfo.getStatusImage(event.getSeverity());
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         EventTemplate event = ((SelectableEvent)element).event;
         switch(columnIndex)
         {
            case 0:
               return Integer.toString(event.getCode());
            case 1:
               return event.getName();
            case 2:
               return event.getTagList();
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableFontProvider#getFont(java.lang.Object, int)
       */
      @Override
      public Font getFont(Object element, int columnIndex)
      {
         return ((SelectableEvent)element).selected ? boldFont : null;
      }

      /**
       * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
       */
      @Override
      public void dispose()
      {
         boldFont.dispose();
         super.dispose();
      }
   }
}
