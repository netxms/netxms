package org.netxms.ui.eclipse.epp;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.epp.messages"; //$NON-NLS-1$
	public String ActionSelectionDialog_Filter;
	public String ActionSelectionDialog_Title;
	public String AttributeEditDialog_Name;
	public String AttributeEditDialog_TitleAdd;
	public String AttributeEditDialog_TitleEdit;
	public String AttributeEditDialog_Value;
	public String EventProcessingPolicyEditor_CloseJob_Error;
	public String EventProcessingPolicyEditor_CloseJob_Title;
	public String EventProcessingPolicyEditor_CollapseAll;
	public String EventProcessingPolicyEditor_Copy;
	public String EventProcessingPolicyEditor_Cut;
	public String EventProcessingPolicyEditor_Delete;
	public String EventProcessingPolicyEditor_Disable;
	public String EventProcessingPolicyEditor_Enable;
	public String EventProcessingPolicyEditor_Error;
	public String EventProcessingPolicyEditor_ExpandAll;
	public String EventProcessingPolicyEditor_InsertAfter;
	public String EventProcessingPolicyEditor_InsertBefore;
	public String EventProcessingPolicyEditor_LayoutH;
	public String EventProcessingPolicyEditor_LayoutV;
	public String EventProcessingPolicyEditor_OpenJob_Error;
	public String EventProcessingPolicyEditor_OpenJob_Title;
	public String EventProcessingPolicyEditor_Paste;
	public String EventProcessingPolicyEditor_Save;
	public String EventProcessingPolicyEditor_SaveError;
	public String EventProcessingPolicyEditor_SaveJob_Error;
	public String EventProcessingPolicyEditor_SaveJob_Title;
	public String OpenPolicyEditor_Error;
	public String OpenPolicyEditor_ErrorText;
	public String OpenSituationsManager_Error;
	public String OpenSituationsManager_ErrorText;
	public String RuleAction_StopProcessing;
	public String RuleAlarm_CreateNew;
	public String RuleAlarm_DoNotChange;
	public String RuleAlarm_FromEvent;
	public String RuleAlarm_Key;
	public String RuleAlarm_Message;
	public String RuleAlarm_Resolve;
	public String RuleAlarm_ResolveAll;
	public String RuleAlarm_Severity;
	public String RuleAlarm_Terminate;
	public String RuleAlarm_TerminateAll;
	public String RuleAlarm_Timeout;
	public String RuleAlarm_TimeoutEvent;
	public String RuleAlarm_UseRegexpForResolve;
	public String RuleAlarm_UseRegexpForTerminate;
	public String RuleAlarm_Warning;
	public String RuleAlarm_WarningInvalidTimeout;
	public String RuleCondition_RuleDisabled;
	public String RuleEditor_Action;
	public String RuleEditor_AND;
	public String RuleEditor_AND_NOT;
	public String RuleEditor_Attributes;
	public String RuleEditor_DisabledSuffix;
	public String RuleEditor_EditActions;
	public String RuleEditor_EditCondition;
	public String RuleEditor_EventIs;
	public String RuleEditor_ExecuteActions;
	public String RuleEditor_Filter;
	public String RuleEditor_GenerateAlarm;
	public String RuleEditor_IF;
	public String RuleEditor_IF_NOT;
	public String RuleEditor_Instance;
	public String RuleEditor_ResolveAlarms;
	public String RuleEditor_Rule;
	public String RuleEditor_ScriptIs;
	public String RuleEditor_SeverityIs;
	public String RuleEditor_SourceIs;
	public String RuleEditor_StopProcessing;
	public String RuleEditor_TerminateAlarms;
	public String RuleEditor_Tooltip_CollapseRule;
	public String RuleEditor_Tooltip_EditRule;
	public String RuleEditor_Tooltip_ExpandRule;
	public String RuleEditor_Unknown;
	public String RuleEditor_UpdateSituation;
	public String RuleEditor_UseRegexpForResolve;
	public String RuleEditor_UserRegexpForTerminate;
	public String RuleEditor_WithKey;
	public String RuleEvents_Add;
	public String RuleEvents_Delete;
	public String RuleEvents_Event;
	public String RuleEvents_InvertedRule;
	public String RuleSelectionDialog_JobError;
   public String RuleSelectionDialog_JobTitle;
   public String RuleSelectionDialog_Title;
   public String RuleServerActions_Action;
	public String RuleServerActions_Add;
	public String RuleServerActions_Delete;
	public String RuleSituation_Add;
	public String RuleSituation_Attributes;
	public String RuleSituation_Delete;
	public String RuleSituation_Edit;
	public String RuleSituation_Instance;
	public String RuleSituation_Name;
	public String RuleSituation_SituationObject;
	public String RuleSituation_Value;
	public String RuleSourceObjects_Add;
	public String RuleSourceObjects_Delete;
	public String RuleSourceObjects_InvertRule;
	public String RuleSourceObjects_Object;
	public String SituationCache_Error;
	public String SituationCache_ErrorText;
	public String SituationCache_JobTitle;
	public String SituationSelectionDialog_Filter;
	public String SituationSelectionDialog_Title;
	public String SituationSelector_None;
	public String SituationSelector_Tooltip;
	public String SituationSelector_Unknown;
	public String SituationsManager_ColAttribute;
	public String SituationsManager_ColValue;
	public String SituationsManager_Create;
	public String SituationsManager_CreateDlg_Text;
	public String SituationsManager_CreateDlg_Title;
	public String SituationsManager_CreateJob_Error;
	public String SituationsManager_CreateJob_Title;
	public String SituationsManager_Delete;
	public String SituationsManager_DeleteJob_Error;
	public String SituationsManager_DeleteJob_Title;
	public String SituationsManager_EmptyNameError;
	public String SituationsManager_LoadJob_Error;
	public String SituationsManager_LoadJob_Title;
	public String SituationTreeContentProvider_Root;
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

