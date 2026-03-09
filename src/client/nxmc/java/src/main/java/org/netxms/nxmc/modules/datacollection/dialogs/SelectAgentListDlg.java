/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.AgentList;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting agent list (enumeration)
 */
public class SelectAgentListDlg extends Dialog
{
   private static final int COLUMN_DESCRIPTION = 0;
   private static final int COLUMN_NAME = 1;

   private final I18n i18n = LocalizationHelper.getI18n(SelectAgentListDlg.class);

   private long nodeId;
   private Text filterText;
   private SortableTableViewer viewer;
   private String filterString = "";
   private String selectedListName;

   /**
    * Constructor
    *
    * @param parentShell parent shell
    * @param nodeId node object ID
    */
   public SelectAgentListDlg(Shell parentShell, long nodeId)
   {
      super(parentShell);
      this.nodeId = nodeId;
      setShellStyle(getShellStyle() | SWT.RESIZE);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Agent List Selection"));
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsPoint("SelectAgentListDlg.size", 600, 400));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Available lists"));

      filterText = new Text(dialogArea, SWT.BORDER);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filterText.setLayoutData(gd);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filterString = filterText.getText().toLowerCase();
            viewer.refresh(false);
         }
      });

      final String[] columnNames = { i18n.tr("Description"), i18n.tr("Name") };
      final int[] columnWidths = { 350, 250 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, COLUMN_DESCRIPTION, SWT.UP, SWT.FULL_SELECTION | SWT.BORDER);
      WidgetHelper.restoreTableViewerSettings(viewer, "SelectAgentListDlg");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AgentListLabelProvider());
      viewer.setComparator(new AgentListComparator());
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return filterString.isEmpty() || ((AgentList)element).getName().toLowerCase().contains(filterString);
         }
      });

      viewer.getTable().addMouseListener(new MouseListener() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            okPressed();
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
         }
      });

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "SelectAgentListDlg");
         }
      });

      gd = new GridData();
      gd.heightHint = 250;
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      viewer.getControl().setLayoutData(gd);

      fillList();

      return dialogArea;
   }

   /**
    * Fill list with agent lists from server
    */
   private void fillList()
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Loading agent lists..."), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AgentList> lists = session.getSupportedLists(nodeId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(lists.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot retrieve list of supported agent lists");
         }
      }.start();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("You must select a list"));
         return;
      }
      selectedListName = ((AgentList)selection.getFirstElement()).getName();
      saveSettings();
      super.okPressed();
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
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set("SelectAgentListDlg.size", size);
   }

   /**
    * Get name of selected list
    *
    * @return selected list name
    */
   public String getSelectedListName()
   {
      return selectedListName;
   }

   /**
    * Label provider for agent list viewer
    */
   private class AgentListLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         AgentList list = (AgentList)element;
         switch(columnIndex)
         {
            case COLUMN_DESCRIPTION:
               return list.getDescription();
            case COLUMN_NAME:
               return list.getName();
         }
         return null;
      }
   }

   /**
    * Comparator for agent list viewer
    */
   private class AgentListComparator extends ViewerComparator
   {
      @Override
      public int compare(Viewer viewer, Object e1, Object e2)
      {
         AgentList l1 = (AgentList)e1;
         AgentList l2 = (AgentList)e2;

         int result;
         switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
         {
            case COLUMN_DESCRIPTION:
               result = l1.getDescription().compareToIgnoreCase(l2.getDescription());
               break;
            case COLUMN_NAME:
               result = l1.getName().compareToIgnoreCase(l2.getName());
               break;
            default:
               result = 0;
               break;
         }
         return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
      }
   }
}
