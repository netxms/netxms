/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectSelectionFilterFactory;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating multiple nodes at once (e.g. from ARP cache).
 * Asks only for parent container and zone; all other settings use defaults.
 */
public class CreateMultipleNodesDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateMultipleNodesDialog.class);

   private NXCSession session;
   private ObjectSelector parentSelector;
   private ZoneSelector zoneSelector;

   private long parentId;
   private int zoneUIN;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param defaultParentId initial parent object id (0 if unknown)
    * @param defaultZoneUIN initial zone UIN
    */
   public CreateMultipleNodesDialog(Shell parentShell, long defaultParentId, int defaultZoneUIN)
   {
      super(parentShell);
      session = Registry.getSession();
      this.parentId = defaultParentId;
      this.zoneUIN = defaultZoneUIN;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Nodes"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      parentSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
      parentSelector.setLabel(i18n.tr("Parent"));
      parentSelector.setClassFilter(ObjectSelectionFilterFactory.getInstance().createContainerSelectionFilter());
      parentSelector.setObjectId(parentId);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      parentSelector.setLayoutData(gd);

      if (session.isZoningEnabled())
      {
         zoneSelector = new ZoneSelector(dialogArea, SWT.NONE, false);
         zoneSelector.setLabel(i18n.tr("Zone"));
         zoneSelector.setZoneUIN(zoneUIN);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         zoneSelector.setLayoutData(gd);
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      parentId = parentSelector.getObjectId();
      if (parentId == 0)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select parent object."));
         return;
      }
      if (session.isZoningEnabled())
         zoneUIN = zoneSelector.getZoneUIN();
      super.okPressed();
   }

   /**
    * @return selected parent object id
    */
   public long getParentId()
   {
      return parentId;
   }

   /**
    * @return selected zone UIN
    */
   public int getZoneUIN()
   {
      return zoneUIN;
   }
}
