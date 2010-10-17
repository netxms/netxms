/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.widgets.PolicyEditor;
import org.netxms.ui.eclipse.epp.widgets.helpers.EPPCellFactory;
import org.netxms.ui.eclipse.epp.widgets.helpers.ImageFactory;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Event processing policy editor
 *
 */
public class EventProcessingPolicyEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.epp.view.policy_editor";
	public static final String JOB_FAMILY = "PolicyEditorJob";

	private NXCSession session;
	private boolean policyLocked = false;
	private EventProcessingPolicy policy; 
	private PolicyEditor editor;
	private NXCListener sessionListener;
	private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();

		// Initiate loading of event manager and object manager plugins if it was not loaded before
		try
		{
			Platform.getAdapterManager().loadAdapter(new EventTemplate(0), "org.eclipse.ui.model.IWorkbenchAdapter");
			Platform.getAdapterManager().loadAdapter(session.getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter");
		}
		catch(Exception e)
		{
		}
		
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				processSessionNotification(n);
			}
		};
		session.addListener(sessionListener);
		
		parent.setLayout(new FillLayout());
		
		editor = new PolicyEditor(parent, SWT.NONE, new EPPCellFactory());
		ConsoleJob job = new ConsoleJob("Open event processing policy", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot open event processing policy";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				List<ServerAction> actions = session.getActions();
				for(ServerAction a : actions)
				{
					EventProcessingPolicyEditor.this.actions.put(a.getId(), a);
				}
				
				policy = session.openEventProcessingPolicy();
				policyLocked = true;
				new UIJob("Fill rules list") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						editor.setContent(policy.getRules());
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected void jobFailureHandler()
			{
				new UIJob("Close event processing policy editor")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						EventProcessingPolicyEditor.this.getViewSite().getPage().hideView(EventProcessingPolicyEditor.this);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.start();
	}

	/**
	 * Process session notifications
	 * 
	 * @param n notification
	 */
	private void processSessionNotification(SessionNotification n)
	{
		switch(n.getCode())
		{
			case NXCNotification.ACTION_CREATED:
				synchronized(actions)
				{
					actions.put(n.getSubCode(), (ServerAction)n.getObject());
				}
				break;
			case NXCNotification.ACTION_MODIFIED:
				synchronized(actions)
				{
					actions.put(n.getSubCode(), (ServerAction)n.getObject());
				}
				break;
			case NXCNotification.ACTION_DELETED:
				synchronized(actions)
				{
					actions.remove(n.getSubCode());
				}
				break;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		editor.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if (sessionListener != null)
			session.removeListener(sessionListener);
		
		if (policyLocked)
		{
			new Job("Close event processing policy")
			{
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;

					try
					{
						session.closeEventProcessingPolicy();
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID,
								(e instanceof NXCException) ? ((NXCException) e).getErrorCode() : 0,
								"Cannot close event processing policy: " + e.getMessage(), null);
					}
					return status;
				}
			}.schedule();
		}
		
		super.dispose();
		
		ImageFactory.clearCache();
	}
}
