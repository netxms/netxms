package org.netxms.ui.eclipse.snmp;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.snmp.messages"; //$NON-NLS-1$
   public static String AddUsmCredDialog_Auth;
   public static String AddUsmCredDialog_AuthPasswd;
   public static String AddUsmCredDialog_AuthTypeMD5;
   public static String AddUsmCredDialog_AuthTypeNone;
   public static String AddUsmCredDialog_AuthTypeSHA1;
   public static String AddUsmCredDialog_EncPasswd;
   public static String AddUsmCredDialog_Encryption;
   public static String AddUsmCredDialog_EncTypeAES;
   public static String AddUsmCredDialog_EncTypeDES;
   public static String AddUsmCredDialog_EncTypeNone;
   public static String AddUsmCredDialog_Title;
   public static String AddUsmCredDialog_UserName;
	public static String LoginListener_JobError;
	public static String LoginListener_JobTitle;
	public static String MibExplorer_CopyName;
	public static String MibExplorer_CopyToClipboard;
	public static String MibExplorer_CopyType;
	public static String MibExplorer_CopyValue;
	public static String MibExplorer_OID;
	public static String MibExplorer_SelectInTree;
   public static String MibExplorer_SetNodeObject;
	public static String MibExplorer_Type;
	public static String MibExplorer_Value;
	public static String MibExplorer_Walk;
	public static String MibExplorer_WalkJob_Error;
	public static String MibExplorer_WalkJob_Title;
	public static String MibObjectDetails_8;
	public static String MibObjectDetails_Access;
	public static String MibObjectDetails_OID;
   public static String MibObjectDetails_OIDAsText;
	public static String MibObjectDetails_Status;
	public static String MibObjectDetails_TextualConv;
   public static String MibObjectDetails_Type;
	public static String MibSelectionDialog_EnterValidOID;
   public static String MibSelectionDialog_Error;
   public static String MibSelectionDialog_MIBTree;
	public static String MibSelectionDialog_OID;
   public static String MibSelectionDialog_OIDParseError;
	public static String MibSelectionDialog_Title;
	public static String MibSelectionDialog_Walk;
   public static String MibSelectionDialog_Warning;
	public static String MibSelectionDialog_WarningText;
   public static String MibWalkDialog_Title;
	public static String OpenMibExplorer_Error;
	public static String OpenMibExplorer_ErrorText;
	public static String OpenMibExplorerForNode_Error;
	public static String OpenMibExplorerForNode_ErrorText;
	public static String OpenSnmpTrapMonitor_Error;
	public static String OpenSnmpTrapMonitor_ErrorText;
	public static String OpenSnmpTrapMonitor_JobError;
	public static String OpenSnmpTrapMonitor_JobTitle;
	public static String OpenTrapEditor_Error;
	public static String OpenTrapEditor_ErrorText;
	public static String ParamMappingEditDialog_ByOID;
	public static String ParamMappingEditDialog_ByPos;
	public static String ParamMappingEditDialog_Description;
	public static String ParamMappingEditDialog_EnterVarbindPos;
	public static String ParamMappingEditDialog_NeverConvertToHex;
	public static String ParamMappingEditDialog_Options;
	public static String ParamMappingEditDialog_Select;
	public static String ParamMappingEditDialog_Title;
	public static String ParamMappingEditDialog_Varbind;
	public static String ParamMappingEditDialog_Warning;
	public static String ParamMappingEditDialog_WarningInvalidOID;
	public static String ParamMappingLabelProvider_PositionPrefix;
   public static String SnmpConfigurator_Add;
   public static String SnmpConfigurator_AddCommunity;
   public static String SnmpConfigurator_AddCommunityDescr;   
   public static String SnmpConfigurator_Error;
   public static String SnmpConfigurator_Remove;
   public static String SnmpConfigurator_Save;
   public static String SnmpConfigurator_SectionCommunities;
   public static String SnmpConfigurator_SectionCommunitiesDescr;
   public static String SnmpConfigurator_SectionUSM;
   public static String SnmpConfigurator_SectionUSMDescr;
	public static String SnmpTrapComparator_Unknown;
	public static String SnmpTrapEditor_ColDescription;
	public static String SnmpTrapEditor_ColEvent;
	public static String SnmpTrapEditor_ColID;
	public static String SnmpTrapEditor_ColOID;
	public static String SnmpTrapEditor_CreateJob_Error;
	public static String SnmpTrapEditor_CreateJob_Title;
	public static String SnmpTrapEditor_Delete;
	public static String SnmpTrapEditor_DeleteJob_Error;
	public static String SnmpTrapEditor_DeleteJob_Title;
	public static String SnmpTrapEditor_LoadJob_Error;
	public static String SnmpTrapEditor_LoadJob_Title;
	public static String SnmpTrapEditor_ModifyJob_Error;
	public static String SnmpTrapEditor_ModifyJob_Title;
	public static String SnmpTrapEditor_NewMapping;
	public static String SnmpTrapEditor_Properties;
	public static String SnmpTrapLabelProvider_Unknown;
	public static String SnmpTrapMonitor_ColOID;
	public static String SnmpTrapMonitor_ColSourceIP;
	public static String SnmpTrapMonitor_ColSourceNode;
	public static String SnmpTrapMonitor_ColTime;
	public static String SnmpTrapMonitor_ColVarbinds;
	public static String SnmpTrapMonitor_SubscribeJob_Error;
	public static String SnmpTrapMonitor_SubscribeJob_Title;
	public static String SnmpTrapMonitor_UnsubscribeJob_Error;
	public static String SnmpTrapMonitor_UnsubscribeJob_Title;
	public static String SnmpTrapMonitorLabelProvider_Unknown;
   public static String SnmpUsmLabelProvider_AuthMD5;
   public static String SnmpUsmLabelProvider_AuthNone;
   public static String SnmpUsmLabelProvider_AuthSHA1;
   public static String SnmpUsmLabelProvider_EncAES;
   public static String SnmpUsmLabelProvider_EncDES;
   public static String SnmpUsmLabelProvider_EncNone;
	public static String TrapConfigurationDialog_Add;
	public static String TrapConfigurationDialog_Delete;
	public static String TrapConfigurationDialog_Description;
	public static String TrapConfigurationDialog_Edit;
	public static String TrapConfigurationDialog_Event;
	public static String TrapConfigurationDialog_MoveDown;
	public static String TrapConfigurationDialog_MoveUp;
	public static String TrapConfigurationDialog_Number;
	public static String TrapConfigurationDialog_Parameter;
	public static String TrapConfigurationDialog_Parameters;
	public static String TrapConfigurationDialog_Select;
	public static String TrapConfigurationDialog_Title;
	public static String TrapConfigurationDialog_TrapOID;
	public static String TrapConfigurationDialog_UserTag;
	public static String TrapConfigurationDialog_Warning;
	public static String TrapConfigurationDialog_WarningInvalidOID;
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
