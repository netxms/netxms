/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * SNMP specific configuration
 */
public class SNMP extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(SNMP.class);
   private static final String[] snmpRawTypes = 
   { 
      LocalizationHelper.getI18n(SNMP.class).tr("None"), 
      LocalizationHelper.getI18n(SNMP.class).tr("32-bit signed integer"), 
      LocalizationHelper.getI18n(SNMP.class).tr("32-bit unsigned integer"),
      LocalizationHelper.getI18n(SNMP.class).tr("64-bit signed integer"), 
      LocalizationHelper.getI18n(SNMP.class).tr("64-bit unsigned integer"),
      LocalizationHelper.getI18n(SNMP.class).tr("Floating point number"), 
      LocalizationHelper.getI18n(SNMP.class).tr("IPv4 address"),
      LocalizationHelper.getI18n(SNMP.class).tr("MAC address"),
      LocalizationHelper.getI18n(SNMP.class).tr("IPv6 address")
   };
   
	private DataCollectionObject dco;
	private Button checkInterpretRawSnmpValue;
   private Combo snmpRawType;
   private Button checkUseCustomSnmpPort;
   private Spinner customSnmpPort;
   private Button checkUseCustomSnmpVersion;
   private Combo customSnmpVersion;
   
   /**
    * Create page.
    *
    * @param editor editor object
    */
   public SNMP(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(SNMP.class).tr("SNMP"), editor);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite pageArea = (Composite)super.createContents(parent);
		dco = editor.getObject();

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      pageArea.setLayout(layout);

      GridData gd = null;
      if (dco instanceof DataCollectionItem)
      {
         checkInterpretRawSnmpValue = new Button(pageArea, SWT.CHECK);
         checkInterpretRawSnmpValue.setText(i18n.tr("Interpret SNMP octet string raw value as"));
         checkInterpretRawSnmpValue.setSelection(((DataCollectionItem)dco).isSnmpRawValueInOctetString());
         checkInterpretRawSnmpValue.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               snmpRawType.setEnabled(checkInterpretRawSnmpValue.getSelection());
            }
         });

         snmpRawType = new Combo(pageArea, SWT.BORDER | SWT.READ_ONLY);
         for(int i = 0; i < snmpRawTypes.length; i++)
            snmpRawType.add(snmpRawTypes[i]);
         snmpRawType.select(((DataCollectionItem)dco).getSnmpRawValueType());
         snmpRawType.setEnabled((dco.getOrigin() == DataOrigin.SNMP) && ((DataCollectionItem)dco).isSnmpRawValueInOctetString());
         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         snmpRawType.setLayoutData(gd);
      }

      checkUseCustomSnmpPort = new Button(pageArea, SWT.CHECK);
      checkUseCustomSnmpPort.setText(i18n.tr("Use custom SNMP port:"));
      checkUseCustomSnmpPort.setSelection(dco.getSnmpPort() != 0);
      checkUseCustomSnmpPort.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            customSnmpPort.setEnabled(checkUseCustomSnmpPort.getSelection());
         }
      });

      customSnmpPort = new Spinner(pageArea, SWT.BORDER);
      customSnmpPort.setMinimum(1);
      customSnmpPort.setMaximum(65535);
      if ((dco.getOrigin() == DataOrigin.SNMP) && (dco.getSnmpPort() != 0))
      {
         customSnmpPort.setEnabled(true);
         customSnmpPort.setSelection(dco.getSnmpPort());
      }
      else
      {
         customSnmpPort.setEnabled(false);
      }
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      customSnmpPort.setLayoutData(gd);

      checkUseCustomSnmpVersion = new Button(pageArea, SWT.CHECK);
      checkUseCustomSnmpVersion.setText("Use custom SNMP version:");
      checkUseCustomSnmpVersion.setSelection(dco.getSnmpVersion() != SnmpVersion.DEFAULT);
      checkUseCustomSnmpVersion.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            customSnmpVersion.setEnabled(checkUseCustomSnmpVersion.getSelection());
         }
      });

      customSnmpVersion = new Combo(pageArea, SWT.BORDER | SWT.READ_ONLY);
      customSnmpVersion.add("1");
      customSnmpVersion.add("2c");
      customSnmpVersion.add("3");
      customSnmpVersion.select(indexFromSnmpVersion(dco.getSnmpVersion()));
      customSnmpVersion.setEnabled(dco.getSnmpVersion() != SnmpVersion.DEFAULT);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      customSnmpVersion.setLayoutData(gd);
      
		return pageArea;
	}

   /**
    * Calculate selection index from SNMP version
    * 
    * @param version
    * @return
    */
   private static int indexFromSnmpVersion(SnmpVersion version)
   {
      switch(version)
      {
         case V1:
            return 0;
         case V2C:
            return 1;
         case V3:
            return 2;
         default:
            return -1;
      }
   }

   /**
    * Get SNMP version value from selection index
    * 
    * @param index
    * @return
    */
   private static SnmpVersion indexToSnmpVersion(int index)
   {
      switch(index)
      {
         case 0:
            return SnmpVersion.V1;
         case 1:
            return SnmpVersion.V2C;
         case 2:
            return SnmpVersion.V3;
         default:
            return SnmpVersion.DEFAULT;
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{    
      if (dco instanceof DataCollectionItem)
      {
   	   ((DataCollectionItem)dco).setSnmpRawValueInOctetString(checkInterpretRawSnmpValue.getSelection());
         ((DataCollectionItem)dco).setSnmpRawValueType(snmpRawType.getSelectionIndex());
      }
      if (checkUseCustomSnmpPort.getSelection())
      {
         dco.setSnmpPort(customSnmpPort.getSelection());
      }
      else
      {
         dco.setSnmpPort(0);
      }
      if (checkUseCustomSnmpVersion.getSelection())
      {
         dco.setSnmpVersion(indexToSnmpVersion(customSnmpVersion.getSelectionIndex()));
      }
      else
      {
         dco.setSnmpVersion(SnmpVersion.DEFAULT);
      }
		editor.modify();
		
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
      if (dco instanceof DataCollectionItem)
      {
         checkInterpretRawSnmpValue.setSelection(false);
      }
      checkUseCustomSnmpPort.setSelection(false);
      customSnmpPort.setSelection(161);
	}
}
