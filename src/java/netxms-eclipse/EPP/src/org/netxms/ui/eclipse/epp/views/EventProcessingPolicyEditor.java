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
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.views.helpers.RuleTreeContentProvider;
import org.netxms.ui.eclipse.epp.views.helpers.RuleTreeElement;
import org.netxms.ui.eclipse.epp.views.helpers.RuleTreeLabelProvider;
import org.netxms.ui.eclipse.epp.widgets.AbstractRuleEditor;
import org.netxms.ui.eclipse.epp.widgets.CommentsEditor;
import org.netxms.ui.eclipse.epp.widgets.RuleOverview;
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
	private NXCListener sessionListener;
	private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
	private SashForm splitter;
	private TreeViewer ruleTree;
	private Composite dataArea;
	private CLabel header;
	private Font headerFont;
	private Font normalFont;
	private Font boldFont;
	private AbstractRuleEditor currentEditor = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();

		// Initiate loading of required plugins if they was not loaded yet
		try
		{
			Platform.getAdapterManager().loadAdapter(new EventTemplate(0), "org.eclipse.ui.model.IWorkbenchAdapter");
			Platform.getAdapterManager().loadAdapter(session.getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter");
			Platform.getAdapterManager().loadAdapter(new ServerAction(0), "org.eclipse.ui.model.IWorkbenchAdapter");
		}
		catch(Exception e)
		{
		}
		
		splitter = new SashForm(parent, SWT.HORIZONTAL);
		splitter.setSashWidth(3);
		
		ruleTree = new TreeViewer(splitter, SWT.BORDER);
		ruleTree.setContentProvider(new RuleTreeContentProvider());
		ruleTree.setLabelProvider(new RuleTreeLabelProvider());
		ruleTree.setAutoExpandLevel(2);
		ruleTree.addSelectionChangedListener(new ISelectionChangedListener()	{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				onSelectionChange();
			}
		});
		
		dataArea = new Composite(splitter, SWT.BORDER);
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 0;
		dataArea.setLayout(layout);
		
		headerFont = new Font(parent.getDisplay(), "Verdana", 10, SWT.BOLD);
		normalFont = new Font(parent.getDisplay(), "Verdana", 8, SWT.NORMAL);
		boldFont = new Font(parent.getDisplay(), "Verdana", 8, SWT.BOLD);
		
		header = new CLabel(dataArea, SWT.BORDER);
		header.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
		header.setFont(headerFont);
		header.setBackground(new Color(parent.getDisplay(), 153, 180, 209));
		header.setForeground(new Color(parent.getDisplay(), 255, 255, 255));
		
		splitter.setWeights(new int[] { 25, 75 });
		
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				processSessionNotification(n);
			}
		};
		session.addListener(sessionListener);
		
		openEventProcessingPolicy();
	}
	
	/**
	 * Open event processing policy
	 */
	private void openEventProcessingPolicy()
	{
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
				synchronized(EventProcessingPolicyEditor.this.actions)
				{
					for(ServerAction a : actions)
					{
						EventProcessingPolicyEditor.this.actions.put(a.getId(), a);
					}
				}
				
				policy = session.openEventProcessingPolicy();
				policyLocked = true;
				new UIJob("Update rules presentation") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						ruleTree.setInput(policy);
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
		splitter.setFocus();
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
			new ConsoleJob("Close event processing policy", null, Activator.PLUGIN_ID, EventProcessingPolicyEditor.JOB_FAMILY) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.closeEventProcessingPolicy();
				}

				@Override
				protected String getErrorMessage()
				{
					return "Cannot close event processing policy";
				}
			}.start();
		}
		
		headerFont.dispose();
		normalFont.dispose();
		boldFont.dispose();
		
		super.dispose();
		ImageFactory.clearCache();
	}
	
	/**
	 * Handler for tree viewer selection change
	 */
	private void onSelectionChange()
	{
		if (currentEditor != null)
			currentEditor.dispose();
		
		IStructuredSelection selection = (IStructuredSelection)ruleTree.getSelection();
		if (selection.size()> 0)
		{
			RuleTreeElement element = (RuleTreeElement)selection.getFirstElement();
			switch(element.getType())
			{
				case RuleTreeElement.RULE:
					currentEditor = new RuleOverview(dataArea, element.getRule(), this);
					break;
				case RuleTreeElement.COMMENTS:
					currentEditor = new CommentsEditor(dataArea, element.getRule(), this);
					break;
				default:
					currentEditor = null;
					break;
			}
			
			if (element.getType() != RuleTreeElement.POLICY)
			{
				header.setText("Rule " + element.getRuleNumber() + element.getCommentString());
			}
			else
			{
				header.setText("");
			}
			
			if (currentEditor != null)
			{
				GridData gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.verticalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				gd.grabExcessVerticalSpace = true;
				currentEditor.setLayoutData(gd);
			}
			
			dataArea.layout();
		}
		else
		{
			currentEditor = null;
			header.setText("");
		}
	}
	
	/**
	 * Find server action by ID
	 * 
	 * @param id action id
	 * @return server action object or null
	 */
	public ServerAction findActionById(Long id)
	{
		return actions.get(id);
	}

	/**
	 * @return the normalFont
	 */
	public Font getNormalFont()
	{
		return normalFont;
	}

	/**
	 * @return the boldFont
	 */
	public Font getBoldFont()
	{
		return boldFont;
	}
}
