/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.snmp.propertypages;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.modules.snmp.dialogs.MibSelectionDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for SNMP trap configuration
 */
public class SnmpTrapGeneral extends PropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(SnmpTrapGeneral.class);
   private SnmpTrap trap;
   private Text description;
   private Text oid;
   private EventSelector event;
   private Text eventTag;
   private Button buttonSelect;

   /**
    * Create page
    * 
    * @param trap SNMP trap object to edit
    */
   public SnmpTrapGeneral(SnmpTrap trap)
   {
      super("General");
      noDefaultAndApplyButton();
      this.trap = trap;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      description = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, i18n.tr("Description"), trap.getDescription(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      Composite oidSelection = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      oidSelection.setLayout(layout);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      oidSelection.setLayoutData(gd);
      
      oid = WidgetHelper.createLabeledText(oidSelection, SWT.BORDER, 300, i18n.tr("Trap OID"), 
            (trap.getObjectId() != null) ? trap.getObjectId().toString() : "", WidgetHelper.DEFAULT_LAYOUT_DATA);

      buttonSelect = new Button(oidSelection, SWT.PUSH);
      buttonSelect.setText(i18n.tr("&Select..."));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.verticalAlignment = SWT.BOTTOM;
      buttonSelect.setLayoutData(gd);
      buttonSelect.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            selectObjectId();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });

      event = new EventSelector(dialogArea, SWT.NONE);
      event.setLabel(i18n.tr("Event"));
      event.setEventCode(trap.getEventCode());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      event.setLayoutData(gd);

      eventTag = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, SWT.DEFAULT, i18n.tr("Event tag"), trap.getEventTag(), WidgetHelper.DEFAULT_LAYOUT_DATA);

      return dialogArea;
   }

   /**
    * Select OID using MIB selection dialog
    */
   private void selectObjectId()
   {
      SnmpObjectId id;
      try
      {
         id = SnmpObjectId.parseSnmpObjectId(oid.getText());
      }
      catch(SnmpObjectIdFormatException e)
      {
         id = null;
      }
      MibSelectionDialog dlg = new MibSelectionDialog(getShell(), id, 0);
      if (dlg.open() == Window.OK)
      {
         oid.setText(dlg.getSelectedObjectId().toString());
         oid.setFocus();
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      try
      {
         trap.setObjectId(SnmpObjectId.parseSnmpObjectId(oid.getText()));
      }
      catch(SnmpObjectIdFormatException e)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("SNMP OID you have entered is invalid. Please enter correct SNMP OID."));
         return false;
      }

      trap.setDescription(description.getText());
      trap.setEventCode((int)event.getEventCode());
      trap.setEventTag(eventTag.getText());
      return true;
   }
}
