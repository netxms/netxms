package org.netxms.ui.eclipse.reporter.views;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.reporting.ReportDefinition;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.widgets.internal.ReportTreeContentProvider;
import org.netxms.ui.eclipse.reporter.widgets.internal.ReportTreeLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class ReportNavigator extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.reporter.views.ReportNavigator"; //$NON-NLS-1$
	private NXCSession session;
	private TreeViewer reportTree;
	private RefreshAction actionRefresh;
	private SessionListener sessionListener;

	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();

		reportTree = new TreeViewer(parent, SWT.NONE);
		reportTree.setContentProvider(new ReportTreeContentProvider());
		reportTree.setLabelProvider(new ReportTreeLabelProvider());
		// reportTree.setInput(session);

		createActions();
		contributeToActionBars();
		createPopupMenu();

		getSite().setSelectionProvider(reportTree);

		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if ((n.getCode() == NXCNotification.OBJECT_CHANGED) && (n.getObject() instanceof DashboardRoot))
				{
					reportTree.getTree().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							refresh();
						}
					});
				}
			}
		};
		session.addListener(sessionListener);
		refresh();
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
				refresh();
			}
		};
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
	 * Create popup menu for object browser
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager manager = new MenuManager();
		manager.setRemoveAllWhenShown(true);
		manager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = manager.createContextMenu(reportTree.getTree());
		reportTree.getTree().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(manager, reportTree);
	}

	/**
	 * Fill context menu
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
	}

	@Override
	public void setFocus()
	{
		reportTree.getTree().setFocus();
	}

	/**
	 * Refresh reports tree
	 */
	private void refresh()
	{
		new ConsoleJob("Load Reports", null, Activator.PLUGIN_ID, null) {

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ReportDefinition> definitions = new ArrayList<ReportDefinition>();
				final List<UUID> reportIds = session.listReports();
				for(UUID reportId : reportIds)
				{
					final ReportDefinition definition = session.getReportDefinition(reportId);
					definitions.add(definition);
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						reportTree.setInput(definitions);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Failed to reports from the server";
			}
		}.start();
	}

	@Override
	public void dispose()
	{
		if ((session != null) && (sessionListener != null))
		{
			session.removeListener(sessionListener);
		}
		super.dispose();
	}
}
