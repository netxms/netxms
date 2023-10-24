/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.actions.dialogs;

import java.util.Collection;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.modules.actions.views.helpers.DecoratingActionLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Action selection dialog
 */
public class ActionSelectionDialog extends Dialog
{
   private Collection<ServerAction> localCache;
   private TableViewer viewer;
   private List<ServerAction> selection;

   /**
    * Action selection dialog construction
    * @param parentShell parent shell
    */
   public ActionSelectionDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * Action selection dialog construction
    * 
    * @param parentShell parent shell
    */
   public ActionSelectionDialog(Shell parentShell, Collection<ServerAction> localCache)
   {
      super(parentShell);
      this.localCache = localCache;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select action");
   }

   /**
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
      viewer.setLabelProvider(new DecoratingActionLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((ServerAction)e1).getName().compareToIgnoreCase(((ServerAction)e2).getName());
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            okPressed();
         }
      });

      if (localCache == null)
      {
         NXCSession session = Registry.getSession();
         new Job("Get server actions", null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final List<ServerAction> list = session.getActions();
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.setInput(list.toArray());
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot get server actions";
            }
         }.start();
      }
      else
      {
         viewer.setInput(localCache.toArray());
      }

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
      gd.minimumWidth = 500;
      viewer.getControl().setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @SuppressWarnings("unchecked")
   @Override
   protected void okPressed()
   {
      selection = viewer.getStructuredSelection().toList();
      super.okPressed();
   }

   /**
    * Get selection 
    * @return the selection
    */
   public List<ServerAction> getSelection()
   {
      return selection;
   }
}
