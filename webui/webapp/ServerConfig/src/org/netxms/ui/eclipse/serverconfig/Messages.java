package org.netxms.ui.eclipse.serverconfig;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.serverconfig.messages"; //$NON-NLS-1$
   public String AddAddressListElementDialog_AddrRange;
   public String AddAddressListElementDialog_EndAddr;
   public String AddAddressListElementDialog_EnterValidData;
   public String AddAddressListElementDialog_NetAddr;
   public String AddAddressListElementDialog_NetMask;
   public String AddAddressListElementDialog_StartAddr;
   public String AddAddressListElementDialog_Subnet;
   public String AddAddressListElementDialog_Title;
   public String AddAddressListElementDialog_Warning;
   public String AddUsmCredDialog_Auth;
   public String AddUsmCredDialog_AuthPasswd;
   public String AddUsmCredDialog_AuthTypeMD5;
   public String AddUsmCredDialog_AuthTypeNone;
   public String AddUsmCredDialog_AuthTypeSHA1;
   public String AddUsmCredDialog_EncPasswd;
   public String AddUsmCredDialog_Encryption;
   public String AddUsmCredDialog_EncTypeAES;
   public String AddUsmCredDialog_EncTypeDES;
   public String AddUsmCredDialog_EncTypeNone;
   public String AddUsmCredDialog_Title;
   public String AddUsmCredDialog_UserName;
   public String ConfigurationImportDialog_Browse;
   public String ConfigurationImportDialog_FileName;
   public String ConfigurationImportDialog_FileTypeAll;
   public String ConfigurationImportDialog_FileTypeXML;
   public String ConfigurationImportDialog_ReplaceByCode;
   public String ConfigurationImportDialog_ReplaceByName;
   public String ConfigurationImportDialog_SelectFile;
   public String ConfigurationImportDialog_Title;
   public String CreateMappingTableDialog_Description;
   public String CreateMappingTableDialog_Name;
   public String CreateMappingTableDialog_Title;
   public String ExportConfiguration_Error;
   public String ExportConfiguration_ErrorOpeningView;
   public String ExportFileBuilder_Add;
   public String ExportFileBuilder_Description;
   public String ExportFileBuilder_EnterValidFileName;
   public String ExportFileBuilder_EPPLoadJobError;
   public String ExportFileBuilder_EPPLoadJobName;
   public String ExportFileBuilder_ExportJobError;
   public String ExportFileBuilder_ExportJobName;
   public String ExportFileBuilder_FileName;
   public String ExportFileBuilder_FormTitle;
   public String ExportFileBuilder_Remove;
   public String ExportFileBuilder_ResolveJobName;
   public String ExportFileBuilder_Save;
   public String ExportFileBuilder_SectionEvents;
   public String ExportFileBuilder_SectionFile;
   public String ExportFileBuilder_SectionRules;
   public String ExportFileBuilder_SectionTemplates;
   public String ExportFileBuilder_SectionTraps;
   public String ExportFileBuilder_TrapsLoadJobError;
   public String ExportFileBuilder_TrapsLoadJobName;
   public String ExportFileBuilder_Warning;
   public String ImportConfiguration_Information;
   public String ImportConfiguration_JobError;
   public String ImportConfiguration_JobName;
   public String ImportConfiguration_SuccessMessage;
   public String LogMacroEditDialog_Name;
   public String LogMacroEditDialog_TitleCreate;
   public String LogMacroEditDialog_TitleEdit;
   public String LogMacroEditDialog_Value;
   public String LogParserEditor_Add;
   public String LogParserEditor_AddRule;
   public String LogParserEditor_Delete;
   public String LogParserEditor_Edit;
   public String LogParserEditor_Editor;
   public String LogParserEditor_Error;
   public String LogParserEditor_InvalidDefinition;
   public String LogParserEditor_LogParser;
   public String LogParserEditor_Macros;
   public String LogParserEditor_Name;
   public String LogParserEditor_Rules;
   public String LogParserEditor_Value;
   public String LogParserEditor_XML;
   public String LogParserRuleEditor_Action;
   public String LogParserRuleEditor_ActiveContext;
   public String LogParserRuleEditor_ChangeContext;
   public String LogParserRuleEditor_Condition;
   public String LogParserRuleEditor_ContextAction;
   public String LogParserRuleEditor_ContextResetMode;
   public String LogParserRuleEditor_CtxActionActivate;
   public String LogParserRuleEditor_CtxActionClear;
   public String LogParserRuleEditor_CtxModeAuto;
   public String LogParserRuleEditor_CtxModeManual;
   public String LogParserRuleEditor_DeleteRule;
   public String LogParserRuleEditor_Facility;
   public String LogParserRuleEditor_GenerateEvent;
   public String LogParserRuleEditor_MatchingRegExp;
   public String LogParserRuleEditor_MoveDown;
   public String LogParserRuleEditor_MoveUp;
   public String LogParserRuleEditor_Parameters;
   public String LogParserRuleEditor_Severity;
   public String LogParserRuleEditor_SyslogTag;
   public String MappingTableEditor_ColComments;
   public String MappingTableEditor_ColKey;
   public String MappingTableEditor_ColValue;
   public String MappingTableEditor_Delete;
   public String MappingTableEditor_InitialPartName;
   public String MappingTableEditor_LoadJobError;
   public String MappingTableEditor_LoadJobName;
   public String MappingTableEditor_NewRow;
   public String MappingTableEditor_PartName;
   public String MappingTableEditor_RefreshConfirmation;
   public String MappingTableEditor_RefreshConfirmationText;
   public String MappingTableEditor_Save;
   public String MappingTableEditor_SaveJobError;
   public String MappingTableEditor_SaveJobName;
   public String MappingTableListLabelProvider_FlagNone;
   public String MappingTableListLabelProvider_FlagNumeric;
   public String MappingTables_ColDescription;
   public String MappingTables_ColFlags;
   public String MappingTables_ColID;
   public String MappingTables_ColName;
   public String MappingTables_CreateJobName;
   public String MappingTables_Delete;
   public String MappingTables_DeleteConfirmation;
   public String MappingTables_DeleteConfirmationText;
   public String MappingTables_DeleteJobError;
   public String MappingTables_DeleteJobName;
   public String MappingTables_Edit;
   public String MappingTables_Error;
   public String MappingTables_ErrorOpeningView;
   public String MappingTables_NewTable;
   public String MappingTables_ReloadJobError;
   public String MappingTables_ReloadJobName;
   public String NetworkDiscoveryConfigurator_AcceptAgent;
   public String NetworkDiscoveryConfigurator_AcceptRange;
   public String NetworkDiscoveryConfigurator_AcceptSNMP;
   public String NetworkDiscoveryConfigurator_ActiveDiscovery;
   public String NetworkDiscoveryConfigurator_Add;
   public String NetworkDiscoveryConfigurator_AddCommunity;
   public String NetworkDiscoveryConfigurator_AddCommunityDescr;
   public String NetworkDiscoveryConfigurator_AutoScript;
   public String NetworkDiscoveryConfigurator_CustomScript;
   public String NetworkDiscoveryConfigurator_DefCommString;
   public String NetworkDiscoveryConfigurator_Disabled;
   public String NetworkDiscoveryConfigurator_Error;
   public String NetworkDiscoveryConfigurator_FormTitle;
   public String NetworkDiscoveryConfigurator_LoadJobError;
   public String NetworkDiscoveryConfigurator_LoadJobName;
   public String NetworkDiscoveryConfigurator_NoFiltering;
   public String NetworkDiscoveryConfigurator_PassiveDiscovery;
   public String NetworkDiscoveryConfigurator_Remove;
   public String NetworkDiscoveryConfigurator_Save;
   public String NetworkDiscoveryConfigurator_SaveErrorText;
   public String NetworkDiscoveryConfigurator_SaveJobError;
   public String NetworkDiscoveryConfigurator_SaveJobName;
   public String NetworkDiscoveryConfigurator_SectionActiveDiscoveryTargets;
   public String NetworkDiscoveryConfigurator_SectionActiveDiscoveryTargetsDescr;
   public String NetworkDiscoveryConfigurator_SectionAddressFilters;
   public String NetworkDiscoveryConfigurator_SectionAddressFiltersDescr;
   public String NetworkDiscoveryConfigurator_SectionCommunities;
   public String NetworkDiscoveryConfigurator_SectionCommunitiesDescr;
   public String NetworkDiscoveryConfigurator_SectionFilter;
   public String NetworkDiscoveryConfigurator_SectionFilterDescr;
   public String NetworkDiscoveryConfigurator_SectionGeneral;
   public String NetworkDiscoveryConfigurator_SectionGeneralDescr;
   public String NetworkDiscoveryConfigurator_SectionUSM;
   public String NetworkDiscoveryConfigurator_SectionUSMDescr;
   public String NetworkDiscoveryConfigurator_UseSNMPTrapsForDiscovery;
   public String OpenMappingTables_Error;
   public String OpenMappingTables_ErrorOpeningView;
   public String OpenNetworkDiscoveryConfig_Error;
   public String OpenNetworkDiscoveryConfig_ErrorOpeningView;
   public String OpenNetworkDiscoveryConfig_JobError;
   public String OpenNetworkDiscoveryConfig_JobName;
   public String OpenServerConfig_Error;
   public String OpenServerConfig_ErrorOpeningView;
   public String OpenSyslogParserConfig_Error;
   public String OpenSyslogParserConfig_ErrorOpeningView;
   public String SelectSnmpTrapDialog_ColDescription;
   public String SelectSnmpTrapDialog_ColEvent;
   public String SelectSnmpTrapDialog_ColOID;
   public String SelectSnmpTrapDialog_Title;
   public String ServerConfigurationEditor_ActionCreate;
   public String ServerConfigurationEditor_ActionDelete;
   public String ServerConfigurationEditor_ActionEdit;
   public String ServerConfigurationEditor_ColName;
   public String ServerConfigurationEditor_ColRestart;
   public String ServerConfigurationEditor_ColValue;
   public String ServerConfigurationEditor_CreateJobError;
   public String ServerConfigurationEditor_CreateJobName;
   public String ServerConfigurationEditor_DeleteConfirmation;
   public String ServerConfigurationEditor_DeleteConfirmationText;
   public String ServerConfigurationEditor_DeleteJobError;
   public String ServerConfigurationEditor_DeleteJobName;
   public String ServerConfigurationEditor_LoadJobError;
   public String ServerConfigurationEditor_LoadJobName;
   public String ServerConfigurationEditor_ModifyJobError;
   public String ServerConfigurationEditor_ModifyJobName;
   public String ServerVariablesLabelProvider_No;
   public String ServerVariablesLabelProvider_Yes;
   public String SnmpUsmLabelProvider_AuthMD5;
   public String SnmpUsmLabelProvider_AuthNone;
   public String SnmpUsmLabelProvider_AuthSHA1;
   public String SnmpUsmLabelProvider_EncAES;
   public String SnmpUsmLabelProvider_EncDES;
   public String SnmpUsmLabelProvider_EncNone;
   public String SyslogParserConfigurator_ConfirmRefresh;
   public String SyslogParserConfigurator_ConfirmRefreshText;
   public String SyslogParserConfigurator_Error;
   public String SyslogParserConfigurator_ErrorSaveConfig;
   public String SyslogParserConfigurator_LoadJobError;
   public String SyslogParserConfigurator_LoadJobName;
   public String SyslogParserConfigurator_Save;
   public String SyslogParserConfigurator_SaveJobError;
   public String SyslogParserConfigurator_SaveJobName;
   public String VariableEditDialog_Name;
   public String VariableEditDialog_TitleCreate;
   public String VariableEditDialog_TitleEdit;
   public String VariableEditDialog_Value;
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
