/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.StatusLineContributionItem;
import org.eclipse.ui.progress.UIJob;

/**
 * @author Victor
 *
 */
public class MemoryMonitor extends StatusLineContributionItem
{
	private long usedMemory = 0;
	private UpdateThread updateThread = null;
	
	/**
	 * Update thread
	 * @author Victor
	 */
	class UpdateThread extends Thread
	{
		private boolean stopCondition = false;
		
		UpdateThread()
		{
			setDaemon(true);
			start();
		}
		
		/* (non-Javadoc)
		 * @see java.lang.Thread#run()
		 */
		@Override
		public void run()
		{
			while(!stopCondition)
			{
				Runtime rt = Runtime.getRuntime();
				usedMemory = rt.totalMemory() - rt.freeMemory();
				new UIJob("Update memory monitor") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						MemoryMonitor.this.setText(Long.toString(usedMemory / 1048576) + "M");
						return Status.OK_STATUS;
					}
				}.schedule();

				try
				{
					sleep(1000);
				}
				catch(InterruptedException e)
				{
				}
			}
		}
		
		public void setStopCondition()
		{
			stopCondition = true;
			interrupt();
		}
	}
	
	/**
	 * Constructor
	 * @param id
	 */
	public MemoryMonitor(String id)
	{
		super(id);
		updateThread = new UpdateThread();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.action.ContributionItem#dispose()
	 */
	@Override
	public void dispose()
	{
		updateThread.setStopCondition();
		try
		{
			updateThread.join();
		}
		catch(InterruptedException e)
		{
		}
		super.dispose();
	}
}
