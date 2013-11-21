package org.netxms.ui.eclipse.serverjobmanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.serverjobmanager.messages"; //$NON-NLS-1$
   public String OpenServerJobManager_Error;
   public String OpenServerJobManager_ErrorOpeningView;
   public String ServerJobLabelProvider_Active;
   public String ServerJobLabelProvider_Cancelled;
   public String ServerJobLabelProvider_CancelPending;
   public String ServerJobLabelProvider_Completed;
   public String ServerJobLabelProvider_Failed;
   public String ServerJobLabelProvider_OnHold;
   public String ServerJobLabelProvider_Pending;
   public String ServerJobLabelProvider_Unknown;
   public String ServerJobManager_ActionErrorName_Cancel;
   public String ServerJobManager_ActionErrorName_Hold;
   public String ServerJobManager_ActionErrorName_Unhold;
   public String ServerJobManager_ActionJobError;
   public String ServerJobManager_ActionJobName;
   public String ServerJobManager_ActionName_Cancel;
   public String ServerJobManager_ActionName_Hold;
   public String ServerJobManager_ActionName_Unhold;
   public String ServerJobManager_Cancel;
   public String ServerJobManager_ColDescription;
   public String ServerJobManager_ColInitiator;
   public String ServerJobManager_ColMessage;
   public String ServerJobManager_ColNode;
   public String ServerJobManager_ColProgress;
   public String ServerJobManager_ColStatus;
   public String ServerJobManager_Hold;
   public String ServerJobManager_RefreshJobError;
   public String ServerJobManager_RefreshJobName;
   public String ServerJobManager_Restart;
   public String ServerJobManager_Unhold;
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
