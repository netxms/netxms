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
package org.netxms.ui.eclipse.dashboard.propertypages;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.UnknownObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.StatusIndicatorElementEditDialog;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig.StatusIndicatorElementConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * DCI list editor for dashboard element
 */
public class StatusIndicatorElements extends PropertyPage
{
	public static final int COLUMN_POSITION = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_OBJECT = 2;
   public static final int COLUMN_METRIC = 3;
   public static final int COLUMN_LABEL = 4;
   public static final int COLUMN_DRILLDOWN_OBJECT = 5;

   private StatusIndicatorConfig config;
   private List<StatusIndicatorElementConfig> elements;
   private ElementLabelProvider labelProvider;
   private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = getElement().getAdapter(StatusIndicatorConfig.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);

      elements = new ArrayList<>();
      for(StatusIndicatorElementConfig e : config.getElements())
         elements.add(new StatusIndicatorElementConfig(e));

      labelProvider = new ElementLabelProvider();
      labelProvider.resolveDciNames(elements);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      final String[] columnNames = { Messages.get().DataSources_Pos, "Type", "Object/Tag", Messages.get().DataSources_Parameter, Messages.get().DataSources_Label, "Drilldown Object" };
      final int[] columnWidths = { 40, 90, 200, 200, 150, 200 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(labelProvider);
      viewer.disableSorting();
      viewer.setInput(elements);

      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      gridData.horizontalSpan = 2;
      viewer.getControl().setLayoutData(gridData);

      /* buttons on left side */
      Composite leftButtons = new Composite(dialogArea, SWT.NONE);
      GridLayout buttonLayout = new GridLayout();
      buttonLayout.numColumns = 2;
      buttonLayout.marginWidth = 0;
      leftButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gridData);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(Messages.get().DataSources_Up);
      GridData gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      upButton.setLayoutData(gd);
      upButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveUp();
			}
      });
      upButton.setEnabled(false);

      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(Messages.get().DataSources_Down);
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      downButton.setLayoutData(gd);
      downButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveDown();
			}
      });
      downButton.setEnabled(false);

      /* buttons on right side */
      Composite rightButtons = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new GridLayout();
      buttonLayout.numColumns = 3;
      buttonLayout.marginWidth = 0;
      rightButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gridData);

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText(Messages.get().DataSources_Add);
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      addButton.setLayoutData(gd);
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addElement();
			}
      });

      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText(Messages.get().DataSources_Modify);
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      editButton.setLayoutData(gd);
      editButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editElement();
			}
      });
      editButton.setEnabled(false);

      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText(Messages.get().DataSources_Delete);
      gd = new GridData();
      gd.minimumWidth = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.grabExcessHorizontalSpace = true;
      deleteButton.setLayoutData(gd);
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteElements();
			}
      });
      deleteButton.setEnabled(false);

      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
            editElement();
			}
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
				editButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
				upButton.setEnabled(selection.size() == 1);
				downButton.setEnabled(selection.size() == 1);
			}
		});

		return dialogArea;
	}

	/**
    * Add new element
    */
	private void addElement()
	{
      StatusIndicatorElementEditDialog dlg = new StatusIndicatorElementEditDialog(getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      StatusIndicatorElementConfig element = dlg.getElement();
      elements.add(element);
      if (element.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI)
         labelProvider.addCacheEntry(element.getObjectId(), element.getDciId(), dlg.getCachedDciName());
      viewer.refresh();
	}

	/**
    * Edit selected element
    */
	private void editElement()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      StatusIndicatorElementConfig element = (StatusIndicatorElementConfig)selection.getFirstElement();
		if (element == null)
			return;

      StatusIndicatorElementEditDialog dlg = new StatusIndicatorElementEditDialog(getShell(), element);
		if (dlg.open() == Window.OK)
		{
         if (element.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI)
            labelProvider.addCacheEntry(element.getObjectId(), element.getDciId(), dlg.getCachedDciName());
			viewer.update(element, null);
		}
	}

	/**
    * Delete selected element(s)
    */
	private void deleteElements()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		for(Object o : selection.toList())
         elements.remove(o);
      viewer.refresh();
	}

	/**
	 * Move selected item up 
	 */
	private void moveUp()
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() == 1)
		{
			Object element = selection.getFirstElement();
         int index = elements.indexOf(element);
			if (index > 0)
			{
            Collections.swap(elements, index - 1, index);
            viewer.refresh();
			}
		}
	}
	
	/**
	 * Move selected item down
	 */
	private void moveDown()
	{
      final IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() == 1)
		{
			Object element = selection.getFirstElement();
			int index = elements.indexOf(element);
			if ((index < elements.size() - 1) && (index >= 0))
			{
				Collections.swap(elements, index + 1, index);
            viewer.refresh();
			}
		}
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
      config.setElements(elements.toArray(new StatusIndicatorElementConfig[elements.size()]));
		return true;
	}

   /**
    * Label provider for status indicator elements
    */
   private class ElementLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private NXCSession session = ConsoleSharedData.getSession();
      private Image imageScript = Activator.getImageDescriptor("icons/script.png").createImage();
      private Image imageDCI = Activator.getImageDescriptor("icons/dci.png").createImage();
      private WorkbenchLabelProvider objectLabelProvider = new WorkbenchLabelProvider();
      private Map<Long, DciInfo> dciNameCache = new HashMap<Long, DciInfo>();

      /**
       * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
       */
      @Override
      public Image getImage(Object element)
      {
         StatusIndicatorElementConfig e = (StatusIndicatorElementConfig)element;
         switch(e.getType())
         {
            case StatusIndicatorConfig.ELEMENT_TYPE_DCI:
               return imageDCI;
            case StatusIndicatorConfig.ELEMENT_TYPE_OBJECT:
               AbstractObject object = session.findObjectById(e.getObjectId());
               return objectLabelProvider.getImage((object != null) ? object : new UnknownObject(e.getObjectId(), session));
            case StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT:
               return imageScript;
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
       */
      @Override
      public String getText(Object element)
      {
         StatusIndicatorElementConfig e = (StatusIndicatorElementConfig)element;
         switch(e.getType())
         {
            case StatusIndicatorConfig.ELEMENT_TYPE_DCI:
               String name = (e.getDciId() != 0) ? dciNameCache.get(e.getDciId()).displayName : e.getDciName();
               if (name == null)
                  name = Messages.get().DciListLabelProvider_Unresolved;
               return ((e.getObjectId() == AbstractObject.CONTEXT) ? "<context>" : session.getObjectName(e.getObjectId())) + " / " + ((e.getDciId() != 0) ? name : e.getDciName());
            case StatusIndicatorConfig.ELEMENT_TYPE_OBJECT:
               return (e.getObjectId() == AbstractObject.CONTEXT) ? "<context>" : session.getObjectName(e.getObjectId());
            case StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT:
               return e.getTag();
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return (columnIndex == 0) ? getImage(element) : null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         StatusIndicatorElementConfig e = (StatusIndicatorElementConfig)element;
         switch(columnIndex)
         {
            case COLUMN_DRILLDOWN_OBJECT:
               return (e.getDrilldownObjectId() != 0) ? session.getObjectName(e.getDrilldownObjectId()) : "";
            case COLUMN_LABEL:
               return e.getLabel();
            case COLUMN_METRIC:
               if (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI)
               {
                  String name = dciNameCache.get(e.getDciId()).displayName;
                  return (name != null) ? name : Messages.get().DciListLabelProvider_Unresolved;
               }
               if (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE)
               {
                  return e.getDciName();
               }
               return null;
            case COLUMN_OBJECT:
               if (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT)
                  return e.getTag();
               return (e.getObjectId() == AbstractObject.CONTEXT) ? "<context>" : session.getObjectName(e.getObjectId());
            case COLUMN_POSITION:
               return Integer.toString(elements.indexOf(e) + 1);
            case COLUMN_TYPE:
               switch(e.getType())
               {
                  case StatusIndicatorConfig.ELEMENT_TYPE_DCI:
                     return "DCI";
                  case StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE:
                     return "DCI Template";
                  case StatusIndicatorConfig.ELEMENT_TYPE_OBJECT:
                     return "Object";
                  case StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT:
                     return "Script";
               }
               return null;
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
       */
      @Override
      public void dispose()
      {
         imageScript.dispose();
         imageDCI.dispose();
         objectLabelProvider.dispose();
      }

      /**
       * Resolve DCI names for given collection of condition DCIs and add to cache
       * 
       * @param dciList
       */
      public void resolveDciNames(final Collection<StatusIndicatorElementConfig> elementList)
      {
         new ConsoleJob(Messages.get().DciListLabelProvider_JobTitle, null, Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               final Map<Long, DciInfo> names = session.dciIdsToNames(elementList);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     dciNameCache = names;
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().DciListLabelProvider_JobError;
            }
         }.runInForeground();
      }

      /**
       * Add single cache entry
       * 
       * @param nodeId
       * @param dciId
       * @param name
       */
      public void addCacheEntry(long nodeId, long dciId, String name)
      {
         if (dciId != 0)
            dciNameCache.put(dciId, new DciInfo("", name, ""));
      }
   }
}
