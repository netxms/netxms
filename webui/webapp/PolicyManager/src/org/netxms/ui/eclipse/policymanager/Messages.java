package org.netxms.ui.eclipse.policymanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.policymanager.messages"; //$NON-NLS-1$
   public String ConfigFile_File;
   public String ConfigFile_JobError;
   public String ConfigFile_JobName;
   public String CreateAgentPolicyConfig_AgentPolicy_Config;
   public String CreateAgentPolicyConfig_JobError;
   public String CreateAgentPolicyConfig_JobName;
   public String CreatePolicyGroup_JobError;
   public String CreatePolicyGroup_JobName;
   public String CreatePolicyGroup_PolicyGroup;
   public String InstallPolicy_JobError;
   public String InstallPolicy_JobName;
   public String Policy_Description;
   public String Policy_JobError;
   public String Policy_JobName;
   public String SelectInstallTargetDialog_InstallOnAlreadyInstalled;
   public String SelectInstallTargetDialog_InstallOnSelected;
   public String SelectInstallTargetDialog_Title;
   public String UninstallPolicy_JobError;
   public String UninstallPolicy_JobName;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   

private Messages()
	{
	}


	/**
	 * Get message class for current locale
	 *
	 * @return
	 */
	public static Messages get()
	{
		return RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
	}

	/**
	 * Get message class for current locale
	 *
	 * @return
	 */
	public static Messages get(Display display)
	{
		CallHelper r = new CallHelper();
		display.syncExec(r);
		return r.messages;
	}

	/**
	 * Helper class to call RWT.NLS.getISO8859_1Encoded from non-UI thread
	 */
	private static class CallHelper implements Runnable
	{
		Messages messages;

		@Override
		public void run()
		{
			messages = RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
		}
	}

}
