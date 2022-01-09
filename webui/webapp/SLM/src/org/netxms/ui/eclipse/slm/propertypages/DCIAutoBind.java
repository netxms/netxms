/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.interfaces.AutoBindDCIObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.slm.Activator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Instance Discovery" property page for DCO
 */
public class DCIAutoBind extends PropertyPage
{
   private BusinessService businessService;
   private Button checkboxEnableBind;
   private Button checkboxEnableUnbind;
   private Combo thresholdCombo;
   private ScriptEditor filterSource;
   private boolean initialBind;
   private boolean initialUnbind;
   private String initialAutoBindFilter;
   private int initialStatusThreshold;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{		
      Composite dialogArea = new Composite(parent, SWT.NONE);

      businessService = (BusinessService)getElement().getAdapter(BusinessService.class);

      initialBind = businessService.isAutoBindEnabled();
      initialUnbind = businessService.isAutoUnbindEnabled();
      initialAutoBindFilter = businessService.getAutoBindFilter();
      initialAutoBindFilter = businessService.getAutoBindFilter();
      initialStatusThreshold = businessService.getObjectStatusThreshold();

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

      thresholdCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Status Threashold", new GridData());
      thresholdCombo.add("Default");
      for(int i = 1; i <= 4; i++)
         thresholdCombo.add(StatusDisplayInfo.getStatusText(i));
      thresholdCombo.select(businessService.getDciStatusThreshold());

      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Filtering script");

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
      label.setLayoutData(gd);

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true,
            "Variables:\r\n\t$node\tnode being tested (null if object is not a node).\r\n\t$object\tobject being tested.\r\n\t$dci\tDCI object being tested.\r\n\r\nReturn value: true to bind dci to this business service, false to unbind, null to make no changes.");
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   private boolean applyChanges(final boolean isApply)
	{       
      if (isApply)
         setValid(false);

      boolean apply = checkboxEnableBind.getSelection();
      boolean remove = checkboxEnableUnbind.getSelection();

      if ((apply == initialBind) && (remove == initialUnbind) && (initialStatusThreshold == thresholdCombo.getSelectionIndex()) && initialAutoBindFilter.equals(filterSource.getText()))
         return true; // Nothing to apply

      if (isApply)
         setValid(false);

      final NXCSession session = ConsoleSharedData.getSession();
      final NXCObjectModificationData md = new NXCObjectModificationData(businessService.getObjectId());
      md.setAutoBindFilter2(filterSource.getText());
      int flags = businessService.getAutoBindFlags();
      flags = apply ? flags | AutoBindDCIObject.DCI_BIND_FLAG : flags & ~AutoBindDCIObject.DCI_BIND_FLAG;
      flags = remove ? flags | AutoBindDCIObject.DCI_UNBIND_FLAG : flags & ~AutoBindDCIObject.DCI_UNBIND_FLAG;
      md.setAutoBindFlags(flags);
      md.setObjectStatusThreshold(thresholdCombo.getSelectionIndex());

      new ConsoleJob("Update auto-bind filter", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
            initialBind = apply;
            initialUnbind = remove;
            initialAutoBindFilter = md.getAutoBindFilter();
            initialStatusThreshold = md.getObjectStatusThreshold();
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
            return "Cannot change DCI automatic bind options for business service";
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
      checkboxEnableBind.setSelection(false);
      checkboxEnableUnbind.setSelection(false);
      filterSource.setText("");
	}
}
