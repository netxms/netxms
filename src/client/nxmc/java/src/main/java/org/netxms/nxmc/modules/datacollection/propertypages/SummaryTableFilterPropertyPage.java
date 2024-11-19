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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Filter" page for DCI summary table
 */
public class SummaryTableFilterPropertyPage extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(SummaryTableFilterPropertyPage.class);
   
	private DciSummaryTable table;
	private ScriptEditor filter;

   /**
    * Constructor
    * @param table
    */
   public SummaryTableFilterPropertyPage(DciSummaryTable table)
   {
      super(LocalizationHelper.getI18n(SummaryTableFilterPropertyPage.class).tr("Filter"));
      this.table = table;
   }
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 0;
      gd.heightHint = 0;
      final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				return new ScriptEditor(parent, style,  SWT.H_SCROLL | SWT.V_SCROLL,  
				      "Variables:\n\t$object\tcurrent object\n\t$node\tcurrent object if it's class is Node\n\nReturn value: true to include current object into this summary table");
			}
      };
      filter = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER, factory, i18n.tr("Filter script"), gd);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      filter.setLayoutData(gd);
      filter.setText(table.getNodeFilter());

      return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);

		table.setNodeFilter(filter.getText());
		
		final NXCSession session = Registry.getSession();
		new Job(i18n.tr("Update DCI summary table configuration"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				synchronized(table)
				{
					int id = session.modifyDciSummaryTable(table);
					table.setId(id);
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update DCI summary table configuration");
			}

			/* (non-Javadoc)
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							SummaryTableFilterPropertyPage.this.setValid(true);
						}
					});
				}
			}
		}.start();
		return true;
	}
}
