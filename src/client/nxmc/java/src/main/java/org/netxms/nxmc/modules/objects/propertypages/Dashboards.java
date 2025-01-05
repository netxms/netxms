/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Dashboards and Maps" property page for NetXMS objects
 */
public class Dashboards extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(Dashboards.class);

	public static final int COLUMN_NAME = 0;
	
	private SortableTableViewer viewer;
	private Button addButton;
	private Button deleteButton;
	private HashMap<Long, AbstractObject> dashboards = new HashMap<Long, AbstractObject>(0);
	private boolean isModified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public Dashboards(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Dashboards.class).tr("Dashboards and Maps"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "dashboards";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof DataCollectionTarget) || (object instanceof Condition) || (object instanceof Container) || (object instanceof EntireNetwork) || (object instanceof ServiceRoot) ||
            (object instanceof Subnet) || (object instanceof Zone) || (object instanceof Rack) || (object instanceof WirelessDomain);
   }

   /**
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
      
      final String[] columnNames = { i18n.tr("Dashboard / Map") };
      final int[] columnWidths = { 300 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DecoratingObjectLabelProvider((object) -> viewer.update(object, null)));
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));

      for(AbstractObject d : object.getDashboards(false))
   		dashboards.put(d.getObjectId(), d);
      for(AbstractObject m : object.getNetworkMaps(false))
         dashboards.put(m.getObjectId(), m);
      viewer.setInput(dashboards.values().toArray());

      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      viewer.getControl().setLayoutData(gridData);

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gridData);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createDashboardAndNetworkMapSelectionFilter());
				dlg.showFilterToolTip(false);
				if (dlg.open() == Window.OK)
				{
               Set<Class<? extends AbstractObject>> classFilter = new HashSet<>();
               classFilter.add(Dashboard.class);
               classFilter.add(NetworkMap.class);
               AbstractObject[] d = dlg.getSelectedObjects(classFilter);
			      for(int i = 0; i < d.length; i++)
			      	dashboards.put(d[i].getObjectId(), d[i]);
			      viewer.setInput(dashboards.values().toArray());
			      isModified = true;
				}
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
		
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@SuppressWarnings("unchecked")
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
				Iterator<AbstractObject> it = selection.iterator();
				if (it.hasNext())
				{
					while(it.hasNext())
					{
						AbstractObject object = it.next();
						dashboards.remove(object.getObjectId());
					}
			      viewer.setInput(dashboards.values().toArray());
			      isModified = true;
				}
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      
		return dialogArea;
	}
	
   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		if (!isModified)
         return true; // Nothing to apply
		
		if (isApply)
			setValid(false);
		
      final NXCSession session = Registry.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		Set<Long> idList = dashboards.keySet();
		Long[] dlist = new Long[idList.size()];
		int i = 0;
		for(Long id : idList)
			dlist[i++] = id;
		md.setDashboards(dlist);

      new Job(i18n.tr("Updating dashboard list for object {0}", object.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> Dashboards.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot update dashboard list for object %s"), object.getObjectName());
			}
      }.start();
      return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		dashboards.clear();
		viewer.setInput(new AbstractObject[0]);
		isModified = true;
		super.performDefaults();
	}
}
