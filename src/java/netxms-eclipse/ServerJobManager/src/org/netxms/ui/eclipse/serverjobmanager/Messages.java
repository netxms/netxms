package org.netxms.ui.eclipse.serverjobmanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.serverjobmanager.messages"; //$NON-NLS-1$
   public static String OpenServerJobManager_Error;
   public static String OpenServerJobManager_ErrorOpeningView;
   public static String ServerJobLabelProvider_Active;
   public static String ServerJobLabelProvider_Cancelled;
   public static String ServerJobLabelProvider_CancelPending;
   public static String ServerJobLabelProvider_Completed;
   public static String ServerJobLabelProvider_Failed;
   public static String ServerJobLabelProvider_OnHold;
   public static String ServerJobLabelProvider_Pending;
   public static String ServerJobLabelProvider_Unknown;
   public static String ServerJobManager_ActionErrorName_Cancel;
   public static String ServerJobManager_ActionErrorName_Hold;
   public static String ServerJobManager_ActionErrorName_Unhold;
   public static String ServerJobManager_ActionJobError;
   public static String ServerJobManager_ActionJobName;
   public static String ServerJobManager_ActionName_Cancel;
   public static String ServerJobManager_ActionName_Hold;
   public static String ServerJobManager_ActionName_Unhold;
   public static String ServerJobManager_Cancel;
   public static String ServerJobManager_ColDescription;
   public static String ServerJobManager_ColInitiator;
   public static String ServerJobManager_ColMessage;
   public static String ServerJobManager_ColNode;
   public static String ServerJobManager_ColProgress;
   public static String ServerJobManager_ColStatus;
   public static String ServerJobManager_Hold;
   public static String ServerJobManager_RefreshJobError;
   public static String ServerJobManager_RefreshJobName;
   public static String ServerJobManager_Restart;
   public static String ServerJobManager_Unhold;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
	{
 }


	private static Messages instance = new Messages();

	public static Messages get()
	{
		return instance;
	}

}
