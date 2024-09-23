/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectSelectionFilterFactory;
import org.netxms.nxmc.modules.objects.widgets.ObjectTree;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object selection dialog
 */
public class ObjectSelectionDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(ObjectSelectionDialog.class);
	private NXCSession session;
	private ObjectTree objectTree;
	private long[] selectedObjects;
	private Set<Integer> classFilter;
	private boolean multiSelection;
	private boolean showFilterToolTip = true;
	private boolean showFilterCloseButton = false;
   private List<AbstractObject> blockedObjects;
   private String title = i18n.tr("Select Object");

	/**
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public static Set<Integer> createNodeSelectionFilter(boolean allowMobileDevices)
	{
		return ObjectSelectionFilterFactory.getInstance().createNodeSelectionFilter(allowMobileDevices);
	}

	/**
	 * Create filter for zone selection - it allows only zone and entire network obejcts.
	 * 
	 * @return Class filter for zone selection
	 */
	public static Set<Integer> createZoneSelectionFilter()
	{
		return ObjectSelectionFilterFactory.getInstance().createZoneSelectionFilter();
	}
	
	/**
    * Create filter for network map selection.
    * 
    * @return Class filter for network map selection
    */
   public static Set<Integer> createNetworkMapSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createNetworkMapSelectionFilter();
   }

   /**
	 * Create filter for network map groups selection.
	 * 
	 * @return Class filter for network map groups selection
	 */
	public static Set<Integer> createNetworkMapGroupsSelectionFilter()
	{
		return ObjectSelectionFilterFactory.getInstance().createNetworkMapGroupsSelectionFilter();
	}

   /**
    * Create filter for network map groups selection.
    * 
    * @return Class filter for network map groups selection
    */
   public static Set<Integer> createAssetGroupsSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createAssetGroupsSelectionFilter();
   }
	
   /**
    * Create filter for dashboard group selection.
    * 
    * @return Class filter for dashboard group selection
    */
   public static Set<Integer> createDashboardGroupSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createDashboardGroupSelectionFilter();
   }
   
   /**
    * Create filter for dashboard selection.
    * 
    * @return Class filter for dashboard selection
    */
   public static Set<Integer> createDashboardSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createDashboardSelectionFilter();
   }

	/**
	 * Create filter for template selection - it allows template objects and possible
	 * parents - template groups.
	 * 
	 * @return Class filter for node selection
	 */
	public static Set<Integer> createTemplateSelectionFilter()
	{
		return ObjectSelectionFilterFactory.getInstance().createTemplateSelectionFilter();
	}
	
	/**
    * Create filter for template group selection - it template groups.   
    * 
    * @return Class filter for node selection
    */
   public static Set<Integer> createTemplateGroupSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createTemplateGroupSelectionFilter();
   }

	/**
    * Create filter for data collection owner selection.
    * 
    * @return Class filter for data collection owner selection
    */
   public static Set<Integer> createDataCollectionOwnerSelectionFilter()
	{
      return ObjectSelectionFilterFactory.getInstance().createDataCollectionOwnerSelectionFilter();
	}

   /**
    * Create filter for data collection targets selection.
    * 
    * @return Class filter for data collection targets selection
    */
   public static Set<Integer> createDataCollectionTargetSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createDataCollectionTargetSelectionFilter();
   }

	/**
    * Create filter for container selection
    * 
    * @return Class filter for container selection
    */
	public static Set<Integer> createContainerSelectionFilter()
	{
		return ObjectSelectionFilterFactory.getInstance().createContainerSelectionFilter();
	}

   /**
    * Create filter for circuit selection
    * 
    * @return Class filter for circuit selection
    */
   public static Set<Integer> createCircuitSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createCircuitSelectionFilter();
   }

	/**
	 * Create filter for business service selection.
	 * 
	 * @return Class filter for node selection
	 */
	public static Set<Integer> createBusinessServiceSelectionFilter()
	{
		return ObjectSelectionFilterFactory.getInstance().createBusinessServiceSelectionFilter();
	}

	/**
	 * Create filter for dashboard and network map selection
	 * 
	 * @return
	 */
	public static Set<Integer> createDashboardAndNetworkMapSelectionFilter()
	{
	   return ObjectSelectionFilterFactory.getInstance().createDashboardAndNetworkMapSelectionFilter();
	}

	/**
    * Create filter for rack selection
    * 
    * @return
    */
   public static Set<Integer> createRackSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createRackSelectionFilter();
   }

   /**
    * Create filter for rack and chassis selection
    * 
    * @return
    */
   public static Set<Integer> createRackOrChassisSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createRackOrChassisSelectionFilter();
   }

   /**
    * Create filter for interface selection
    * 
    * @return filter for interface selection
    */
   public static Set<Integer> createInterfaceSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createInterfaceSelectionFilter();
   }

   /**
    * Create filter for asset selection
    * 
    * @return
    */
   public static Set<Integer> createAssetSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createAssetSelectionFilter();
   }

   /**
    * Create object selection dialog without class filtering.
    * 
    * @param parentShell parent shell
    */
   public ObjectSelectionDialog(Shell parentShell)
   {
      this(parentShell, null, null);
   }

	/**
    * Create object selection dialog with optional class filtering.
    * 
    * @param parentShell parent shell
    * @param classFilter set of allowed object classes (set to null to show all classes)
    */
   public ObjectSelectionDialog(Shell parentShell, Set<Integer> classFilter)
   {
      this(parentShell, classFilter, null);
   }

	/**
	 * Create object selection dialog.
	 * 
	 * @param parentShell parent shell
	 * @param classFilter set of allowed object classes (set to null to show all classes)
	 * @param currentObject object list selected for move (the same object can't be selected as a move terget)
	 */
   public ObjectSelectionDialog(Shell parentShell, Set<Integer> classFilter, List<AbstractObject> blockedObjects)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.classFilter = classFilter;
		multiSelection = true;
      session = Registry.getSession();
      this.blockedObjects = blockedObjects;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(title);
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsPoint("ObjectSelectionDialog.Size", 400, 350));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
      PreferenceStore settings = PreferenceStore.getInstance();
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);

      objectTree = new ObjectTree(dialogArea, SWT.NONE, multiSelection, classFilter, null, null, showFilterToolTip, showFilterCloseButton);

      String text = settings.getAsString("ObjectSelectionDialog.Filter");
		if (text != null)
			objectTree.setFilterText(text);

		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		objectTree.setLayoutData(gd); 

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		if (multiSelection)
		{
			Long[] objects = objectTree.getSelectedObjects();
			selectedObjects = new long[objects.length];
			for(int i = 0; i < objects.length; i++)
				selectedObjects[i] = objects[i].longValue();
		}
		else
		{
			long objectId = objectTree.getFirstSelectedObject();
			if (objectId != 0)
			{
            if (blockedObjects != null)
            {
               for(int i = 0; i < blockedObjects.size(); i++)
               {
                  if (blockedObjects.get(i).getObjectId() == objectId)
                  {
                     MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
                           i18n.tr("Same object was selected as move source and move target."));
                     return;
                  }
               }
            }
				selectedObjects = new long[1];
				selectedObjects[0] = objectId;
			}
			else
			{
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select object and then press OK."));
				return;
			}
		}
		
		saveSettings();
		super.okPressed();
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();

      settings.set("ObjectSelectionDialog.Size", size);
      settings.set("ObjectSelectionDialog.Filter", objectTree.getFilterText());
	}

	/**
	 * Get selected objects
	 * 
	 * @return
	 */
	public List<AbstractObject> getSelectedObjects()
	{
		if (selectedObjects == null)
			return new ArrayList<AbstractObject>(0);

		return session.findMultipleObjects(selectedObjects, false);
	}

   /**
    * Get selected objects of given type
    * 
    * @param classFilter 
    * @return
    */
   public AbstractObject[] getSelectedObjects(Class<? extends AbstractObject> classFilter)
   {
      Set<Class<? extends AbstractObject>> classFilterSet = new HashSet<Class<? extends AbstractObject>>();
      classFilterSet.add(classFilter);
      return getSelectedObjects(classFilterSet);
   }

	/**
	 * Get selected objects of given type
	 * 
	 * @param classFilter 
	 * @return
	 */
	public AbstractObject[] getSelectedObjects(Set<Class<? extends AbstractObject>> classFilterSet)
	{
		if (selectedObjects == null)
			return new AbstractObject[0];

		final Set<AbstractObject> resultSet = new HashSet<AbstractObject>(selectedObjects.length);
		for(int i = 0; i < selectedObjects.length; i++)
		{
			AbstractObject object = session.findObjectById(selectedObjects[i]);
			for(Class<? extends AbstractObject> cl : classFilterSet)
   			if (cl.isInstance(object))
   				resultSet.add(object);
		}
      return resultSet.toArray(AbstractObject[]::new);
	}

	/**
	 * @return true if multiple object selection is enabled
	 */
	public boolean isMultiSelectionEnabled()
	{
		return multiSelection;
	}

	/**
	 * Enable or disable selection of multiple objects. If multiselection is enabled,
	 * object tree will appear with check boxes, and objects can be selected by checking them.
	 * 
	 * @param enable true to enable multiselection, false to disable
	 */
	public void enableMultiSelection(boolean enable)
	{
		this.multiSelection = enable;
	}
	
	/**
	 * Show or hide filter tooltip (Search by IP, etc..)
	 * @param showFilterToolTip
	 */
	public void showFilterToolTip(boolean showFilterToolTip)
	{
	   this.showFilterToolTip = showFilterToolTip;
	}
	
	/**
    * Show or hide filter tooltip`s close button
    *
    * @param showFilterCloseButton
    */
	public void showFilterCloseButton(boolean showFilterCloseButton)
	{
	   this.showFilterCloseButton = showFilterCloseButton;
	}
	
   /**
    * Set title
    *
    * @param title new title
    */
	public void setTitle(String title)
	{
	   this.title = title;
	}
}
