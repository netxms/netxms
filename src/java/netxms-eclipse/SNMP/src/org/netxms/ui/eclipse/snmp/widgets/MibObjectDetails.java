/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.SnmpConstants;
import org.netxms.ui.eclipse.snmp.shared.MibCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Widget which shows details of MIB object
 */
public class MibObjectDetails extends Composite
{
	private Text oid;
	private Text oidText;
	private Text description;
	private Text textualConvention;
	private Text type;
	private Text status;
	private Text access;
	private MibBrowser mibTree;
	private boolean updateObjectId = true;

	/**
	 * Create MIB object details widget.
	 * 
	 * @param parent parent composite
	 * @param style standard SWT styles for widget
	 * @param showOID if true, "Object ID" input field will be shown
	 * @param mibTree associated MIB tree
	 */
	public MibObjectDetails(Composite parent, int style, boolean showOID, MibBrowser mibTree)
	{
		super(parent, style);
		
		this.mibTree = mibTree;
		
		GridLayout layout = new GridLayout();
		setLayout(layout);
		
		if (showOID)
		{
			oid = WidgetHelper.createLabeledText(this, SWT.BORDER, 500, Messages.MibObjectDetails_OID, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
			oid.addModifyListener(new ModifyListener() {
				@Override
				public void modifyText(ModifyEvent e)
				{
					onManualOidChange();
				}
			});

			oidText = WidgetHelper.createLabeledText(this, SWT.BORDER, 500, "OID as text", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
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
		
		type = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, Messages.MibObjectDetails_Type, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
		status = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, Messages.MibObjectDetails_Status, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
		access = WidgetHelper.createLabeledText(infoGroup, SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, Messages.MibObjectDetails_Access, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
		
		/* MIB object's description */
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		description = WidgetHelper.createLabeledText(this, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL | SWT.READ_ONLY,
		                                             500, Messages.MibObjectDetails_8, "", gd); //$NON-NLS-1$
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
		                                                   500, "Textual Convention", "", gd);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.heightHint = 150;
		textualConvention.setLayoutData(gd);
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
				oid.setText((objectId != null) ? objectId.toString() : ""); //$NON-NLS-1$
			}
			if (oidText != null)
			{
				oidText.setText(object.getFullName());
			}
			description.setText(object.getDescription());
			textualConvention.setText(object.getTextualConvention());
			type.setText(SnmpConstants.getObjectTypeName(object.getType()));
			status.setText(SnmpConstants.getObjectStatusName(object.getStatus()));
			access.setText(SnmpConstants.getObjectAccessName(object.getAccess()));
		}
		else
		{
			if (oid != null)
				oid.setText(""); //$NON-NLS-1$
			if (oidText != null)
				oidText.setText(""); //$NON-NLS-1$
			description.setText(""); //$NON-NLS-1$
			textualConvention.setText(""); //$NON-NLS-1$
			type.setText(""); //$NON-NLS-1$
			status.setText(""); //$NON-NLS-1$
			access.setText(""); //$NON-NLS-1$
		}
	}
}
