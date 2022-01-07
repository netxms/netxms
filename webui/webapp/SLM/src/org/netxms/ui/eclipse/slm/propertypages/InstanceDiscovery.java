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
package org.netxms.ui.eclipse.slm.propertypages;

import java.util.Arrays;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.slm.Activator;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Instance Discovery" property page for business service prototype
 */
public class InstanceDiscovery extends PropertyPage
{
   public static final List<String> DISCOVERY_TYPES = Arrays.asList(null, "Agent List", "Agent Table", null, null, "Script", null, null, null);

   private BusinessServicePrototype object;
	private Combo discoveryMethod;
	private LabeledText discoveryData;
	private ObjectSelector instanceSourceSelector;
	private ScriptEditor filterScript;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{		
      object = (BusinessServicePrototype)getElement().getAdapter(BusinessServicePrototype.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

      discoveryMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, "Instance discovery method", WidgetHelper.DEFAULT_LAYOUT_DATA);

      for (String type : DISCOVERY_TYPES)
      {
         if (type != null)
            discoveryMethod.add(type);            
      }
      discoveryMethod.select(discoveryMethod.indexOf(DISCOVERY_TYPES.get(object.getInstanceDiscoveryMethod())));
      discoveryMethod.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				int method = DISCOVERY_TYPES.indexOf(discoveryMethod.getText());
		      discoveryData.setLabel(getDataLabel(method));
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

      instanceSourceSelector  = new ObjectSelector(dialogArea, SWT.NONE, true);
      instanceSourceSelector.setLabel("Source");
      instanceSourceSelector.setObjectClass(Node.class);
      instanceSourceSelector.setObjectId(object.getSourceNode());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      instanceSourceSelector.setLayoutData(gd);

      discoveryData = new LabeledText(dialogArea, SWT.NONE);
      discoveryData.setLabel(getDataLabel(object.getInstanceDiscoveryMethod()));
      discoveryData.setText(object.getInstanceDiscoveryData());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      discoveryData.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 0;
      gd.heightHint = 0;
      final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite dialogArea, int style)
			{
				return new ScriptEditor(dialogArea, style, SWT.H_SCROLL | SWT.V_SCROLL, false, 
				      "Variables:\r\n\t$1\t\t\tInstance to test;\r\n\t$2\t\t\tInstance data (secondary value for Script method);\r\n\t$node\t\trelated node;\r\n\t$prototype\t\tthis business service prototype object;\r\n\r\nReturn value:\r\n\ttrue/false to accept or reject instance without additional changes or\r\n\tarray of two or three elements to modify instance:\r\n\t\t1st element - true/false to indicate acceptance;\r\n\t\t2nd element - new instance name;\r\n\t\t3rd element - new instance display name.");
			}
      };
      filterScript = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, "Instance discovery filter script", gd);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      filterScript.setLayoutData(gd);
      filterScript.setText(object.getInstanceDiscoveryFilter());

		return dialogArea;
	}

	/**
	 * Get label for data field
	 * 
	 * @param method
	 * @return
	 */
	private static String getDataLabel(int method)
	{    
	   switch(method)
      {
         case DataCollectionObject.IDM_NONE:
            return "Discovery data";
         case DataCollectionObject.IDM_AGENT_LIST:
            return "List name";
         case DataCollectionObject.IDM_AGENT_TABLE:
         case DataCollectionObject.IDM_INTERNAL_TABLE:
            return "Table name";
         case DataCollectionObject.IDM_SNMP_WALK_VALUES:
         case DataCollectionObject.IDM_SNMP_WALK_OIDS:
            return "Base SNMP OID";
         case DataCollectionObject.IDM_SCRIPT:
            return "Script name";
         case DataCollectionObject.IDM_WEB_SERVICE:
            return "Web service request";
         case DataCollectionObject.IDM_WINPERF:
            return "Object name";
      }
      return "";
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   private boolean applyChanges(final boolean isApply)
	{       
      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setInstanceDiscoveryMethod(DISCOVERY_TYPES.indexOf(discoveryMethod.getText()));
      md.setInstanceDiscoveryData(discoveryData.getText());
      md.setInstanceDiscoveryFilter(filterScript.getText());
      md.setSourceNode(instanceSourceSelector.getObjectId());

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob(String.format("Modify instance discovery for busines service prototype %s", object.getObjectName()), null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     InstanceDiscovery.this.setValid(true);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot modify instance discovery settings";
         }
      }.start();

      return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      applyChanges(false);
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
      discoveryMethod.select(discoveryMethod.indexOf(DISCOVERY_TYPES.get(DataCollectionObject.IDM_SCRIPT)));
      discoveryData.setLabel(getDataLabel(DataCollectionObject.IDM_SCRIPT));
      discoveryData.setText("");
      filterScript.setText("");
	}
}
