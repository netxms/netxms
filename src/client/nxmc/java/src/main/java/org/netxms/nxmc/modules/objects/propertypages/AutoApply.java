/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Auto apply" property page for template object
 */
public class AutoApply extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(AutoApply.class);

   private Template template;
	private Button checkboxEnableApply;
	private Button checkboxEnableRemove;
	private ScriptEditor filterSource;
	private boolean initialBind;
   private boolean initialUnbind;
	private String initialApplyFilter;
	
   /**
    * Create "auto apply" property page for given object
    *
    * @param object object to create property page for
    */
   public AutoApply(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(AutoApply.class).tr("Automatic Apply Rules"), object);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      template = (Template)object;
		if (object == null)	// Paranoid check
			return dialogArea;
		
		initialBind = template.isAutoApplyEnabled();
		initialUnbind = template.isAutoRemoveEnabled();
		initialApplyFilter = template.getAutoApplyFilter();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableApply = new Button(dialogArea, SWT.CHECK);
      checkboxEnableApply.setText(i18n.tr("Apply this template automatically to objects selected by filter"));
      checkboxEnableApply.setSelection(template.isAutoApplyEnabled());
      checkboxEnableApply.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableApply.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
					checkboxEnableRemove.setEnabled(true);
				}
				else
				{
					filterSource.setEnabled(false);
					checkboxEnableRemove.setEnabled(false);
				}
			}
      });

      // Enable/disable check box
      checkboxEnableRemove = new Button(dialogArea, SWT.CHECK);
      checkboxEnableRemove.setText(i18n.tr("Remove this template automatically when object no longer passes through filter"));
      checkboxEnableRemove.setSelection(template.isAutoRemoveEnabled());
      checkboxEnableRemove.setEnabled(template.isAutoApplyEnabled());

      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Filtering script"));

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
		label.setLayoutData(gd);

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, 
            "Variables:\n\t$node\tnode being tested (null if object is not a node).\n\t$object\tobject being tested.\n\t$template\tthis template object.\n\nReturn value: true to apply this template to node, false to remove, null to make no changes.");
		filterSource.setText(template.getAutoApplyFilter());
		filterSource.setEnabled(template.isAutoApplyEnabled());

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
	   boolean apply = checkboxEnableApply.getSelection();
      boolean remove = checkboxEnableRemove.getSelection();
			
		if ((apply == initialBind) && (remove == initialUnbind) && initialApplyFilter.equals(filterSource.getText()))
			return true;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = Registry.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setAutoBindFilter(filterSource.getText());
      int flags = ((Template)object).getAutoApplyFlags();
      flags = apply ? flags | AutoBindObject.OBJECT_BIND_FLAG : flags & ~AutoBindObject.OBJECT_BIND_FLAG;  
      flags = remove ? flags | AutoBindObject.OBJECT_UNBIND_FLAG : flags & ~AutoBindObject.OBJECT_UNBIND_FLAG;  
      md.setAutoBindFlags(flags);

      new Job(i18n.tr("Updating auto-apply filter"), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
		      initialBind = apply;
		      initialUnbind = remove;
				initialApplyFilter = md.getAutoBindFilter();
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> AutoApply.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot change template automatic apply options");
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
      return "autoApply";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Template;
   }
}
