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
package org.netxms.ui.eclipse.objectview.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.PassiveRackElementType;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.dialogs.helpers.RackPassiveElementComparator;
import org.netxms.ui.eclipse.objectview.dialogs.helpers.RackPassiveElementLabelProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Patch panel selection dialog
 */
public class PatchPanelSelectonDialog extends Dialog
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_POSITION = 2;
   public static final int COLUMN_ORIENTATION = 3;
   
   private static final String CONFIG_PREFIX = "PatchPanel"; //$NON-NLS-1$

   private SortableTableViewer viewer;
   private List<PassiveRackElement> passiveElements;
   private long id; 
   
   public PatchPanelSelectonDialog(Shell parentShell, Rack rack)
   {
      super(parentShell);
      passiveElements = new ArrayList<PassiveRackElement>();
      for(PassiveRackElement el : rack.getPassiveElements())
         if (el.getType() == PassiveRackElementType.PATCH_PANEL)
            passiveElements.add(el);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      dialogArea.setLayout(new FillLayout());

      final String[] columnNames = { "Name", "Type", "Position", "Orientation" };
      final int[] columnWidths = { 150, 100, 70, 30 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new RackPassiveElementLabelProvider());
      viewer.setComparator(new RackPassiveElementComparator());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            okPressed();
         }
      });
      viewer.setInput(passiveElements.toArray());

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select Patch Panel");

      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      try
      {
         newShell.setSize(settings.getInt(CONFIG_PREFIX + ".cx"), settings.getInt(CONFIG_PREFIX + ".cy")); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(NumberFormatException e)
      {
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {      
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
      {
         MessageDialogHelper.openError(getShell(), "Error", "Please select element from the list");
         return;
      }
      final PassiveRackElement element = (PassiveRackElement)selection.getFirstElement();
      id = element.getId();
      super.okPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
   @Override
   protected void buttonPressed(int buttonId)
   {
      Point size = getShell().getSize();
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put(CONFIG_PREFIX + ".cx", size.x); //$NON-NLS-1$
      settings.put(CONFIG_PREFIX + ".cy", size.y); //$NON-NLS-1$
      super.buttonPressed(buttonId);
   }

   /**
    * Get selected patch panel id
    * @return patch panel id
    */
   public long getPatchPanelId()
   {
      return id;
   }
}
