/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.CompositeWithMessageBar;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.LastValuesComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.LastValuesFilter;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.LastValuesLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Viewer for last values of given object
 */
public class LastValuesWidget extends CompositeWithMessageBar
{
	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_DESCRIPTION = 1;
	public static final int COLUMN_VALUE = 2;
	public static final int COLUMN_TIMESTAMP = 3;
	public static final int COLUMN_THRESHOLD = 4;
	
   private I18n i18n = LocalizationHelper.getI18n(LastValuesWidget.class);
   private final View view;
	private DataCollectionTarget dcTarget;
	private NXCSession session;
	private SessionListener clientListener = null;
	private boolean filterEnabled = true;
	private FilterText filterText;
	private SortableTableViewer dataViewer;
	private LastValuesLabelProvider labelProvider;
	private LastValuesComparator comparator;
	private LastValuesFilter filter;
	private boolean autoRefreshEnabled = false;
	private int autoRefreshInterval = 30;	// in seconds
	private ViewRefreshController refreshController;
	private Action actionUseMultipliers;
	private Action actionShowErrors;
	private Action actionShowDisabled;
	private Action actionShowUnsupported;
   private Action actionShowHidden;
	private Action actionExportToCsv;
	private Action actionCopyToClipboard;
	private Action actionCopyDciName;
	private List<OpenHandlerData> openHandlers = new ArrayList<OpenHandlerData>(0);

	/**
	 * Create "last values" widget
	 * 
	 * @param viewPart owning view part
	 * @param parent parent widget
	 * @param style style
	 * @param _node node to display data for
	 * @param configPrefix configuration prefix for saving/restoring viewer settings
	 * @param validator additional visibility validator (used to suppress unneded refresh calls, may be null)
	 */
   public LastValuesWidget(View view, Composite parent, int style, AbstractObject dcTarget, final String configPrefix,
         VisibilityValidator validator)
	{
		super(parent, style);
      session = Registry.getSession();
      this.view = view;
		setDataCollectionTarget(dcTarget);

		registerOpenHandlers();

		final IDialogSettings ds = Activator.getDefault().getDialogSettings();

      refreshController = new ViewRefreshController(view, -1, new Runnable() {
			@Override
			public void run()
			{
				if (isDisposed())
					return;
				
				getDataFromServer();
			}
		}, validator);
		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
		
      getContent().setLayout(new FormLayout());
		
		// Create filter area
		filterText = new FilterText(getContent(), SWT.NONE);
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				onFilterModify();
			}
		});
		filterText.setCloseAction(new Action() {
			@Override
			public void run()
			{
				enableFilter(false);
			}
		});
		
		// Setup table columns
		final String[] names = { Messages.get().LastValuesWidget_ColID, Messages.get().LastValuesWidget_ColDescr, Messages.get().LastValuesWidget_ColValue, Messages.get().LastValuesWidget_ColTime, Messages.get().LastValuesWidget_ColThreshold };
		final int[] widths = { 70, 250, 150, 120, 150 };
		dataViewer = new SortableTableViewer(getContent(), names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
	
		labelProvider = new LastValuesLabelProvider();
		comparator = new LastValuesComparator();
		filter = new LastValuesFilter();
		dataViewer.setLabelProvider(labelProvider);
		dataViewer.setContentProvider(new ArrayContentProvider());
		dataViewer.setComparator(comparator);
		dataViewer.addFilter(filter);
		WidgetHelper.restoreTableViewerSettings(dataViewer, ds, configPrefix);
		
		dataViewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)dataViewer.getSelection();
				if (selection.size() == 1)
				{
					callOpenObjectHandler((DciValue)selection.getFirstElement());
				}
			}
		});
		
		dataViewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(dataViewer, ds, configPrefix);
				ds.put(configPrefix + ".autoRefresh", autoRefreshEnabled); //$NON-NLS-1$
				ds.put(configPrefix + ".autoRefreshInterval", autoRefreshEnabled); //$NON-NLS-1$
				ds.put(configPrefix + ".useMultipliers", labelProvider.areMultipliersUsed()); //$NON-NLS-1$
				ds.put(configPrefix + ".showErrors", isShowErrors()); //$NON-NLS-1$
				ds.put(configPrefix + ".showDisabled", isShowDisabled()); //$NON-NLS-1$
				ds.put(configPrefix + ".showUnsupported", isShowUnsupported()); //$NON-NLS-1$
            ds.put(configPrefix + ".showHidden", isShowHidden()); //$NON-NLS-1$
			}
		});

		// Setup layout
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterText);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		dataViewer.getTable().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterText.setLayoutData(fd);
		
		try
		{
			autoRefreshInterval = ds.getInt(configPrefix + ".autoRefreshInterval"); //$NON-NLS-1$
			if (autoRefreshInterval >= 1000)
			{
			   // assume it was saved by old version in milliseconds
			   autoRefreshInterval /= 1000;
			}
		}
		catch(NumberFormatException e)
		{
		}
		setAutoRefreshEnabled(ds.getBoolean(configPrefix + ".autoRefresh")); //$NON-NLS-1$
		if (ds.get(configPrefix + ".useMultipliers") != null) //$NON-NLS-1$
			labelProvider.setUseMultipliers(ds.getBoolean(configPrefix + ".useMultipliers")); //$NON-NLS-1$
		else
			labelProvider.setUseMultipliers(true);
		if (ds.get(configPrefix + ".showErrors") != null) //$NON-NLS-1$
		{
			labelProvider.setShowErrors(ds.getBoolean(configPrefix + ".showErrors")); //$NON-NLS-1$
			comparator.setShowErrors(ds.getBoolean(configPrefix + ".showErrors")); //$NON-NLS-1$
		}
		else
		{
			labelProvider.setShowErrors(true);
			comparator.setShowErrors(true);
		}
		filter.setShowDisabled(ds.getBoolean(configPrefix + ".showDisabled")); //$NON-NLS-1$
		filter.setShowUnsupported(ds.getBoolean(configPrefix + ".showUnsupported")); //$NON-NLS-1$
      filter.setShowHidden(ds.getBoolean(configPrefix + ".showHidden")); //$NON-NLS-1$
		
		createActions();
		createPopupMenu();

		if ((validator == null) || validator.isVisible())
		   getDataFromServer();
		
		// Set initial focus to filter input line
		if (filterEnabled)
			filterText.setFocus();
		else
			enableFilter(false);	// Will hide filter area correctly
		
		clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.FORCE_DCI_POLL) &&
                (LastValuesWidget.this.dcTarget != null) &&
                (n.getSubCode() == LastValuesWidget.this.dcTarget.getObjectId()))
               refresh();
         }		   
		};
		
		session.addListener(clientListener);
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionUseMultipliers = new Action(Messages.get().LastValuesWidget_UseMultipliers, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setUseMultipliers(actionUseMultipliers.isChecked());
			}
		};
		actionUseMultipliers.setChecked(areMultipliersUsed());

		actionShowErrors = new Action(Messages.get().LastValuesWidget_ShowErrors, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setShowErrors(actionShowErrors.isChecked());
			}
		};
		actionShowErrors.setChecked(isShowErrors());

		actionShowUnsupported = new Action(Messages.get().LastValuesWidget_ShowUnsupported, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setShowUnsupported(actionShowUnsupported.isChecked());
			}
		};
		actionShowUnsupported.setChecked(isShowUnsupported());
		
		actionShowHidden = new Action("Show hidden", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setShowHidden(actionShowHidden.isChecked());
         }
      };
      actionShowHidden.setChecked(isShowHidden());
		
		actionShowDisabled = new Action(Messages.get().LastValuesWidget_ShowDisabled, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setShowDisabled(actionShowDisabled.isChecked());
			}
		};
		actionShowDisabled.setChecked(isShowDisabled());
		
      actionExportToCsv = new ExportToCsvAction(view, dataViewer, true);
		
		actionCopyToClipboard = new Action(Messages.get().LastValuesWidget_CopyToClipboard, SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySelection();
         }
      };
      
      actionCopyDciName = new Action(i18n.tr("Copy DCI Name"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySelectionDciName();
         }
      };
	}
	
	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(dataViewer.getControl());
		dataViewer.getControl().setMenu(menu);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionCopyToClipboard);
		manager.add(actionCopyDciName);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(actionUseMultipliers);
		manager.add(actionShowErrors);
		manager.add(actionShowDisabled);
		manager.add(actionShowUnsupported);
      manager.add(actionShowHidden);
	}

	/**
	 * Get data from server
	 */
	private void getDataFromServer()
	{
		if (dcTarget == null)
		{
			dataViewer.setInput(new DciValue[0]);
			return;
		}

		final DataCollectionTarget jobTarget = dcTarget;
      Job job = new Job(Messages.get().LastValuesWidget_JobTitle + jobTarget.getObjectName(), viewPart, Activator.PLUGIN_ID, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final DciValue[] data = session.getLastValues(jobTarget.getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!isDisposed() && (dcTarget != null) && (dcTarget.getObjectId() == jobTarget.getObjectId()))
                  {
                     dataViewer.setInput(data);
                     hideMessage();
                  }
               }
            });
         }

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot get DCI values for node %s"), jobTarget.getObjectName());
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * Change data collection target object
	 * 
	 * @param dcTarget new data collection target object
	 */
	public void setDataCollectionTarget(AbstractObject dcTarget)
	{
		this.dcTarget = (dcTarget instanceof DataCollectionTarget) ? (DataCollectionTarget)dcTarget : null;
	}
	
	/**
	 * Refresh view
	 */
	public void refresh()
	{
		getDataFromServer();
	}

	/**
	 * @return the autoRefreshEnabled
	 */
	public boolean isAutoRefreshEnabled()
	{
		return autoRefreshEnabled;
	}

	/**
	 * @param autoRefreshEnabled the autoRefreshEnabled to set
	 */
	public void setAutoRefreshEnabled(boolean autoRefreshEnabled)
	{
		this.autoRefreshEnabled = autoRefreshEnabled;
		refreshController.setInterval(autoRefreshEnabled ? autoRefreshInterval : -1);
	}

	/**
	 * @return the autoRefreshInterval
	 */
	public int getAutoRefreshInterval()
	{
		return autoRefreshInterval;
	}

	/**
	 * @param autoRefreshInterval the autoRefreshInterval to set
	 */
	public void setAutoRefreshInterval(int autoRefreshInterval)
	{
		this.autoRefreshInterval = autoRefreshInterval;
	}
	
	/**
	 * @return the useMultipliers
	 */
	public boolean areMultipliersUsed()
	{
		return (labelProvider != null) ? labelProvider.areMultipliersUsed() : false;
	}

	/**
	 * @param useMultipliers the useMultipliers to set
	 */
	public void setUseMultipliers(boolean useMultipliers)
	{
		if (labelProvider != null)
		{
			labelProvider.setUseMultipliers(useMultipliers);
			if (dataViewer != null)
			{
				dataViewer.refresh(true);
			}
		}
	}

	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	public void enableFilter(boolean enable)
	{
		filterEnabled = enable;
		filterText.setVisible(filterEnabled);
		FormData fd = (FormData)dataViewer.getTable().getLayoutData();
		fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
		getContent().layout();
		if (enable)
			filterText.setFocus();
		else
			setFilter(""); //$NON-NLS-1$
	}

	/**
	 * @return the filterEnabled
	 */
	public boolean isFilterEnabled()
	{
		return filterEnabled;
	}

	/**
	 * Set filter text
	 * 
	 * @param text New filter text
	 */
	public void setFilter(final String text)
	{
		filterText.setText(text);
		onFilterModify();
	}

	/**
	 * Handler for filter modification
	 */
	private void onFilterModify()
	{
		final String text = filterText.getText();
		filter.setFilterString(text);
		dataViewer.refresh(false);
	}
	
	/**
	 * Set action to be executed when user press "Close" button in object filter.
	 * Default implementation will hide filter area without notifying parent.
	 * 
	 * @param action
	 */
	public void setFilterCloseAction(Action action)
	{
		filterText.setCloseAction(action);
	}
	
	/**
	 * @return
	 */
	public boolean isShowErrors()
	{
		return (labelProvider != null) ? labelProvider.isShowErrors() : false;
	}

	/**
	 * @param show
	 */
	public void setShowErrors(boolean show)
	{
		labelProvider.setShowErrors(show);
		comparator.setShowErrors(show);
		if (dataViewer != null)
		{
			dataViewer.refresh(true);
		}
	}
	
	/**
	 * @return
	 */
	public boolean isShowDisabled()
	{
		return (filter != null) ? filter.isShowDisabled() : false;
	}

	/**
	 * @param show
	 */
	public void setShowDisabled(boolean show)
	{
		filter.setShowDisabled(show);
		if (dataViewer != null)
		{
			dataViewer.refresh(true);
		}
	}

	/**
	 * @return
	 */
	public boolean isShowUnsupported()
	{
		return (filter != null) ? filter.isShowUnsupported() : false;
	}

	/**
	 * @param show
	 */
	public void setShowUnsupported(boolean show)
	{
		filter.setShowUnsupported(show);
		if (dataViewer != null)
		{
			dataViewer.refresh(true);
		}
	}

   /**
    * @return
    */
   public boolean isShowHidden()
   {
      return (filter != null) ? filter.isShowHidden() : false;
   }

   /**
    * @param show
    */
   public void setShowHidden(boolean show)
   {
      filter.setShowHidden(show);
      if (dataViewer != null)
      {
         dataViewer.refresh(true);
      }
   }

	/**
    * @return the actionUseMultipliers
    */
   public Action getActionUseMultipliers()
   {
      return actionUseMultipliers;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowDisabled()
   {
      return actionShowDisabled;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowErrors()
   {
      return actionShowErrors;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowUnsupported()
   {
      return actionShowUnsupported;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowHidden()
   {
      return actionShowHidden;
   }

	/**
	 * Copy selection to clipboard
	 */
	private void copySelection()
	{
	   IStructuredSelection selection = (IStructuredSelection)dataViewer.getSelection();
	   if (selection.isEmpty())
	      return;
	   
      final String nl = WidgetHelper.getNewLineCharacters();
	   StringBuilder sb = new StringBuilder();
	   for(Object o : selection.toList())
	   {
	      if (sb.length() > 0)
	         sb.append(nl);
	      DciValue v = (DciValue)o;
	      sb.append(v.getDescription());
	      sb.append(" = "); //$NON-NLS-1$
	      sb.append(v.getValue());
	   }
	   if (selection.size() > 1)
	      sb.append(nl);
	   WidgetHelper.copyToClipboard(sb.toString());
	}
	
	/**
	 * Copy DCI name of selection
	 */
	private void copySelectionDciName()
	{
      IStructuredSelection selection = dataViewer.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      StringBuilder dciName = new StringBuilder();
      for(Object o : selection.toList())
      {
         if (dciName.length() > 0)
            dciName.append(' ');
         DciValue v = (DciValue)o;
         dciName.append(v.getName());
      }
      WidgetHelper.copyToClipboard(dciName.toString());
	}
}
