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
	private Button checkMatchNodeOS;
	private Button checkMatchWorkstationOS;
	private Button checkMatchTemplate;
	private Text textOID;
	private Text textNodeOS;
	private Text textWorkstationOS;
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
		
		checkMatchNodeOS = new Button(dialogArea, SWT.CHECK);
		checkMatchNodeOS.setText(Messages.get().Filter_OSShouldMatch);
		checkMatchNodeOS.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_NODE_OS_MATCH) != 0);
		checkMatchNodeOS.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textNodeOS.setEnabled(checkMatchNodeOS.getSelection());
				if (checkMatchNodeOS.getSelection())
					textNodeOS.setFocus();
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		textNodeOS = new Text(dialogArea, SWT.BORDER);
		textNodeOS.setText(objectTool.getToolNodeOS());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textNodeOS.setLayoutData(gd);
		textNodeOS.setEnabled(checkMatchNodeOS.getSelection());
		
		if (objectTool.getType() == ObjectTool.TYPE_LOCAL_COMMAND)
		{
   		checkMatchWorkstationOS = new Button(dialogArea, SWT.CHECK);
   		checkMatchWorkstationOS.setText("Workstation OS name should match this template(coma separated regular expression list)");
   		checkMatchWorkstationOS.setSelection((objectTool.getFlags() & ObjectTool.REQUIRES_WORKSTATION_OS_MATCH) != 0);
   		checkMatchWorkstationOS.addSelectionListener(new SelectionListener() {         
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               textWorkstationOS.setEnabled(checkMatchWorkstationOS.getSelection());
               if (checkMatchWorkstationOS.getSelection())
                  textWorkstationOS.setFocus();            
            }
            
            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);            
            }
         });
   		
   		textWorkstationOS = new Text(dialogArea, SWT.BORDER);
   		textWorkstationOS.setText(objectTool.getToolWorkstationOS());
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalIndent = 20;
         textWorkstationOS.setLayoutData(gd);
         textWorkstationOS.setEnabled(checkMatchWorkstationOS.getSelection());
		}
		
		checkMatchTemplate = new Button(dialogArea, SWT.CHECK);
		checkMatchTemplate.setText(Messages.get().Filter_TemplateShouldMatch);
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

      if (checkMatchNodeOS.getSelection())
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_NODE_OS_MATCH);
      else
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_NODE_OS_MATCH);
      
      objectTool.setToolNodeOS(textNodeOS.getText());
      
      if ((checkMatchWorkstationOS != null) && checkMatchWorkstationOS.getSelection())
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.REQUIRES_WORKSTATION_OS_MATCH);
      else
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.REQUIRES_WORKSTATION_OS_MATCH);
      
      if (textWorkstationOS != null)
         objectTool.setToolWorkstationOS(textWorkstationOS.getText());

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
