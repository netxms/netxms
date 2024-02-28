/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.ObjectViewerFilter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog used to select related object(s) of given object
 */
public class RelatedObjectSelectionDialog extends Dialog
{
   public enum RelationType
   {
      DIRECT_SUBORDINATES, DIRECT_SUPERORDINATES
   }

   private final I18n i18n = LocalizationHelper.getI18n(RelatedObjectSelectionDialog.class);

   private Set<Long> seedObjectSet = null;
   private RelationType relationType;
	private Set<Integer> classFilter;
	private ObjectViewerFilter filter;
   private FilterText filterText;
	private TableViewer objectList;
	private List<AbstractObject> selectedObjects;
	private boolean showObjectPath;
	
	/**
	 * Create class filter from array of allowed classes.
	 * 
	 * @param classes
	 * @return
	 */
	public static Set<Integer> createClassFilter(int[] classes)
	{
		Set<Integer> filter = new HashSet<Integer>(classes.length);
		for(int i = 0; i < classes.length; i++)
			filter.add(classes[i]);
		return filter;
	}

	/**
	 * Create class filter with single allowed class
	 * 
	 * @param c allowed class
	 * @return
	 */
	public static Set<Integer> createClassFilter(int c)
	{
		Set<Integer> filter = new HashSet<Integer>(1);
		filter.add(c);
		return filter;
	}
	
	/**
    * @param parentShell parent shell
    * @param seedObject object used for populating related object list
    * @param relationType relation type
    * @param classFilter class filter for object list
    */
   public RelatedObjectSelectionDialog(Shell parentShell, long seedObject, RelationType relationType, Set<Integer> classFilter)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		seedObjectSet = new HashSet<Long>();
		seedObjectSet.add(seedObject);
      this.relationType = relationType;
		this.classFilter = classFilter;
	}
   
   /**
    * @param parentShell parent shell
    * @param seedObjectSet object set used for populating related object list
    * @param relationType relation type
    * @param classFilter class filter for object list
    */
   public RelatedObjectSelectionDialog(Shell parentShell, Set<Long> seedObjectSet, RelationType relationType, Set<Integer> classFilter)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.seedObjectSet = seedObjectSet;
      this.relationType = relationType;
      this.classFilter = classFilter;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText((relationType == RelationType.DIRECT_SUBORDINATES) ? i18n.tr("Select Subordinate Object") : i18n.tr("Select Parent Object"));
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger("RelatedObjectSelectionDialog.cx", 350), settings.getAsInteger("RelatedObjectSelectionDialog.cy", 400));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
      Set<AbstractObject> sourceSet = new HashSet<AbstractObject>();
      for(Long seed : seedObjectSet)
      {
         AbstractObject object = Registry.getSession().findObjectById(seed);
         if (object != null)
         {
            sourceSet.addAll((relationType == RelationType.DIRECT_SUBORDINATES) ? Arrays.asList(object.getChildrenAsArray()) : Arrays.asList(object.getParentsAsArray()));
         }
      }
      AbstractObject[] sourceObjects = sourceSet.toArray(AbstractObject[]::new);

		GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginHeight = 0;
      dialogLayout.marginWidth = 0;
      dialogArea.setLayout(dialogLayout);

      createAdditionalControls(dialogArea);

      Composite listArea = new Composite(dialogArea, SWT.BORDER);
      listArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      listArea.setLayout(layout);

      filterText = new FilterText(listArea, SWT.NONE, null, false, false);
		filterText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				objectList.refresh(false);
				AbstractObject obj = filter.getLastMatch();
				if (obj != null)
				{
					objectList.setSelection(new StructuredSelection(obj), true);
					objectList.reveal(obj);
				}
			}
		});

		// Create object list
      objectList = new TableViewer(listArea, SWT.FULL_SELECTION | SWT.MULTI);
		objectList.getTable().setHeaderVisible(false);
		objectList.setContentProvider(new ArrayContentProvider());
      objectList.setLabelProvider(new DecoratingObjectLabelProvider((object) -> objectList.update(object, null), showObjectPath));
		objectList.setComparator(new ViewerComparator());
      filter = new ObjectViewerFilter(sourceObjects, classFilter);
		objectList.addFilter(filter);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		objectList.getControl().setLayoutData(gd);

      objectList.setInput(sourceObjects);

      objectList.getControl().setFocus();

		return dialogArea;
	}
	
	/**
    * Create additional fields above object list. Default implementation does nothing.
    * 
    * @param parent parent composite
    */
	protected void createAdditionalControls(Composite parent)
	{
	}

   /**
    * @return the showObjectPath
    */
   public boolean isShowObjectPath()
   {
      return showObjectPath;
   }

   /**
    * @param showObjectPath the showObjectPath to set
    */
   public void setShowObjectPath(boolean showObjectPath)
   {
      this.showObjectPath = showObjectPath;
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
	@SuppressWarnings("rawtypes")
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)objectList.getSelection();
		selectedObjects = new ArrayList<AbstractObject>(selection.size());
		Iterator it = selection.iterator();
		while(it.hasNext())
			selectedObjects.add((AbstractObject)it.next());
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
      settings.set("RelatedObjectSelectionDialog.cx", size.x);
      settings.set("RelatedObjectSelectionDialog.cy", size.y);
	}

	/**
	 * @return the selectedObjects
	 */
	public List<AbstractObject> getSelectedObjects()
	{
		return selectedObjects;
	}
}
