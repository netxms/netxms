/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Filter" property page for object tool
 */
public class Filter extends PropertyPage
{
	private ObjectToolDetails objectTool;
	private Button checkAgent;
	private Button checkSNMP;
	private Button checkMatchOID;
	private Button checkMatchOS;
	private Button checkMatchTemplate;
	private Text textOID;
	private Text textOS;
	private Text textTemplate;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createControl(Composite parent)
	{
		noDefaultAndApplyButton();
		super.createControl(parent);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		objectTool = (ObjectToolDetails)getElement().getAdapter(ObjectToolDetails.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);
		
		checkAgent = new Button(dialogArea, SWT.CHECK);
		checkAgent.setText(Messages.get().Filter_AgentNeeded);
		checkAgent.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_AGENT) != 0);
		
		checkSNMP = new Button(dialogArea, SWT.CHECK);
		checkSNMP.setText(Messages.get().Filter_SNMPNeeded);
		checkSNMP.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_SNMP) != 0);
		
		checkMatchOID = new Button(dialogArea, SWT.CHECK);
		checkMatchOID.setText(Messages.get().Filter_OIDShouldMatch);
		checkMatchOID.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_OID_MATCH) != 0);
		checkMatchOID.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textOID.setEnabled(checkMatchOID.getSelection());
				if (checkMatchOID.getSelection())
					textOID.setFocus();
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		textOID = new Text(dialogArea, SWT.BORDER);
		textOID.setText(objectTool.getSnmpOid());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textOID.setLayoutData(gd);
		textOID.setEnabled(checkMatchOID.getSelection());
		
		checkMatchOS = new Button(dialogArea, SWT.CHECK);
		checkMatchOS.setText("System OS name should match this template(coma separated regular expression list):");
		checkMatchOS.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_OS_MATCH) != 0);
		checkMatchOS.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textOS.setEnabled(checkMatchOS.getSelection());
				if (checkMatchOS.getSelection())
					textOS.setFocus();
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		textOS = new Text(dialogArea, SWT.BORDER);
		textOS.setText(objectTool.getToolOS());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textOS.setLayoutData(gd);
		textOS.setEnabled(checkMatchOS.getSelection());
		
		checkMatchTemplate = new Button(dialogArea, SWT.CHECK);
		checkMatchTemplate.setText("Parent template name should match this template(coma separated regular expression list):");
		checkMatchTemplate.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_TEMPLATE_MATCH) != 0);
		checkMatchTemplate.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textTemplate.setEnabled(checkMatchTemplate.getSelection());
            if (checkMatchTemplate.getSelection())
               textTemplate.setFocus();
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
		textTemplate = new Text(dialogArea, SWT.BORDER);
		textTemplate.setText(objectTool.getToolTemplate());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = 20;
      textTemplate.setLayoutData(gd);
      textTemplate.setEnabled(checkMatchTemplate.getSelection());
		
		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (checkAgent.getSelection())
			objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_AGENT);
		else
			objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_AGENT);

		if (checkSNMP.getSelection())
			objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_SNMP);
		else
			objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_SNMP);

		if (checkMatchOID.getSelection())
			objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_OID_MATCH);
		else
			objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_OID_MATCH);
		
		objectTool.setSnmpOid(textOID.getText());		

      if (checkMatchOS.getSelection())
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_OS_MATCH);
      else
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_OS_MATCH);
      
      objectTool.setToolOS(textOS.getText());

      if (checkMatchTemplate.getSelection())
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_TEMPLATE_MATCH);
      else
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_TEMPLATE_MATCH);
      
      objectTool.setToolTemplate(textTemplate.getText());
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}
}
