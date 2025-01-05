/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.businessservice.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BaseBusinessService;
import org.netxms.client.objects.interfaces.AutoBindDCIObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "DCI Auto Bind" property page
 */
public class DCIAutoBind extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(DCIAutoBind.class);

   private BaseBusinessService businessService;
	private Button checkboxEnableBind;
	private Button checkboxEnableUnbind;
   private Combo thresholdCombo;
	private ScriptEditor filterSource;
   private boolean initialBind;
   private boolean initialUnbind;
	private String initialAutoBindFilter;
   private int initialStatusThreshold;

   public DCIAutoBind(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(DCIAutoBind.class).tr("DCI Auto Bind"), object);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      businessService = (BaseBusinessService)object;
		if (businessService == null)	// Paranoid check
			return dialogArea;

      initialBind = businessService.isDciAutoBindEnabled();
      initialUnbind = businessService.isDciAutoUnbindEnabled();
		initialAutoBindFilter = businessService.getDciAutoBindFilter();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableBind = new Button(dialogArea, SWT.CHECK);
      checkboxEnableBind.setText("Automatically add DCI selected by filter to this business service as check");
      checkboxEnableBind.setSelection(businessService.isDciAutoBindEnabled());
      checkboxEnableBind.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableBind.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
					checkboxEnableUnbind.setEnabled(true);
					thresholdCombo.setEnabled(true);
				}
				else
				{
					filterSource.setEnabled(false);
					checkboxEnableUnbind.setEnabled(false);
               thresholdCombo.setEnabled(false);
				}
			}
      });
      
      checkboxEnableUnbind = new Button(dialogArea, SWT.CHECK);
      checkboxEnableUnbind.setText("Automatically remove DCI selected by filter from this business service");
      checkboxEnableUnbind.setSelection(businessService.isDciAutoUnbindEnabled());
      checkboxEnableUnbind.setEnabled(businessService.isDciAutoBindEnabled());

      thresholdCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Status Threashold"), new GridData());
      thresholdCombo.add(i18n.tr("Default"));   
      for (int i = 1; i <= 4; i++)
         thresholdCombo.add(StatusDisplayInfo.getStatusText(i)); 
      thresholdCombo.select(businessService.getDciStatusThreshold());

      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Filtering script"));

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
      label.setLayoutData(gd);

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, 
            "Variables:\n\t$node\t\tnode being tested (null if object is not a node).\n\t$object\t\tobject being tested.\n\t$dci\t\t\tDCI object being tested.\n\t$service\tcurrent business service this check belongs to.\n\nReturn value: true to bind dci to this business service, false to unbind, null to make no changes.");
      filterSource.setText(businessService.getDciAutoBindFilter());
      filterSource.setEnabled(businessService.isDciAutoBindEnabled());

		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.widthHint = 0;
      gd.heightHint = 0;
		filterSource.setLayoutData(gd);

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{      
      boolean apply = checkboxEnableBind.getSelection();
      boolean remove = checkboxEnableUnbind.getSelection();
			
		if ((apply == initialBind) && (remove == initialUnbind) && (initialStatusThreshold == thresholdCombo.getSelectionIndex()) && initialAutoBindFilter.equals(filterSource.getText()))
			return true;		// Nothing to apply

		if (isApply)
			setValid(false);
		
		final NXCSession session =  Registry.getSession();
      final NXCObjectModificationData md = new NXCObjectModificationData(businessService.getObjectId());
		md.setAutoBindFilter2(filterSource.getText());      
		int flags = businessService.getAutoBindFlags();
      flags = apply ? flags | AutoBindDCIObject.DCI_BIND_FLAG : flags & ~AutoBindDCIObject.DCI_BIND_FLAG;  
      flags = remove ? flags | AutoBindDCIObject.DCI_UNBIND_FLAG : flags & ~AutoBindDCIObject.DCI_UNBIND_FLAG;  
      md.setAutoBindFlags(flags);
      md.setDciStatusThreshold(thresholdCombo.getSelectionIndex());
		
		new Job(i18n.tr("Update auto-bind filter"), null, null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
		      initialBind = apply;
		      initialUnbind = remove;
				initialAutoBindFilter = md.getAutoBindFilter2();
            initialStatusThreshold = md.getDciStatusThreshold();
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
							DCIAutoBind.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot change DCI automatic bind options for business service");
			}
		}.start();
		
		return true;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "dciAutoBind";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof BaseBusinessService;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 27;
   }
}
