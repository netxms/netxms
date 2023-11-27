/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.ObjectMenuFilter;
import org.netxms.client.objecttools.ObjectAction;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Filter" property page for object tool
 */
public class Filter extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Filter.class);
   
	private ObjectMenuFilter filter;
	private Button checkAgent;
	private Button checkSNMP;
	private Button checkMatchOID;
	private Button checkMatchNodeOS;
	private Button checkMatchWorkstationOS;
	private Button checkMatchTemplate;
   private Button checkMatchCustomAttributes;
	private Text textOID;
	private Text textNodeOS;
	private Text textWorkstationOS;
	private Text textTemplate;
   private Text textCustomAttributes;
	private ObjectToolDetails objectTool = null;
	private ObjectAction action = null;
   
   
   /**
    * Constructor
    * 
    * @param toolDetails
    */
   public Filter(ObjectAction action)
   {
      super("Filter");
      noDefaultAndApplyButton();
      this.action = action;
   }     

   /**
    * Constructor
    * 
    * @param toolDetails
    */
   public Filter(ObjectToolDetails toolDetails)
   {
      super("Filter");
      noDefaultAndApplyButton();
      objectTool = toolDetails;
      action = (ObjectAction)objectTool;
   }	

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{	   
	   filter = action.getMenuFilter();

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);
		
		checkAgent = new Button(dialogArea, SWT.CHECK);
		checkAgent.setText(i18n.tr("NetXMS agent should be available"));
		checkAgent.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_AGENT) != 0);
		
		checkSNMP = new Button(dialogArea, SWT.CHECK);
		checkSNMP.setText(i18n.tr("Node should support SNMP"));
		checkSNMP.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_SNMP) != 0);
		
		checkMatchOID = new Button(dialogArea, SWT.CHECK);
		checkMatchOID.setText(i18n.tr("Node SNMP OID should match with the following template:"));
		checkMatchOID.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_OID_MATCH) != 0);
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
		textOID.setText(filter.snmpOid);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textOID.setLayoutData(gd);
		textOID.setEnabled(checkMatchOID.getSelection());
		
		checkMatchNodeOS = new Button(dialogArea, SWT.CHECK);
		checkMatchNodeOS.setText(i18n.tr("Node platform name should match the following template (comma separated regular expression list)"));
		checkMatchNodeOS.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_NODE_OS_MATCH) != 0);
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
		textNodeOS.setText(filter.toolNodeOS);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textNodeOS.setLayoutData(gd);
		textNodeOS.setEnabled(checkMatchNodeOS.getSelection());
		
		if (action instanceof ObjectTool && action.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND)
		{
   		checkMatchWorkstationOS = new Button(dialogArea, SWT.CHECK);
   		checkMatchWorkstationOS.setText("Workstation OS name should match the following template (comma separated regular expression list)");
   		checkMatchWorkstationOS.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_WORKSTATION_OS_MATCH) != 0);
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
   		textWorkstationOS.setText(filter.toolWorkstationOS);
   		gd = new GridData();
   		gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalIndent = 20;
         textWorkstationOS.setLayoutData(gd);
         textWorkstationOS.setEnabled(checkMatchWorkstationOS.getSelection());
		}
		
		checkMatchTemplate = new Button(dialogArea, SWT.CHECK);
		checkMatchTemplate.setText(i18n.tr("Parent template name should match the following template (comma separated regular expression list)"));
		checkMatchTemplate.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_TEMPLATE_MATCH) != 0);
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
		textTemplate.setText(filter.toolTemplate);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = 20;
      textTemplate.setLayoutData(gd);
      textTemplate.setEnabled(checkMatchTemplate.getSelection());
		
      checkMatchCustomAttributes = new Button(dialogArea, SWT.CHECK);
      checkMatchCustomAttributes.setText("The following custom attribute(s) should exist");
      checkMatchCustomAttributes.setSelection((filter.flags & ObjectMenuFilter.REQUIRES_CUSTOM_ATTRIBUTE_MATCH) != 0);
      checkMatchCustomAttributes.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textCustomAttributes.setEnabled(checkMatchCustomAttributes.getSelection());
            if (checkMatchCustomAttributes.getSelection())
               textCustomAttributes.setFocus();
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      textCustomAttributes = new Text(dialogArea, SWT.BORDER);
      textCustomAttributes.setText(filter.toolCustomAttributes);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = 20;
      textCustomAttributes.setLayoutData(gd);
      textCustomAttributes.setEnabled(checkMatchCustomAttributes.getSelection());
      
		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{      
		if (checkAgent.getSelection())
		   setFlag(ObjectMenuFilter.REQUIRES_AGENT);
		else
		   clearFlag(ObjectMenuFilter.REQUIRES_AGENT);

		if (checkSNMP.getSelection())
		   setFlag(ObjectMenuFilter.REQUIRES_SNMP);
		else
		   clearFlag(ObjectMenuFilter.REQUIRES_SNMP);

		if (checkMatchOID.getSelection())
			setFlag(ObjectMenuFilter.REQUIRES_OID_MATCH);
		else
		   clearFlag(ObjectMenuFilter.REQUIRES_OID_MATCH);
		
      setFilter(textOID.getText(), ObjectMenuFilter.REQUIRES_OID_MATCH);	

      if (checkMatchNodeOS.getSelection())
         setFlag(ObjectMenuFilter.REQUIRES_NODE_OS_MATCH);
      else
         clearFlag(ObjectMenuFilter.REQUIRES_NODE_OS_MATCH);
      
      setFilter(textNodeOS.getText().trim(), ObjectMenuFilter.REQUIRES_NODE_OS_MATCH);
      
      if ((checkMatchWorkstationOS != null) && checkMatchWorkstationOS.getSelection())
         setFlag(ObjectMenuFilter.REQUIRES_WORKSTATION_OS_MATCH);
      else
         clearFlag(ObjectMenuFilter.REQUIRES_WORKSTATION_OS_MATCH);
      
      if (textWorkstationOS != null)
         setFilter(textWorkstationOS.getText().trim(), ObjectMenuFilter.REQUIRES_WORKSTATION_OS_MATCH);

      if (checkMatchTemplate.getSelection())
         setFlag(ObjectMenuFilter.REQUIRES_TEMPLATE_MATCH);
      else
         clearFlag(ObjectMenuFilter.REQUIRES_TEMPLATE_MATCH);
      
      setFilter(textTemplate.getText().trim(), ObjectMenuFilter.REQUIRES_TEMPLATE_MATCH);    

      if (checkMatchCustomAttributes.getSelection())
         setFlag(ObjectMenuFilter.REQUIRES_CUSTOM_ATTRIBUTE_MATCH);
      else
         clearFlag(ObjectMenuFilter.REQUIRES_CUSTOM_ATTRIBUTE_MATCH);
      
      setFilter(textCustomAttributes.getText().trim(), ObjectMenuFilter.REQUIRES_CUSTOM_ATTRIBUTE_MATCH);    
      
      return true;
	}

   /**
    * Set filter text
    *
    * @param filterText
    * @param filterType
    */
	private void setFilter(String filterText, int filterType)
	{
      if (objectTool != null)
         objectTool.setFilter(filterText, filterType);
      else
         filter.setFilter(filterText, filterType);
	}
	
	/**
	 * Set filter flag
	 * 
	 * @param flag
	 */
	private void setFlag(int flag)
	{
      if (objectTool != null)
         objectTool.setFilterFlags(objectTool.getMenuFilter().flags | flag);
      else
         filter.flags = filter.flags | flag;
	}
	
	/**
	 * Clear filter flag
	 * 
	 * @param flag
	 */
	private void clearFlag(int flag)
   {
      if (objectTool != null)
         objectTool.setFilterFlags(objectTool.getMenuFilter().flags & ~flag);
      else
         filter.flags = filter.flags & ~flag;
   }
}
