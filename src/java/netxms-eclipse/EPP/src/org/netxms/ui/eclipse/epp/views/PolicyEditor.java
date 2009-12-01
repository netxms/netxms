/**
 * 
 */
package org.netxms.ui.eclipse.epp.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ExpandBar;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.widgets.RuleList;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class PolicyEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.epp.view.policy_editor";
	public static final String JOB_FAMILY = "PolicyEditorJob";

	private NXCSession session;
	private boolean policyLocked = false;
	private EventProcessingPolicy policy; 
	private RuleList ruleList;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = NXMCSharedData.getInstance().getSession();

		parent.setLayout(new FillLayout());
		
		ruleList = new RuleList(parent, SWT.NONE);
		ConsoleJob job = new ConsoleJob("Open event processing policy", this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot open event processing policy";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				policy = session.openEventProcessingPolicy();
				policyLocked = true;
				new UIJob("Fill rules list") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						ruleList.setContent(policy.getRules());
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
						PolicyEditor.this.getViewSite().getPage().hideView(PolicyEditor.this);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
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
	}
}
