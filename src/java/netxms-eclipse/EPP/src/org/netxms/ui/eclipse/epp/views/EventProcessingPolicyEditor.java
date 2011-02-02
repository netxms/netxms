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
package org.netxms.ui.eclipse.epp.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
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

	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	
	private NXCSession session;
	private boolean policyLocked = false;
	private EventProcessingPolicy policy; 
	private NXCListener sessionListener;
	private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
	private ScrolledComposite scroller;
	private Composite dataArea;
	private List<RuleEditor> ruleEditors = new ArrayList<RuleEditor>();
	private boolean verticalLayout = false;
	private Font headerFont;
	private Font normalFont;
	private Font boldFont;
	
	private Action actionHorizontal;
	private Action actionVertical;
	private Action actionSave; 
	
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
			Platform.getAdapterManager().loadAdapter(new ServerAction(0), "org.eclipse.ui.model.IWorkbenchAdapter");
			Platform.getAdapterManager().loadAdapter(session.getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter");
		}
		catch(Exception e)
		{
		}
		
		scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
		
		dataArea = new Composite(scroller, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 0;
		dataArea.setLayout(layout);
		dataArea.setBackground(BACKGROUND_COLOR);
		
		scroller.setContent(dataArea);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
		scroller.getVerticalBar().setIncrement(20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = scroller.getClientArea();
				scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
			}
		});

		headerFont = new Font(parent.getDisplay(), "Verdana", 10, SWT.BOLD);
		normalFont = new Font(parent.getDisplay(), "Verdana", 8, SWT.NORMAL);
		boldFont = new Font(parent.getDisplay(), "Verdana", 8, SWT.BOLD);
		
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				processSessionNotification(n);
			}
		};
		session.addListener(sessionListener);

		createActions();
		contributeToActionBars();
		//createPopupMenu();
		
		openEventProcessingPolicy();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionHorizontal = new Action("&Horizontal layout", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				verticalLayout = false;
				updateLayout();
			}
		};
		actionHorizontal.setChecked(!verticalLayout);
		actionHorizontal.setImageDescriptor(Activator.getImageDescriptor("icons/h_layout.gif"));
		
		actionVertical = new Action("&Vertical layout", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				verticalLayout = true;
				updateLayout();
			}
		};
		actionVertical.setChecked(verticalLayout);
		actionVertical.setImageDescriptor(Activator.getImageDescriptor("icons/v_layout.gif"));
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
		manager.add(actionHorizontal);
		manager.add(actionVertical);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionHorizontal);
		manager.add(actionVertical);
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
						initPolicyEditor();
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
	 * Init policy editor
	 */
	private void initPolicyEditor()
	{
		int ruleNumber = 1;
		for(EventProcessingPolicyRule rule : policy.getRules())
		{
			RuleEditor editor = new RuleEditor(dataArea, rule, ruleNumber++, this);
			ruleEditors.add(editor);
			GridData gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			editor.setLayoutData(gd);
		}
		dataArea.layout();

		Rectangle r = scroller.getClientArea();
		scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
	}
	
	/**
	 * Update editor's layout
	 */
	private void updateLayout()
	{
		for(RuleEditor editor : ruleEditors)
			editor.setVerticalLayout(verticalLayout, false);
		dataArea.layout();
		Rectangle r = scroller.getClientArea();
		scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
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
		dataArea.setFocus();
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
