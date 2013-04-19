/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.views;

import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.NodePollListener;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Forced node poll view
 *
 */
public class NodePollerView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objectmanager.views.NodePollerView";
	
	private static final String[] POLL_NAME = { "", "Status Poll", "Configuration Poll", "Interface Poll", "Topology Poll" };

	private NXCSession session;
	private AbstractNode node;
	private int pollType;
	private Text textArea;
	private boolean pollActive = false;
	private Action actionRestart;
	private Action actionClearOutput;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		// Secondary ID must by in form nodeId&pollType
		String[] parts = site.getSecondaryId().split("&");
		if (parts.length != 2)
			throw new PartInitException("Internal error");
		
		AbstractObject obj = session.findObjectById(Long.parseLong(parts[0]));
		node = ((obj != null) && (obj instanceof AbstractNode)) ? (AbstractNode)obj : null;
		if (node == null)
			throw new PartInitException("Invalid object ID");
		pollType = Integer.parseInt(parts[1]);
		
		setPartName(POLL_NAME[pollType] + " - " + node.getObjectName());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		textArea = new Text(parent, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		textArea.setEditable(false);
		textArea.setFont(JFaceResources.getTextFont());
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRestart = new Action("&Restart poll", SharedIcons.RESTART) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				startPoll();
			}
		};

		actionClearOutput = new Action("&Clear output", SharedIcons.CLEAR_LOG) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				textArea.setText("");
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
		manager.add(actionRestart);
		manager.add(actionClearOutput);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRestart);
		manager.add(actionClearOutput);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(textArea);
		textArea.setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionClearOutput);
	}

	/**
	 * Add poller message to text area
	 * 
	 * @param message poller message
	 */
	private void addPollerMessage(String message)
	{
		Date now = new Date();
		textArea.append("[" + RegionalSettings.getDateTimeFormat().format(now) + "] ");
		
		int index = message.indexOf(0x7F);
		if (index != -1)
		{
			textArea.append(message.substring(0, index));
			final String msgPart = message.substring(index + 2);
			textArea.append(msgPart);
		}
		else
		{
			textArea.append(message);
		}
		
		//textArea.setCaretOffset(textArea.getCharCount());
		//textArea.setTopIndex(textArea.getLineCount() - 1);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		textArea.setFocus();
	}

	/**
	 * Start poll
	 */
	public void startPoll()
	{
		if (pollActive)
			return;
		pollActive = true;
		actionRestart.setEnabled(false);
		
		addPollerMessage("\u007Fl**** Poll request sent to server ****\r\n");
		
		final NodePollListener listener = new NodePollListener() {
			@Override
			public void onPollerMessage(final String message)
			{
				new UIJob(textArea.getDisplay(), "Update poller window") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						if (!textArea.isDisposed())
							addPollerMessage(message);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		
		Job job = new Job("Node poll: " + node.getObjectName() + " [" + node.getObjectId() + "]") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				try
				{
					session.pollNode(node.getObjectId(), pollType, listener);
					onPollComplete(true, null);
				}
				catch(Exception e)
				{
					onPollComplete(false, e.getMessage());
				}
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
	
	/**
	 * Poll completion handler
	 * 
	 * @param success
	 * @param errorMessage
	 */
	private void onPollComplete(final boolean success, final String errorMessage)
	{
		new UIJob(textArea.getDisplay(), "Update poller window") {
			@Override
			public IStatus runInUIThread(IProgressMonitor monitor)
			{
				if (!textArea.isDisposed())
				{
					if (success)
					{
						addPollerMessage("\u007Fl**** Poll completed successfully ****\r\n\r\n");
					}
					else
					{
						addPollerMessage("\u007FePOLL ERROR: " + errorMessage);
						addPollerMessage("\u007Fl**** Poll failed ****\r\n\r\n");
					}
					pollActive = false;
					actionRestart.setEnabled(true);
				}
				return Status.OK_STATUS;
			}
		}.schedule();
	}
}
