/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.CreateNewToolDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateObjectDialog;
import org.netxms.nxmc.modules.objects.propertypages.ObjectToolsAccessControl;
import org.netxms.nxmc.modules.objects.propertypages.ObjectToolsColumns;
import org.netxms.nxmc.modules.objects.propertypages.ObjectToolsFilterPropertyPage;
import org.netxms.nxmc.modules.objects.propertypages.ObjectToolsGeneral;
import org.netxms.nxmc.modules.objects.propertypages.ObjectToolsInputFields;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectToolsComparator;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectToolsFilter;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectToolsLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Editor for object tools
 */
public class ObjectToolsEditor extends ConfigurationView implements SessionListener
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectToolsEditor.class);

	private static final String TABLE_CONFIG_PREFIX = "ObjectToolsEditor"; //$NON-NLS-1$
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_TYPE = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	
	private Map<Long, ObjectTool> tools = new HashMap<Long, ObjectTool>();
	private SortableTableViewer viewer;
	private NXCSession session;
	private Action actionRefresh;
	private Action actionNew;
	private Action actionEdit;
   private Action actionClone;
	private Action actionDelete;
	private Action actionDisable;
	private Action actionEnable;

   /**
    * Create object tools editor view
    */
   public ObjectToolsEditor()
   {
      super(i18n.tr("Object Tools"), ResourceManager.getImageDescriptor("icons/config-views/tools.png"), "ObjectTools", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
	{				
		final String[] columnNames = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Description") };
		final int[] columnWidths = { 90, 200, 100, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ObjectToolsLabelProvider());		
		ObjectToolsFilter filter = new ObjectToolsFilter();
		viewer.addFilter(filter);
		setFilterClient(viewer, filter);
		
		viewer.setComparator(new ObjectToolsComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
               actionClone.setEnabled(selection.size() == 1);
					actionDelete.setEnabled(selection.size() > 0);
					actionDisable.setEnabled(containsEnabled(selection));
					actionEnable.setEnabled(containsDisabled(selection));
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionEdit.run();
			}
		});
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
			}
		});

		createActions();
		createContextMenu();
		
		session.addListener(this);		
		refreshToolList();
	}
	
	 /**
	 * @param selection
	 * @return
	 */
	private boolean containsDisabled(IStructuredSelection selection)
    {
       List<?> l = selection.toList();
       for (int i = 0; i < l.size(); i++)
          if ((((ObjectTool)l.get(i)).getFlags() & ObjectTool.DISABLED) > 0)
                return true;
       return false;
    }

    /**
    * @param selection
    * @return
    */
   private boolean containsEnabled(IStructuredSelection selection)
    {
       List<?> l = selection.toList();
       for (int i = 0; i < l.size(); i++)
          if ((((ObjectTool)l.get(i)).getFlags() & ObjectTool.DISABLED) == 0)
             return true;
       return false;
    }

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refreshToolList();
			}
		};
		
		actionNew = new Action(i18n.tr("&New...")) {
			@Override
			public void run()
			{
				createTool();
			}
		};
		actionNew.setImageDescriptor(SharedIcons.ADD_OBJECT);
      addKeyBinding("M1+N", actionNew);

		actionEdit = new Action(i18n.tr("&Edit...")) {
			@Override
			public void run()
			{
			   editTool();
			}
		};
		actionEdit.setImageDescriptor(SharedIcons.EDIT);
      addKeyBinding("M3+ENTER", actionEdit);
		
		actionDelete = new Action(i18n.tr("&Delete")) {
			@Override
			public void run()
			{
				deleteTools();
			}
		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
      addKeyBinding("M1+D", actionDelete);
		
		actionDisable = new Action(i18n.tr("Disable")) {
         @Override
         public void run()
         {
            disableSelectedTools();
         }
      };
      addKeyBinding("M1+I", actionDisable);
      
      actionEnable = new Action(i18n.tr("Enable")) {
         @Override
         public void run()
         {
            enableSelectedTools();
         }
      };
      addKeyBinding("M1+E", actionEnable);
      
      actionClone = new Action(i18n.tr("Clone")) {
         @Override
         public void run()
         {
            cloneTool();
         }
      };
	}
	
	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
      manager.add(actionDelete);
      manager.add(actionEdit);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionDelete);
      manager.add(actionClone);
      manager.add(actionEdit);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createContextMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionNew);
		mgr.add(actionDelete);
      mgr.add(actionClone);  
		mgr.add(new Separator());
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (containsEnabled(selection))
      {
         mgr.add(actionDisable);
      }
      if (containsDisabled(selection))
      {
         mgr.add(actionEnable);
      }
		mgr.add(new Separator());
      mgr.add(actionEdit);   
	}

	/**
	 * Refresh tool list
	 */
	private void refreshToolList()
	{
		new Job(i18n.tr("Get object tools configuration"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<ObjectTool> tl = session.getObjectTools();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						tools.clear();
						for(ObjectTool t : tl)
							tools.put(t.getId(), t);
						viewer.setInput(tools.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get object tools configuration");
			}
		}.start();
	}
	
	/**
	 * Edit existing object tool 
	 */
	private void editTool()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      new Job(i18n.tr("Get object tool details"), this) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            ObjectToolDetails objectToolDetails = session.getObjectToolDetails(((ObjectTool)selection.getFirstElement()).getId());
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  if (showObjectToolPropertyPages(objectToolDetails))
                  {
                     saveObjectTool(objectToolDetails);
                  }                  
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to load object tool details");
         }
      }.start();      
	}
	
	/**
	 * Create new tool
	 */
	private void createTool()
	{
		final CreateNewToolDialog dlg = new CreateNewToolDialog(getWindow().getShell());
		if (dlg.open() == Window.OK)
		{
			new Job(i18n.tr("Generate new object tool ID"), this) {
				@Override
				protected void run(IProgressMonitor monitor) throws Exception
				{
					final long toolId = session.generateObjectToolId();
					final ObjectToolDetails details = new ObjectToolDetails(toolId, dlg.getType(), dlg.getName());
					session.modifyObjectTool(details);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
						   if (showObjectToolPropertyPages(details))
			            {
	                     if (details.isModified())
	                        saveObjectTool(details);
			            }
						}
					});
				}

				@Override
				protected String getErrorMessage()
				{
					return i18n.tr("Cannot generate object tool ID");
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected tools
	 */
	private void deleteTools()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
			return;
		
		if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirmation"), i18n.tr("Do you really want to delete selected tools?")))
			return;
		
		final Object[] objects = selection.toArray();
		new Job(i18n.tr("Delete objecttools"), this) {
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot delete object tool");
			}

			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
					session.deleteObjectTool(((ObjectTool)objects[i]).getId());
				}
			}
		}.start();
	}
	
	/**
	 * Save object tool configuration on server
	 * 
	 * @param details object tool details
	 */
	private void saveObjectTool(final ObjectToolDetails details)
	{
		new Job(i18n.tr("Save object tool"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObjectTool(details);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot save object tool configuration");
			}
		}.start();
	}

   /**
    * Disable selected object tools
    */
   private void disableSelectedTools()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirmation"),
            i18n.tr("Are you sure you want to disable this Object Tool?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("DisableObject Tool"), this) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("It is not possible to disable this object tool.");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               if ((((ObjectTool)objects[i]).getFlags() & ObjectTool.DISABLED) == 0)
                  session.changeObjecToolDisableStatuss(((ObjectTool)objects[i]).getId(), false);
            }
         }
      }.start();
   }

   /**
    * Enable selected object tools
    */
   private void enableSelectedTools()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirmation"),
            i18n.tr("Are you sure you want to enable this Object Tool?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Enable Object Tool"), this) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("It is not possible to enable this object tool.");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               if ((((ObjectTool)objects[i]).getFlags() & ObjectTool.DISABLED) > 0)
                  session.changeObjecToolDisableStatuss(((ObjectTool)objects[i]).getId(), true);
            }
         }
      }.start();
   }

   /**
    * Clone object tool
    */
   private void cloneTool()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final CreateObjectDialog dlg = new CreateObjectDialog(getWindow().getShell(), i18n.tr("Object tool"));
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Clone object tool"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final long toolId = session.generateObjectToolId();
               ObjectTool objTool = (ObjectTool)selection.toArray()[0];
               ObjectToolDetails details = session.getObjectToolDetails(objTool.getId());
               details.setId(toolId);
               details.setName(dlg.getObjectName());
               session.modifyObjectTool(details);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot clone object tool");
            }
         }.start();
      }
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		switch(n.getCode())
		{
			case SessionNotification.OBJECT_TOOLS_CHANGED:
				refreshToolList();
				break;
			case SessionNotification.OBJECT_TOOL_DELETED:
	         getDisplay().asyncExec(new Runnable() {
	            @Override
	            public void run()
	            {
                  tools.remove(n.getSubCode());
                  viewer.setInput(tools.values().toArray());
	            }
	         });
				break;
		}
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
	   
   /**
    * Show Object tools configuration dialog
    * 
    * @param trap Object tool details object
    * @return true if OK was pressed
    */
   private boolean showObjectToolPropertyPages(final ObjectToolDetails objectTool)
   {
      PreferenceManager pm = new PreferenceManager();    
      pm.addToRoot(new PreferenceNode("general", new ObjectToolsGeneral(objectTool)));
      pm.addToRoot(new PreferenceNode("access_control", new ObjectToolsAccessControl(objectTool)));
      pm.addToRoot(new PreferenceNode("filter", new ObjectToolsFilterPropertyPage(objectTool)));
      pm.addToRoot(new PreferenceNode("input_fields", new ObjectToolsInputFields(objectTool)));
      if (objectTool.getToolType() == ObjectTool.TYPE_AGENT_LIST || objectTool.getToolType() == ObjectTool.TYPE_SNMP_TABLE)
         pm.addToRoot(new PreferenceNode("columns", new ObjectToolsColumns(objectTool)));
      
      PreferenceDialog dlg = new PreferenceDialog(getWindow().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText("Properties for " + objectTool.getCommandDisplayName());
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }

   @Override
   public boolean isModified()
   {
      return false;
   }

   @Override
   public void save()
   {
   }
}
