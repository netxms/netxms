/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.ui.eclipse.datacollection.api.SummaryTablesCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Object tool selection dialog
 */
public class SummaryTableSelectionDialog extends Dialog
{
   private TableViewer viewer;
   private List<DciSummaryTableDescriptor> selection;
   
   /**
    * @param parentShell
    */
   public SummaryTableSelectionDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select DCI Summary Table");
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      final Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);
      
      viewer = new TableViewer(dialogArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((DciSummaryTableDescriptor)element).getTitle() + " (" + ((DciSummaryTableDescriptor)element).getMenuPath().replace("&", "") + ")";
         }
      });
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            String n1 = ((DciSummaryTableDescriptor)e1).getTitle() + " (" + ((DciSummaryTableDescriptor)e1).getMenuPath().replace("&", "") + ")";
            String n2 = ((DciSummaryTableDescriptor)e2).getTitle() + " (" + ((DciSummaryTableDescriptor)e2).getMenuPath().replace("&", "") + ")";
            return n1.compareToIgnoreCase(n2);
         }
      });
      viewer.setInput(SummaryTablesCache.getInstance().getTables());
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @SuppressWarnings("unchecked")
   @Override
   protected void okPressed()
   {
      selection = ((IStructuredSelection)viewer.getSelection()).toList();
      super.okPressed();
   }

   /**
    * @return the selection
    */
   public List<DciSummaryTableDescriptor> getSelection()
   {
      return selection;
   }
}
