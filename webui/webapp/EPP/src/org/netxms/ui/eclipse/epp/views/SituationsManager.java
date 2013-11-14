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
package org.netxms.ui.eclipse.epp.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.situations.Situation;
import org.netxms.client.situations.SituationInstance;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.propertypages.helpers.AttributeLabelProvider;
import org.netxms.ui.eclipse.epp.views.helpers.AttributeComparator;
import org.netxms.ui.eclipse.epp.views.helpers.SituationTreeContentProvider;
import org.netxms.ui.eclipse.epp.views.helpers.SituationTreeLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Situation manager
 */
public class SituationsManager extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.epp.views.SituationsManager"; //$NON-NLS-1$
	
	private NXCSession session;
	private Map<Long, Situation> situations = new HashMap<Long, Situation>();
	private TreeViewer situationTree;
	private SortableTableViewer details;
	
	private Action actionRefresh;
	private Action actionCreate;
	private Action actionDelete;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		super.init(site);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		
		final SashForm splitter = new SashForm(parent, SWT.HORIZONTAL);
		
		situationTree = new TreeViewer(splitter, SWT.BORDER | SWT.MULTI);
		situationTree.setContentProvider(new SituationTreeContentProvider());
		situationTree.setLabelProvider(new SituationTreeLabelProvider());
		situationTree.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)situationTree.getSelection();
				if (!selection.isEmpty())
				{
					Object object = selection.getFirstElement();
					if (object instanceof SituationInstance)
					{
						details.setInput(((SituationInstance)object).getAttributes().entrySet().toArray());
					}
					else
					{
						details.setInput(new Object[0]);
					}
				}
				else
				{
					details.setInput(new Object[0]);
				}
			}
		});
		
		final String[] names = { Messages.get().SituationsManager_ColAttribute, Messages.get().SituationsManager_ColValue };
		final int[] widths = { 150, 150 };
		details = new SortableTableViewer(splitter, names, widths, 0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION);
		details.setContentProvider(new ArrayContentProvider());
		details.setLabelProvider(new AttributeLabelProvider());
		details.setComparator(new AttributeComparator());
		details.getTable().setHeaderVisible(true);
		details.getTable().setLinesVisible(true);
		WidgetHelper.restoreTableViewerSettings(details, settings, "SituationInstanceDetails"); //$NON-NLS-1$
		details.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(details, settings, "SituationInstanceDetails"); //$NON-NLS-1$
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session.addListener(this);
		
		int[] weights = new int[] { 50, 50 };
		weights[0] = safeGetInt(settings, "SituationManager.weights.0", weights[0]); //$NON-NLS-1$
		weights[1] = safeGetInt(settings, "SituationManager.weights.1", weights[1]); //$NON-NLS-1$
		splitter.setWeights(weights);
		splitter.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				int[] weights = splitter.getWeights();
				settings.put("SituationManager.weights.0", weights[0]); //$NON-NLS-1$
				settings.put("SituationManager.weights.1", weights[1]); //$NON-NLS-1$
			}
		});
		
		refresh(true);
	}
	
	/**
	 * @param settings
	 * @param key
	 * @param defVal
	 * @return
	 */
	private static int safeGetInt(IDialogSettings settings, String key, int defVal)
	{
		try
		{
			String v = settings.get(key);
			if (v == null)
				return defVal;
			return Integer.parseInt(v);
		}
		catch(Exception e)
		{
			return defVal;
		}
	}
	
	/**
	 * @param hideOnFailure
	 */
	private void refresh(final boolean hideOnFailure)
	{
		new ConsoleJob(Messages.get().SituationsManager_LoadJob_Title, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<Situation> list = session.getSituations();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(Situation s : list)
							situations.put(s.getId(), s);
						situationTree.setInput(situations);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().SituationsManager_LoadJob_Error;
			}

			@Override
			protected void jobFailureHandler()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (hideOnFailure)
							PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().hideView(SituationsManager.this);
					}
				});
			}
		}.start();
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
				refresh(false);
			}
		};
		
		actionCreate = new Action(Messages.get().SituationsManager_Create) {
			@Override
			public void run()
			{
				createSituation();
			}
		};
		
		actionDelete = new Action(Messages.get().SituationsManager_Delete) {
			@Override
			public void run()
			{
				deleteSelectedElements();
			}
		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
	}
	
	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionCreate);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}

	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
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
		Menu menu = menuMgr.createContextMenu(situationTree.getControl());
		situationTree.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, situationTree);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		IStructuredSelection selection = (IStructuredSelection)situationTree.getSelection();
		if (selection.size() > 0)
		{
			if ((selection.size() == 1) && (selection.getFirstElement() instanceof String))
				manager.add(actionCreate);		// root element selected
			else
				manager.add(actionDelete);
			manager.add(new Separator());
		}
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		situationTree.getTree().setFocus();
	}
	
	/**
	 * Create new situation object
	 */
	private void createSituation()
	{
		InputDialog dlg = new InputDialog(getSite().getShell(), Messages.get().SituationsManager_CreateDlg_Title, Messages.get().SituationsManager_CreateDlg_Text, "", new IInputValidator()	{ //$NON-NLS-1$
			@Override
			public String isValid(String newText)
			{
				if (newText.trim().isEmpty())
					return Messages.get().SituationsManager_EmptyNameError;
				return null;
			}
		});
		if (dlg.open() == Window.OK)
		{
			final String name = dlg.getValue().trim();
			new ConsoleJob(Messages.get().SituationsManager_CreateJob_Title, this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.createSituation(name, ""); //$NON-NLS-1$
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.get().SituationsManager_CreateJob_Error;
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected element(s)
	 */
	private void deleteSelectedElements()
	{
		IStructuredSelection selection = (IStructuredSelection)situationTree.getSelection();
		final Object[] elements = selection.toArray();
		new ConsoleJob(Messages.get().SituationsManager_DeleteJob_Title, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(Object o : elements)
				{
					if (o instanceof Situation)
					{
						session.deleteSituation(((Situation)o).getId());
					}
					else if (o instanceof SituationInstance)
					{
						session.deleteSituationInstance(((SituationInstance)o).getParent().getId(), ((SituationInstance)o).getName());
					}
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().SituationsManager_DeleteJob_Error;
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		if ((n.getCode() == NXCNotification.SITUATION_CREATED) ||
		    (n.getCode() == NXCNotification.SITUATION_UPDATED) ||
		    (n.getCode() == NXCNotification.SITUATION_DELETED))
		{
			PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
				@Override
				public void run()
				{
					switch(n.getCode())
					{
						case NXCNotification.SITUATION_CREATED:
						case NXCNotification.SITUATION_UPDATED:
							situations.put(n.getSubCode(), (Situation)n.getObject());
							break;
						case NXCNotification.SITUATION_DELETED:
							situations.remove(n.getSubCode());
							break;
					}
					situationTree.refresh();
				}
			});
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
}
