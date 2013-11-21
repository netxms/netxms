package org.netxms.ui.eclipse.policymanager;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.policymanager.messages"; //$NON-NLS-1$
   public static String ConfigFile_File;
   public static String ConfigFile_JobError;
   public static String ConfigFile_JobName;
   public static String CreateAgentPolicyConfig_AgentPolicy_Config;
   public static String CreateAgentPolicyConfig_JobError;
   public static String CreateAgentPolicyConfig_JobName;
   public static String CreatePolicyGroup_JobError;
   public static String CreatePolicyGroup_JobName;
   public static String CreatePolicyGroup_PolicyGroup;
   public static String InstallPolicy_JobError;
   public static String InstallPolicy_JobName;
   public static String Policy_Description;
   public static String Policy_JobError;
   public static String Policy_JobName;
   public static String SelectInstallTargetDialog_InstallOnAlreadyInstalled;
   public static String SelectInstallTargetDialog_InstallOnSelected;
   public static String SelectInstallTargetDialog_Title;
   public static String UninstallPolicy_JobError;
   public static String UninstallPolicy_JobName;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
   {
   }
}
