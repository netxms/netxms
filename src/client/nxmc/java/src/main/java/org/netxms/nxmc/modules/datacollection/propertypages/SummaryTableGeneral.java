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
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectDciDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" page for DCI summary table
 */
public class SummaryTableGeneral extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(SummaryTableGeneral.class);
   
	private DciSummaryTable table;
	private LabeledText menuPath;
	private LabeledText title;
	private LabeledText dciName;
	private Button importButton;

   /**
    * Constructor
    * @param table
    */
   public SummaryTableGeneral(DciSummaryTable table)
   {
      super(LocalizationHelper.getI18n(SummaryTableGeneral.class).tr("General"));
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
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      menuPath = new LabeledText(dialogArea, SWT.NONE);
      menuPath.setLabel(i18n.tr("Menu path"));
      menuPath.setText(table.getMenuPath());
      menuPath.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false, 2, 1));

      title = new LabeledText(dialogArea, SWT.NONE);
      title.setLabel(i18n.tr("Title"));
      title.setText(table.getTitle());
      title.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false, 2, 1));
      
      if (table.isTableSoure())
      {         
         dciName = new LabeledText(dialogArea, SWT.NONE);
         dciName.setLabel("DCI name");
         dciName.getTextControl().setTextLimit(255);
         dciName.setText(table.getTableDciName());
         dciName.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

         importButton = new Button(dialogArea, SWT.PUSH);
         importButton.setText("Import...");
         GridData gd = new GridData();
         gd.verticalAlignment = SWT.BOTTOM;
         gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
         importButton.setLayoutData(gd);
         importButton.addSelectionListener(new SelectionAdapter() {            
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               selectDci();
            }
         });
      }

      return dialogArea;
	}
	
	private void selectDci()
	{
	   SelectDciDialog dlg = new SelectDciDialog(getShell(), 0);
	   dlg.setDcObjectType(2);
	   dlg.setSingleSelection(true);
	   if (dlg.open() == Window.OK)
	   {
	      table.setTableDciName(dlg.getSelection().get(0).getName());
	      dciName.setText(table.getTableDciName());
	   }
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

		table.setMenuPath(menuPath.getText());
		table.setTitle(title.getText());
		
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
							SummaryTableGeneral.this.setValid(true);
						}
					});
				}
			}
		}.start();
		
		return true;
	}
}
