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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
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
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.propertypages.helpers.RackPassiveElementComparator;
import org.netxms.nxmc.modules.objects.propertypages.helpers.RackPassiveElementLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Patch panel selection dialog
 */
public class PatchPanelSelectonDialog extends Dialog
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_POSITION = 2;
   public static final int COLUMN_ORIENTATION = 3;
   
   private static final String CONFIG_PREFIX = "PatchPanelSelectonDialog";

   private I18n i18n = LocalizationHelper.getI18n(PatchPanelSelectonDialog.class);
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

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Position"), i18n.tr("Orientation") };
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
      newShell.setText(i18n.tr("Select Patch Panel"));

      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger(CONFIG_PREFIX + ".cx", 400), settings.getAsInteger(CONFIG_PREFIX + ".cy", 200));
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
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please select element from the list"));
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
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x);
      settings.set(CONFIG_PREFIX + ".cy", size.y);
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
