/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.dialogs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.PlatformUI;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Perspective selection dialog
 */
public class SelectPerspectiveDialog extends Dialog
{
   private IPerspectiveDescriptor selectedPerspective = null;

   /**
    * @param parentShell
    */
   public SelectPerspectiveDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select Initial Perspective");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 10;
      layout.marginHeight = 20;
      layout.marginWidth = 30;
      dialogArea.setLayout(layout);

      List<String> allowedPerspectives;
      String v = ConsoleSharedData.getSession().getClientConfigurationHint("AllowedPerspectives");
      if ((v != null) && !v.isEmpty())
      {
         String[] parts = v.split(",");
         allowedPerspectives = new ArrayList<String>(parts.length);
         for(String s : parts)
         {
            if (!s.isBlank())
               allowedPerspectives.add(s.trim().toLowerCase());
         }
      }
      else
      {
         allowedPerspectives = null;
      }

      IPerspectiveDescriptor[] perspectives = PlatformUI.getWorkbench().getPerspectiveRegistry().getPerspectives();
      if (allowedPerspectives != null)
      {
         Arrays.sort(perspectives, new Comparator<IPerspectiveDescriptor>() {
            @Override
            public int compare(IPerspectiveDescriptor p1, IPerspectiveDescriptor p2)
            {
               return allowedPerspectives.indexOf(p1.getLabel().toLowerCase()) - allowedPerspectives.indexOf(p2.getLabel().toLowerCase());
            }
         });
      }

      for(final IPerspectiveDescriptor p : perspectives)
      {
         if ((allowedPerspectives != null) && !allowedPerspectives.contains(p.getLabel().toLowerCase()))
            continue;

         Button button = new Button(dialogArea, SWT.PUSH | SWT.FLAT);
         button.setImage(p.getImageDescriptor().createImage());
         button.setText(p.getLabel());
         button.setFont(JFaceResources.getBannerFont());
         button.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               selectedPerspective = p;
               okPressed();
            }
         });
         GridData gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         gd.widthHint = 500;
         button.setLayoutData(gd);
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
   }

   /**
    * @return the selectedPerspective
    */
   public IPerspectiveDescriptor getSelectedPerspective()
   {
      return selectedPerspective;
   }
}
