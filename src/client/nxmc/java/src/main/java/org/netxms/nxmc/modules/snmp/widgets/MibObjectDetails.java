/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.snmp.widgets;

import java.util.function.Consumer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.SnmpConstants;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Widget which shows details of MIB object
 */
public class MibObjectDetails extends Composite
{
   private I18n i18n = LocalizationHelper.getI18n(MibObjectDetails.class);
   
	private Text oid;
	private Text oidText;
	private Text description;
   private Text textualConvention;
   private Text index;
	private Text type;
	private Text status;
	private Text access;
	private MibBrowser mibTree;
	private boolean updateObjectId = true;
   private boolean updateTreeSelection = true;

	/**
	 * Create MIB object details widget.
	 * 
	 * @param parent parent composite
	 * @param style standard SWT styles for widget
	 * @param showOID if true, "Object ID" input field will be shown
	 * @param mibTree associated MIB tree
	 * @param mibExplorer 
	 */
	public MibObjectDetails(Composite parent, int style, boolean showOID, MibBrowser mibTree, Consumer<String> consumer)
	{
		super(parent, style);

		this.mibTree = mibTree;

		GridLayout layout = new GridLayout();
		setLayout(layout);

		if (showOID)
		{
	      /* MIB object information: status, type, etc. */
	      Composite oidGroup = new Composite(this, SWT.NONE);
	      layout = new GridLayout();
	      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
	      layout.marginHeight = 0;
	      layout.marginWidth = 0;
	      layout.numColumns = 2;
	      layout.makeColumnsEqualWidth = false;
	      oidGroup.setLayout(layout);
	      GridData gd = new GridData();
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      oidGroup.setLayoutData(gd);
	      
         oid = WidgetHelper.createLabeledText(oidGroup, SWT.BORDER, 500, i18n.tr("Object identifier (OID)"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
			oid.addModifyListener(new ModifyListener() {
				@Override
				public void modifyText(ModifyEvent e)
				{
               if (updateTreeSelection)
                  onManualOidChange();
				}
			});
			
         Button button = new Button(oidGroup, SWT.PUSH);
         button.setText(i18n.tr("&Walk..."));
         gd = new GridData();
         gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
         gd.verticalAlignment = SWT.BOTTOM;
         button.setLayoutData(gd);
         button.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               if (consumer != null)
                  consumer.accept(oid.getText());
            }
         });

         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessVerticalSpace = true;
         gd.verticalAlignment = SWT.FILL;
         gd.horizontalSpan = 2;
         oidText = WidgetHelper.createLabeledText(oidGroup, SWT.BORDER, 500, i18n.tr("OID as text"), "", gd);
			oidText.setEditable(false);
		}
		else
		{
			oid = null;
		}

		/* MIB object information: status, type, etc. */
		Composite infoGroup = new Composite(this, SWT.NONE);
		layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 3;
		layout.makeColumnsEqualWidth = true;
		infoGroup.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		infoGroup.setLayoutData(gd);

      type = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, i18n.tr("Type"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      status = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, i18n.tr("Status"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      access = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, i18n.tr("Access"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      index = WidgetHelper.createLabeledText(this, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, i18n.tr("Index"), "", gd);

		/* MIB object's description */
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		description = WidgetHelper.createLabeledText(this, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL | SWT.READ_ONLY,
		                                             500, i18n.tr("Description"), "", gd); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.heightHint = 150;
		description.setLayoutData(gd);

		/* MIB object's textual convention */
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		textualConvention = WidgetHelper.createLabeledText(this, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL | SWT.READ_ONLY,
		                                                   500, i18n.tr("Textual Convention"), "", gd); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.heightHint = 150;
		textualConvention.setLayoutData(gd);
	}

   /**
    * Programmatically set OID. Will cause same handling as if new OID entered in editor field.
    *
    * @param oidText new OID as text
    */
	public void setOid(String oidText)
	{
	   oid.setText(oidText);
	   onManualOidChange();
	}

	/**
	 * Handler for manual OID change
	 */
	private void onManualOidChange()
	{
		MibObject o = MibCache.findObject(oid.getText(), false);
		if ((o != null) && (mibTree != null))
		{
			updateObjectId = false;
			mibTree.setSelection(o);
			updateObjectId = true;
		}
	}

	/**
	 * Set MIB object to show
	 * 
	 * @param object MIB object
	 */
	public void setObject(MibObject object)
	{
		if (object != null)
		{
			if ((oid != null) && updateObjectId)
			{
            SnmpObjectId objectId = object.getObjectId();
            updateTreeSelection = false;
            oid.setText((objectId != null) ? objectId.toString() : "");
            updateTreeSelection = true;
			}
			if (oidText != null)
			{
				oidText.setText(object.getFullName());
			}
         index.setText(object.getIndex());
			description.setText(object.getDescription());
			textualConvention.setText(object.getTextualConvention());
			type.setText(SnmpConstants.getObjectTypeName(object.getType()));
			status.setText(SnmpConstants.getObjectStatusName(object.getStatus()));
			access.setText(SnmpConstants.getObjectAccessName(object.getAccess()));
		}
		else
		{
			if (oid != null)
            oid.setText("");
			if (oidText != null)
            oidText.setText("");
         index.setText("");
         description.setText("");
         textualConvention.setText("");
         type.setText("");
         status.setText("");
         access.setText("");
		}
	}
}
