package org.netxms.ui.eclipse.epp;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.epp.messages"; //$NON-NLS-1$
	public static String ActionSelectionDialog_Filter;
	public static String ActionSelectionDialog_Title;
	public static String AttributeEditDialog_Name;
	public static String AttributeEditDialog_TitleAdd;
	public static String AttributeEditDialog_TitleEdit;
	public static String AttributeEditDialog_Value;
	public static String EventProcessingPolicyEditor_CloseJob_Error;
	public static String EventProcessingPolicyEditor_CloseJob_Title;
	public static String EventProcessingPolicyEditor_CollapseAll;
	public static String EventProcessingPolicyEditor_Copy;
	public static String EventProcessingPolicyEditor_Cut;
	public static String EventProcessingPolicyEditor_Delete;
	public static String EventProcessingPolicyEditor_Disable;
	public static String EventProcessingPolicyEditor_Enable;
	public static String EventProcessingPolicyEditor_Error;
	public static String EventProcessingPolicyEditor_ExpandAll;
	public static String EventProcessingPolicyEditor_InsertAfter;
	public static String EventProcessingPolicyEditor_InsertBefore;
	public static String EventProcessingPolicyEditor_LayoutH;
	public static String EventProcessingPolicyEditor_LayoutV;
	public static String EventProcessingPolicyEditor_OpenJob_Error;
	public static String EventProcessingPolicyEditor_OpenJob_Title;
	public static String EventProcessingPolicyEditor_Paste;
	public static String EventProcessingPolicyEditor_Save;
	public static String EventProcessingPolicyEditor_SaveError;
	public static String EventProcessingPolicyEditor_SaveJob_Error;
	public static String EventProcessingPolicyEditor_SaveJob_Title;
   public static String EventProcessingPolicyEditor_ShowFilter;
	public static String OpenPolicyEditor_Error;
	public static String OpenPolicyEditor_ErrorText;
	public static String RuleAction_StopProcessing;
	public static String RuleAlarm_CreateNew;
	public static String RuleAlarm_DoNotChange;
	public static String RuleAlarm_FromEvent;
	public static String RuleAlarm_Key;
	public static String RuleAlarm_Message;
	public static String RuleAlarm_Resolve;
	public static String RuleAlarm_ResolveAll;
	public static String RuleAlarm_Severity;
	public static String RuleAlarm_Terminate;
	public static String RuleAlarm_TerminateAll;
	public static String RuleAlarm_Timeout;
	public static String RuleAlarm_TimeoutEvent;
	public static String RuleAlarm_UseRegexpForResolve;
	public static String RuleAlarm_UseRegexpForTerminate;
	public static String RuleAlarm_Warning;
	public static String RuleAlarm_WarningInvalidTimeout;
	public static String RuleCondition_RuleDisabled;
	public static String RuleEditor_Action;
	public static String RuleEditor_AND;
	public static String RuleEditor_AND_NOT;
	public static String RuleEditor_Attributes;
	public static String RuleEditor_DisabledSuffix;
	public static String RuleEditor_EditActions;
	public static String RuleEditor_EditCondition;
	public static String RuleEditor_EventIs;
	public static String RuleEditor_ExecuteActions;
	public static String RuleEditor_Filter;
	public static String RuleEditor_GenerateAlarm;
	public static String RuleEditor_IF;
	public static String RuleEditor_IF_NOT;
	public static String RuleEditor_ResolveAlarms;
	public static String RuleEditor_Rule;
	public static String RuleEditor_ScriptIs;
	public static String RuleEditor_SeverityIs;
	public static String RuleEditor_SourceIs;
	public static String RuleEditor_StopProcessing;
	public static String RuleEditor_TerminateAlarms;
	public static String RuleEditor_Tooltip_CollapseRule;
	public static String RuleEditor_Tooltip_EditRule;
	public static String RuleEditor_Tooltip_ExpandRule;
	public static String RuleEditor_Unknown;
	public static String RuleEditor_UseRegexpForResolve;
	public static String RuleEditor_UserRegexpForTerminate;
	public static String RuleEditor_WithKey;
	public static String RuleEvents_Add;
	public static String RuleEvents_Delete;
	public static String RuleEvents_Event;
	public static String RuleEvents_InvertedRule;
	public static String RuleSelectionDialog_JobError;
   public static String RuleSelectionDialog_JobTitle;
   public static String RuleSelectionDialog_Title;
   public static String RuleServerActions_Action;
	public static String RuleServerActions_Add;
	public static String RuleServerActions_Delete;
	public static String RuleSourceObjects_Add;
	public static String RuleSourceObjects_Delete;
	public static String RuleSourceObjects_InvertRule;
	public static String RuleSourceObjects_Object;
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
