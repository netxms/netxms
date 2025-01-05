/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Object Filter" property page
 */
public class MapObjectFilter extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(MapObjectFilter.class);
   
   private NetworkMap map;
	private Button checkboxEnableFilter;
	private ScriptEditor filterSource;
	private String initialFilter;
	private boolean initialEnable;
   
   /**
    * Constructor 
    * 
    * @param object object to show properties for
    */
   public MapObjectFilter(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(MapObjectFilter.class).tr("Object Filter"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "map-object-filter";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof NetworkMap);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
      map = (NetworkMap)object;

      initialFilter = map.getFilter();
      initialEnable = (map.getFlags() & NetworkMap.MF_FILTER_OBJECTS) != 0;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableFilter = new Button(dialogArea, SWT.CHECK);
      checkboxEnableFilter.setText(i18n.tr("Filter objects"));
      checkboxEnableFilter.setSelection(initialEnable);
      checkboxEnableFilter.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableFilter.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
				}
				else
				{
					filterSource.setEnabled(false);
				}
			}
      });
      
      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Filtering script"));

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
		label.setLayoutData(gd);
      
      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL,  
            "Variables:\n\t$object\tcurrent object\n\t$node\tcurrent object if it's class is Node\n\nReturn value: true to include current object into this map");
		filterSource.setText(initialFilter);
		filterSource.setEnabled(initialEnable);
		
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
		final String filter = filterSource.getText();
		if (initialFilter.equals(filter) && (checkboxEnableFilter.getSelection() == initialEnable))
			return true;		// Nothing to apply

		if (isApply)
			setValid(false);
		
		final NXCSession session = Registry.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setFilter(filter);
		md.setObjectFlags(checkboxEnableFilter.getSelection() ? NetworkMap.MF_FILTER_OBJECTS : 0, NetworkMap.MF_FILTER_OBJECTS);
		
      new Job(i18n.tr("Updating map object filter"), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
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
			            initialFilter = filter;
			            initialEnable = checkboxEnableFilter.getSelection();
							MapObjectFilter.this.setValid(true);
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot change map object filter");
			}
		}.start();
		
      return true;
	}
}
