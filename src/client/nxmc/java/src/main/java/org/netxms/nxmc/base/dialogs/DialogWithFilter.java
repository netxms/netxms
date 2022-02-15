/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.widgets.FilterText;

/**
 * Dialog with filter input capability
 */
public abstract class DialogWithFilter extends Dialog
{
   private FilterText filterText;
   private AbstractViewerFilter filter;
   private StructuredViewer viewer;

   protected DialogWithFilter(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      // create the top level composite for the dialog
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      Composite composite = new Composite(parent, SWT.NONE);
      composite.setLayout(layout);
      composite.setLayoutData(new GridData(GridData.FILL_BOTH));
      applyDialogFont(composite);
      // initialize the dialog units
      initializeDialogUnits(composite);
      // create the dialog area and button bar

      filterText = new FilterText(composite, SWT.NONE, null, false);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);      
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });    

      dialogArea = createDialogArea(composite);
      dialogArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      buttonBar = createButtonBar(composite);
      filterText.setFocus();

      return composite;
   }

   /**
    * Handler for filter modification
    */
   protected void onFilterModify()
   {
      if ((filter != null) && (viewer != null))
         defaultOnFilterModify();
   }

   /**
    * Set viewer and filter that will be managed by default filter string handler
    * 
    * @param viewer viewer to refresh
    * @param filter filter to set filtering text
    */
   protected void setFilterClient(StructuredViewer viewer, AbstractViewerFilter filter)
   {
      this.viewer = viewer;
      this.filter = filter;
   }

   /**
    * Default on modify for view with viewer
    */
   private void defaultOnFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }
}
