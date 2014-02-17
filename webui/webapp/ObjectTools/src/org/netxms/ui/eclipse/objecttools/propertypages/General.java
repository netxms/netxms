/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "General" property page for object tool
 */
public class General extends PropertyPage
{
	private ObjectToolDetails objectTool;
	private LabeledText textName;
	private LabeledText textDescription;
	private LabeledText textData;
	private LabeledSpinner maxFileSize;
	private LabeledText textParameter;
	private LabeledText textRegexp;
	private Button checkOutput;
	private Button checkConfirmation;
	private Button disable;
	private Button follow;
	private LabeledText textConfirmation;
	private Button radioIndexOID;
	private Button radioIndexValue;
	
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
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);
		
		textName = new LabeledText(dialogArea, SWT.NONE);
		textName.setLabel(Messages.get().General_Name);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textName.setLayoutData(gd);
		textName.setText(objectTool.getName());
		
		textDescription = new LabeledText(dialogArea, SWT.NONE);
		textDescription.setLabel(Messages.get().General_Description);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textDescription.setLayoutData(gd);
		textDescription.setText(objectTool.getDescription());
		
		textData = new LabeledText(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textData.setLayoutData(gd);
		textData.setText(objectTool.getData());
		
		switch(objectTool.getType())
		{
			case ObjectTool.TYPE_INTERNAL:
				textData.setLabel(Messages.get().General_Operation);
				break;
			case ObjectTool.TYPE_LOCAL_COMMAND:
			case ObjectTool.TYPE_SERVER_COMMAND:
				textData.setLabel(Messages.get().General_Command);
				createOutputGroup(dialogArea);
				break;
			case ObjectTool.TYPE_ACTION:
				textData.setLabel(Messages.get().General_AgentAction);
				createOutputGroup(dialogArea);
				break;
			case ObjectTool.TYPE_URL:
				textData.setLabel(Messages.get().General_URL);
				break;
			case ObjectTool.TYPE_FILE_DOWNLOAD:
			   String[] parameters = objectTool.getData().split("\u007F"); //$NON-NLS-1$
			   
				textData.setLabel(Messages.get().General_RemoteFileName);
				textData.setText((parameters.length > 0) ? parameters[0] : ""); //$NON-NLS-1$
				
				Group fileOptionsGoup = new Group(dialogArea, SWT.NONE);
				fileOptionsGoup.setText(Messages.get().General_FileOptions);
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            fileOptionsGoup.setLayoutData(gd);

				GridLayout fileGroupLayout = new GridLayout();
				fileGroupLayout.verticalSpacing = WidgetHelper.OUTER_SPACING;
				fileGroupLayout.numColumns = 1;
				fileOptionsGoup.setLayout(fileGroupLayout);
				
		      maxFileSize  = new LabeledSpinner(fileOptionsGoup, SWT.NONE);
		      gd = new GridData();
		      gd.horizontalAlignment = SWT.FILL;
		      gd.grabExcessHorizontalSpace = true;
		      maxFileSize.setLayoutData(gd);
				maxFileSize.setLabel(Messages.get().General_LimitDownloadFileSizeLable);
				maxFileSize.setRange(0, 0x7FFFFFFF);
				try
				{
				   maxFileSize.setSelection((parameters.length > 1) ? Integer.parseInt(parameters[1]) : 0);
				}
				catch(NumberFormatException e)
				{
				   maxFileSize.setSelection(0);
				}
							
				follow = new Button(fileOptionsGoup, SWT.CHECK);
				follow.setText(Messages.get().General_FollowFileChanges);
				if(parameters.length > 2) //$NON-NLS-1$
				{
				   follow.setSelection( parameters[2].equals("true") ? true : false);  //$NON-NLS-1$
				}	
				break;
			case ObjectTool.TYPE_TABLE_SNMP:
				textData.setLabel(Messages.get().General_Title);
				
				Group snmpOptGroup = new Group(dialogArea, SWT.NONE);
				snmpOptGroup.setText(Messages.get().General_SNMPTableOptions);
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				snmpOptGroup.setLayoutData(gd);
				layout = new GridLayout();
				snmpOptGroup.setLayout(layout);
				
				new Label(snmpOptGroup, SWT.NONE).setText(Messages.get().General_UseAsIndex);
				
				radioIndexOID = new Button(snmpOptGroup, SWT.RADIO);
				radioIndexOID.setText(Messages.get().General_OIDSuffix);
				radioIndexOID.setSelection((objectTool.getFlags() & ObjectTool.SNMP_INDEXED_BY_VALUE) == 0);
				
				radioIndexValue = new Button(snmpOptGroup, SWT.RADIO);
				radioIndexValue.setText(Messages.get().General_FirstColumnValue);
				radioIndexValue.setSelection(!radioIndexOID.getSelection());

				break;
			case ObjectTool.TYPE_TABLE_AGENT:
				textData.setLabel(Messages.get().General_Title);
				
				String[] parts = objectTool.getData().split("\u007F"); //$NON-NLS-1$
				textData.setText((parts.length > 0) ? parts[0] : ""); //$NON-NLS-1$

				textParameter = new LabeledText(dialogArea, SWT.NONE);
				textParameter.setLabel(Messages.get().General_Parameter);
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				textParameter.setLayoutData(gd);
				textParameter.setText((parts.length > 1) ? parts[1] : ""); //$NON-NLS-1$

				textRegexp = new LabeledText(dialogArea, SWT.NONE);
				textRegexp.setLabel(Messages.get().General_RegExp);
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				textRegexp.setLayoutData(gd);
				textRegexp.setText((parts.length > 2) ? parts[2] : ""); //$NON-NLS-1$
				break;
		}
		
		Group confirmationGroup = new Group(dialogArea, SWT.NONE);
		confirmationGroup.setText(Messages.get().General_Confirmation);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		confirmationGroup.setLayoutData(gd);
		layout = new GridLayout();
		confirmationGroup.setLayout(layout);
		
		checkConfirmation = new Button(confirmationGroup, SWT.CHECK);
		checkConfirmation.setText(Messages.get().General_RequiresConfirmation);
		checkConfirmation.setSelection((objectTool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0);
		checkConfirmation.addSelectionListener(new SelectionListener()	{
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textConfirmation.setEnabled(checkConfirmation.getSelection());
				if (checkConfirmation.getSelection())
					textConfirmation.setFocus();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		textConfirmation = new LabeledText(confirmationGroup, SWT.NONE);
		textConfirmation.setLabel(Messages.get().General_ConfirmationMessage);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textConfirmation.setLayoutData(gd);
		textConfirmation.setText(objectTool.getConfirmationText());
		textConfirmation.setEnabled(checkConfirmation.getSelection());
		
		//Disable option
		disable = new Button(dialogArea, SWT.CHECK);
		disable.setText(Messages.get().General_DisableObjectToll);
		disable.setSelection((objectTool.getFlags() & ObjectTool.DISABLED) > 0);
		
		return dialogArea;
	}
	
	/**
	 * @param parent
	 */
	private void createOutputGroup(Composite parent)
	{
		Group outputGroup = new Group(parent, SWT.NONE);
		outputGroup.setText(Messages.get().General_ExecOptions);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		outputGroup.setLayoutData(gd);
		outputGroup.setLayout(new GridLayout());
		
		checkOutput = new Button(outputGroup, SWT.CHECK);
		checkOutput.setText(Messages.get().General_GeneratesOutput);
		checkOutput.setSelection((objectTool.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0);
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		objectTool.setName(textName.getText());
		objectTool.setDescription(textDescription.getText());
		if (objectTool.getType() == ObjectTool.TYPE_TABLE_AGENT)
		{
			objectTool.setData(textData.getText() + "\u007F" + textParameter.getText() + "\u007F" + textRegexp.getText()); //$NON-NLS-1$ //$NON-NLS-2$
		}
		else
		{
		   if(objectTool.getType() == ObjectTool.TYPE_FILE_DOWNLOAD)
   		{
		      objectTool.setData(textData.getText() + "\u007F" + maxFileSize.getSelection() + "\u007F" + follow.getSelection()); //$NON-NLS-1$ //$NON-NLS-2$
   		}
		   else
		   {
		      objectTool.setData(textData.getText());
		   }
		}
		
		if (checkConfirmation.getSelection())
		{
			objectTool.setFlags(objectTool.getFlags() | ObjectTool.ASK_CONFIRMATION);
		}
		else
		{
			objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.ASK_CONFIRMATION);
		}
		objectTool.setConfirmationText(textConfirmation.getText());
		
      if (disable.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.DISABLED);
      }
      else
      {
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.DISABLED);
      }
		
		if (objectTool.getType() == ObjectTool.TYPE_TABLE_SNMP)
		{
			if (radioIndexValue.getSelection())
			{
				objectTool.setFlags(objectTool.getFlags() | ObjectTool.SNMP_INDEXED_BY_VALUE);
			}
			else
			{
				objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.SNMP_INDEXED_BY_VALUE);
			}
		}
		
		if ((objectTool.getType() == ObjectTool.TYPE_LOCAL_COMMAND) ||
		    (objectTool.getType() == ObjectTool.TYPE_SERVER_COMMAND) ||
		    (objectTool.getType() == ObjectTool.TYPE_ACTION))
		{
			if (checkOutput.getSelection())
			{
				objectTool.setFlags(objectTool.getFlags() | ObjectTool.GENERATES_OUTPUT);
			}
			else
			{
				objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.GENERATES_OUTPUT);
			}
		}
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
