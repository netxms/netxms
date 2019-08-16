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
package org.netxms.ui.eclipse.objectbrowser.dialogs;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectSelectionFilterFactory;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Object selection dialog
 */
public class ObjectSelectionDialog extends Dialog
{
	private NXCSession session;
	private ObjectTree objectTree;
	private Button checkHideNodeComponents;
	private long[] rootObjects;
	private long[] selectedObjects;
	private Set<Integer> classFilter;
	private boolean multiSelection;
	private boolean showFilterToolTip = true;
	private boolean showFilterCloseButton = false;
	List<Object> currentObject;
	private String title = Messages.get().ObjectSelectionDialog_Title;

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
    * Create filter for policy group selection.
    * 
    * @return Class filter for policy group selection
    */
   public static Set<Integer> createPolicyGroupSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createPolicyGroupSelectionFilter();
   }
   
   /**
    * Create filter for policy selection.
    * 
    * @return Class filter for policy selection
    */
   public static Set<Integer> createPolicySelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createPolicySelectionFilter();
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
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public static Set<Integer> createNodeAndTemplateSelectionFilter(boolean allowMobileDevices)
	{
		return ObjectSelectionFilterFactory.getInstance().createNodeAndTemplateSelectionFilter(allowMobileDevices);
	}

	/**
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public static Set<Integer> createContainerSelectionFilter()
	{
		return ObjectSelectionFilterFactory.getInstance().createContainerSelectionFilter();
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
    * Create filter for rack and chassis selection
    * 
    * @return
    */
   public static Set<Integer> createRackSelectionFilter()
   {
      return ObjectSelectionFilterFactory.getInstance().createRackSelectionFilter();
   }
	
	/**
    * Create object selection dialog.
    * 
    * @param parentShell parent shell
    * @param rootObjects list of root objects (set to null to show entire object tree)
    * @param classFilter set of allowed object classes (set to null to show all classes)
    */
   public ObjectSelectionDialog(Shell parentShell, long[] rootObjects, Set<Integer> classFilter)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.rootObjects = rootObjects;
      this.classFilter = classFilter;
      multiSelection = true;
      session = (NXCSession)ConsoleSharedData.getSession();
      this.currentObject = new ArrayList<Object>();
   }

	/**
	 * Create object selection dialog.
	 * 
	 * @param parentShell parent shell
	 * @param rootObjects list of root objects (set to null to show entire object tree)
	 * @param classFilter set of allowed object classes (set to null to show all classes)
	 * @param currentObject object list selected for move (the same object can't be selected as a move terget)
	 */
	public ObjectSelectionDialog(Shell parentShell, long[] rootObjects, Set<Integer> classFilter, List<Object> currentObject)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.rootObjects = rootObjects;
		this.classFilter = classFilter;
		multiSelection = true;
		session = (NXCSession)ConsoleSharedData.getSession();
		this.currentObject = currentObject;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(title);
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectObject.cx"), settings.getInt("SelectObject.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(400, 350);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets
	 * .Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);

		boolean checked = (settings.get("SelectObject.hideNodeComponents") != null) ? settings.getBoolean("SelectObject.hideNodeComponents") : true; //$NON-NLS-1$  
		objectTree = new ObjectTree(dialogArea, SWT.NONE, multiSelection ? ObjectTree.MULTI : 0, rootObjects, classFilter, showFilterToolTip, showFilterCloseButton, false);
		objectTree.setHideNodeComponent(checked);
		
		String text = settings.get("SelectObject.Filter"); //$NON-NLS-1$
		if (text != null)
			objectTree.setFilter(text);

		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		objectTree.setLayoutData(gd); 
		    
		checkHideNodeComponents = new Button(dialogArea, SWT.CHECK);
		checkHideNodeComponents.setText("Hide node components");
		checkHideNodeComponents.setSelection(checked);
		checkHideNodeComponents.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            objectTree.setHideNodeComponent(checkHideNodeComponents.getSelection());           
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);            
         }
      });		
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.CENTER;
      gd.grabExcessHorizontalSpace = true;
		checkHideNodeComponents.setLayoutData(gd);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/* (non-Javadoc)
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
				selectedObjects = new long[1];
				selectedObjects[0] = objectId;
				for (int i = 0; i < currentObject.size(); i++)
				{
				   if(((AbstractObject)currentObject.get(i)).getObjectId() == objectId)
				   {
				      MessageDialogHelper.openWarning(getShell(), Messages.get().ObjectSelectionDialog_Warning, Messages.get().ObjectSelectionDialog_SameObjecsAstargetAndSourceWarning);
		            return;
				   }
				}
			}
			else
			{
				MessageDialogHelper.openWarning(getShell(), Messages.get().ObjectSelectionDialog_Warning, Messages.get().ObjectSelectionDialog_WarningText);
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
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectObject.cx", size.x); //$NON-NLS-1$
		settings.put("SelectObject.cy", size.y); //$NON-NLS-1$
		settings.put("SelectObject.Filter", objectTree.getFilter()); //$NON-NLS-1$
      settings.put("SelectObject.hideNodeComponents", checkHideNodeComponents.getSelection()); //$NON-NLS-1$
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
		if (selectedObjects == null)
			return new AbstractObject[0];

		final Set<AbstractObject> resultSet = new HashSet<AbstractObject>(selectedObjects.length);
		for(int i = 0; i < selectedObjects.length; i++)
		{
			AbstractObject object = session.findObjectById(selectedObjects[i]);
			if (classFilter.isInstance(object))
				resultSet.add(object);
		}
		return resultSet.toArray(new AbstractObject[resultSet.size()]);
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
    * @param showFilterCloseButton
    */
	public void showFilterCloseButton(boolean showFilterCloseButton)
	{
	   this.showFilterCloseButton = showFilterCloseButton;
	}
	
	public void setTitle(String title)
	{
	   this.title = title;
	}
}
