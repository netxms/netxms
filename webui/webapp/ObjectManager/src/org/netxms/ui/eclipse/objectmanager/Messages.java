package org.netxms.ui.eclipse.objectmanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objectmanager.messages"; //$NON-NLS-1$
   public String AbstractNodePoll_Error;
   public String AbstractNodePoll_ErrorText;
   public String AddClusterNode_JobError;
   public String AddClusterNode_JobTitle;
   public String BindObject_JobError;
   public String BindObject_JobTitle;
   public String ChangeInterfaceExpectedState_JobError;
   public String ChangeInterfaceExpectedState_JobTitle;
   public String ChangeZone_JobError;
   public String ChangeZone_JobTitle;
   public String CreateCluster_Cluster;
   public String CreateCluster_JobError;
   public String CreateCluster_JobTitle;
   public String CreateCondition_Condition;
   public String CreateCondition_JobError;
   public String CreateCondition_JobTitle;
   public String CreateContainer_Container;
   public String CreateContainer_JobError;
   public String CreateContainer_JobTitle;
   public String CreateInterface_JobError;
   public String CreateInterface_JobTitle;
   public String CreateMobileDevice_JobError;
   public String CreateMobileDevice_JobTitle;
   public String CreateNetworkService_JobError;
   public String CreateNetworkService_JobTitle;
   public String CreateNode_JobError;
   public String CreateNode_JobTitle;
   public String CreateRack_JobError;
   public String CreateRack_JobTitle;
   public String CreateRack_Rack;
   public String CreateZone_JobError;
   public String CreateZone_JobTitle;
   public String DeleteObject_ConfirmDelete;
   public String DeleteObject_ConfirmQuestionPlural;
   public String DeleteObject_ConfirmQuestionSingular;
   public String DeleteObject_JobDescription;
   public String DeleteObject_JobError;
   public String Manage_JobDescription;
   public String Manage_JobError;
   public String RemoveClusterNode_JobError;
   public String RemoveClusterNode_JobTitle;
   public String UnbindObject_JobError;
   public String UnbindObject_JobTitle;
   public String Unmanage_JobDescription;
   public String Unmanage_JobError;
   public String ZoneSelectionDialog_EmptySelectionWarning;
   public String ZoneSelectionDialog_Title;
   public String ZoneSelectionDialog_Warning;
   public String ZoneSelectionDialog_ZoneObject;
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

