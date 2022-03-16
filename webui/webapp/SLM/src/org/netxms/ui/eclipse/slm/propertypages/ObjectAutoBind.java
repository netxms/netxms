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
import org.netxms.client.objects.BaseBusinessService;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.slm.Activator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Object Auto Bind" property page for business service
 */
public class ObjectAutoBind extends PropertyPage
{
   private BaseBusinessService businessService;
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

      businessService = (BaseBusinessService)getElement().getAdapter(BaseBusinessService.class);

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
      checkboxEnableBind.setText("Automatically add objects selected by filter to this business service as check");
      checkboxEnableBind.setSelection(businessService.isAutoBindEnabled());
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
      checkboxEnableUnbind.setText("Automatically remove objects selected by filter from this business service");
      checkboxEnableUnbind.setSelection(businessService.isAutoUnbindEnabled());
      checkboxEnableUnbind.setEnabled(businessService.isAutoBindEnabled());

      thresholdCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Status Threashold", new GridData());
      thresholdCombo.add("Default");
      for(int i = 1; i <= 4; i++)
         thresholdCombo.add(StatusDisplayInfo.getStatusText(i));
      thresholdCombo.select(businessService.getObjectStatusThreshold());

      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Filtering script");

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
      label.setLayoutData(gd);

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true,
            "Variables:\r\n\t$node\t\tnode being tested (null if object is not a node).\r\n\t$object\t\tobject being tested.\r\n\t$service\tcurrent business service this check belongs to.\r\n\r\nReturn value: true to bind object to this business service, false to unbind, null to make no changes.");
      filterSource.setText(businessService.getAutoBindFilter());
      filterSource.setEnabled(businessService.isAutoBindEnabled());

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
      md.setAutoBindFilter(filterSource.getText());
      int flags = businessService.getAutoBindFlags();
      flags = apply ? flags | AutoBindObject.OBJECT_BIND_FLAG : flags & ~AutoBindObject.OBJECT_BIND_FLAG;
      flags = remove ? flags | AutoBindObject.OBJECT_UNBIND_FLAG : flags & ~AutoBindObject.OBJECT_UNBIND_FLAG;
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
                     ObjectAutoBind.this.setValid(true);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot change business service objects automatic bind options";
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
