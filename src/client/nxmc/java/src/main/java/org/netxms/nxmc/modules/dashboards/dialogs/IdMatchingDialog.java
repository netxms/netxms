/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.dialogs;

import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.DashboardImporter;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.DciIdMatchingData;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.IdMatchingContentProvider;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.IdMatchingLabelProvider;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectNodeDciDialog;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for node and DCI ID matching
 */
public class IdMatchingDialog extends Dialog
{
	public static final int COLUMN_SOURCE_ID = 0;
	public static final int COLUMN_SOURCE_NAME = 1;
	public static final int COLUMN_DESTINATION_ID = 2;
	public static final int COLUMN_DESTINATION_NAME = 3;

   private final I18n i18n = LocalizationHelper.getI18n(IdMatchingDialog.class);

	private SortableTreeViewer viewer;
	private Map<Long, ObjectIdMatchingData> objects;
	private Map<Long, DciIdMatchingData> dcis;
	private Action actionMap;
	private boolean showFilterToolTip;

	/**
	 * @param parentShell
	 * @param objects
	 * @param dcis
	 */
	public IdMatchingDialog(Shell parentShell, Map<Long, ObjectIdMatchingData> objects, Map<Long, DciIdMatchingData> dcis)
	{
		super(parentShell);
		this.objects = objects;
		this.dcis = dcis;
      setShellStyle(getShellStyle() | SWT.RESIZE);
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("ID Mapping Editor"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);

		Label label = new Label(dialogArea, SWT.WRAP);
		label.setText(i18n.tr("Please check that source objects and data collection items correctly mapped to destination system. Incorrect mappings can be changed by selecting \"Map to...\" from appropriate item's context menu. When done, press \"OK\" to continue dashboard import or \"Cancel\" to cancel import process."));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 800;
		label.setLayoutData(gd);

      final String[] names = { i18n.tr("Original ID"), i18n.tr("Original Name"), i18n.tr("Match ID"), i18n.tr("Match Name") };
		final int[] widths = { 100, 300, 80, 300 };
		viewer = new SortableTreeViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.SINGLE | SWT.FULL_SELECTION);
		viewer.getTree().setLinesVisible(true);
		viewer.getTree().setHeaderVisible(true);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
		viewer.getControl().setLayoutData(gd);
		viewer.setContentProvider(new IdMatchingContentProvider());
		viewer.setLabelProvider(new IdMatchingLabelProvider());
		viewer.setInput(new Object[] { objects, dcis });
		viewer.expandAll();

		createActions();
		createPopupMenu();

		return dialogArea;
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionMap = new Action(i18n.tr("&Map to..."), ResourceManager.getImageDescriptor("icons/sync.png")) {
			@Override
			public void run()
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
				if (selection.size() != 1)
					return;

				Object element = selection.getFirstElement();
				if (element instanceof ObjectIdMatchingData)
					mapNode((ObjectIdMatchingData)element);
				if (element instanceof DciIdMatchingData)
					mapDci((DciIdMatchingData)element);
			}
		};
	}

	/**
	 * Create context menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> m.add(actionMap));

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Map node
	 * 
	 * @param data
	 */
	private void mapNode(ObjectIdMatchingData data)
	{
		Set<Integer> classFilter;
		switch(data.objectClass)
		{
         case AbstractObject.OBJECT_ACCESSPOINT:
         case AbstractObject.OBJECT_CHASSIS:
         case AbstractObject.OBJECT_CIRCUIT:
         case AbstractObject.OBJECT_CLUSTER:
         case AbstractObject.OBJECT_MOBILEDEVICE:
			case AbstractObject.OBJECT_NODE:
         case AbstractObject.OBJECT_SENSOR:
            classFilter = ObjectSelectionDialog.createDataCollectionTargetSelectionFilter();
				showFilterToolTip = true;
				break;
         case AbstractObject.OBJECT_COLLECTOR:
			case AbstractObject.OBJECT_CONTAINER:
				classFilter = ObjectSelectionDialog.createContainerSelectionFilter();
				showFilterToolTip = false;
				break;
			case AbstractObject.OBJECT_DASHBOARD:
            classFilter = ObjectSelectionDialog.createDashboardSelectionFilter();
				break;
         case AbstractObject.OBJECT_ZONE:
            classFilter = ObjectSelectionDialog.createZoneSelectionFilter();
            showFilterToolTip = true;
            break;
			default:
				classFilter = null;
				break;
		}
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), classFilter);
		dlg.showFilterToolTip(showFilterToolTip);
		if (dlg.open() == Window.OK)
		{
			AbstractObject object = dlg.getSelectedObjects().get(0);
			if (DashboardImporter.isCompatibleClasses(object.getObjectClass(), data.objectClass))
			{
				data.dstId = object.getObjectId();
				data.dstName = object.getObjectName();

				updateDciMapping(data);
				viewer.update(data, null);
			}
			else
			{
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Target object must be of same class as source object."));
			}
		}
	}

	/**
	 * Update mapping for all DCIs of given node after node mapping change
	 * 
	 * @param objData
	 */
	private void updateDciMapping(final ObjectIdMatchingData objData)
	{
		if (objData.dcis.size() == 0)
			return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get DCI information from node"), null) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final DciValue[] dciValues = session.getLastValues(objData.dstId);
            runInUIThread(() -> {
               for(DciIdMatchingData d : objData.dcis)
					{
                  d.dstNodeId = objData.dstId;
                  d.dstDciId = -1;
                  d.dstName = null;
                  for(DciValue v : dciValues)
						{
                     if (v.getDescription().equalsIgnoreCase(d.srcName))
							{
                        d.dstDciId = v.getId();
                        d.dstName = v.getDescription();
                        break;
							}
						}
					}
               viewer.refresh(true);
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot get DCI information from node");
			}
		}.start();
	}

	/**
	 * Map DCI
	 * 
	 * @param data
	 */
	private void mapDci(DciIdMatchingData data)
	{
		if (data.dstNodeId == 0)
			return;

		SelectNodeDciDialog dlg = new SelectNodeDciDialog(getShell(), data.dstNodeId);
		if (dlg.open() == Window.OK)
		{
			DciValue v = dlg.getSelection();
			if (v != null)
			{
				data.dstDciId = v.getId();
				data.dstName = v.getDescription();
				viewer.update(data, null);
			}
		}
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		// check if all elements have a match
		boolean ok = true;
		for(ObjectIdMatchingData o : objects.values())
      {
         if (o.dstId == AbstractObject.UNKNOWN)
			{
				ok = false;
				break;
			}
      }
		for(DciIdMatchingData d : dcis.values())
      {
         if ((d.dstNodeId == AbstractObject.UNKNOWN) || (d.dstDciId == 0))
			{
				ok = false;
				break;
			}
      }

		if (!ok)
		{
         if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Matching Errors"),
               i18n.tr("Not all elements was matched correctly to destination system. Imported dashboard will not behave correctly. Are you sure to continue dashboard import?")))
				return;
		}

		super.okPressed();
	}
}
