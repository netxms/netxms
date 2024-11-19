/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Condition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Script" property page for condition object
 */
public class ConditionScript extends ObjectPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ConditionScript.class);
   
	private Condition condition;
	private ScriptEditor filterSource;
	private String initialScript;

   /**
    * Create new page.
    *
    * @param condition object to edit
    */
   public ConditionScript(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ConditionScript.class).tr("Script"), object);
   }

   @Override
   public String getId()
   {
      return "conditionEvents";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Condition;
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
      this.condition = (Condition)object;
		
		initialScript = condition.getScript();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      // Script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Status calculation script"));

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, "Variables:\n\t$values\tarray containing values for configured DCIs (in same order as DCIs are listed in configuration)\n\nReturn value: true/false to indicate if condition is active or not");
		filterSource.setText(condition.getScript());
		
		GridData gd = new GridData();
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
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
		if (initialScript.equals(filterSource.getText()))
			return true;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final String newScript = filterSource.getText();
		final NXCSession session = Registry.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(condition.getObjectId());
		md.setScript(newScript);
      new Job(i18n.tr("Updating condition script"), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				initialScript = newScript;
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot change condition script");
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
							ConditionScript.this.setValid(true);
						}
					});
				}
			}
		}.start();
		
		return true;
	}
}
