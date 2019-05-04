package org.netxms.ui.eclipse.objectmanager;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objectmanager.messages"; //$NON-NLS-1$
   public String AbstractNodePoll_Error;
   public String AbstractNodePoll_ErrorText;
   public String AbstractNodePoll_Warning;
   public String AccessControl_AccessAccessControl;
   public String AccessControl_AccessControl;
   public String AccessControl_AccessCreate;
   public String AccessControl_AccessDelete;
   public String AccessControl_AccessModify;
   public String AccessControl_AccessPushData;
   public String AccessControl_AccessRead;
   public String AccessControl_AccessRights;
   public String AccessControl_AccessSendEvents;
   public String AccessControl_AccessTermAlarms;
   public String AccessControl_AccessUpdateAlarms;
   public String AccessControl_AccessViewAlarms;
   public String AccessControl_Add;
   public String AccessControl_ColLogin;
   public String AccessControl_ColRights;
   public String AccessControl_CreateTicket;
   public String AccessControl_Delete;
   public String AccessControl_DownloadFiles;
   public String AccessControl_InheritRights;
   public String AccessControl_JobError;
   public String AccessControl_JobName;
   public String AccessControl_ManageFiles;
   public String AccessControl_UploadFiles;
   public String AccessControl_UsersGroups;
   public String AddAddressListElementDialog_AddressValidationError;
   public String AddAddressListElementDialog_NetworkAddress;
   public String AddAddressListElementDialog_NetworkMask;
   public String AddAddressListElementDialog_Title;
   public String AddAddressListElementDialog_Warning;
   public String AddClusterNode_JobError;
   public String AddClusterNode_JobTitle;
   public String AddSubnetDialog_Title;
   public String AttributeEditDialog_AddAttr;
   public String AttributeEditDialog_ModifyAttr;
   public String AttributeEditDialog_Name;
   public String AttributeEditDialog_Value;
   public String AutoApply_AutoApply;
   public String AutoApply_AutoRemove;
   public String AutoApply_JobError;
   public String AutoApply_JobName;
   public String AutoApply_Script;
   public String AutoBind_AutoBind;
   public String AutoBind_AUtoUnbind;
   public String AutoBind_JobError;
   public String AutoBind_JobName;
   public String AutoBind_Script;
   public String BindObject_JobError;
   public String BindObject_JobTitle;
   public String ChangeInterfaceExpectedState_JobError;
   public String ChangeInterfaceExpectedState_JobTitle;
   public String ChangeZone_JobError;
   public String ChangeZone_JobTitle;
   public String ClusterNetworkEditDialog_AddNet;
   public String ClusterNetworkEditDialog_Address;
   public String ClusterNetworkEditDialog_Mask;
   public String ClusterNetworkEditDialog_ModifyNet;
   public String ClusterNetworkEditDialog_Warning;
   public String ClusterNetworkEditDialog_WarningInvalidIP;
   public String ClusterNetworks_Add;
   public String ClusterNetworks_ColAddress;
   public String ClusterNetworks_ColMask;
   public String ClusterNetworks_Delete;
   public String ClusterNetworks_JobError;
   public String ClusterNetworks_JobName;
   public String ClusterNetworks_Modify;
   public String ClusterResources_Add;
   public String ClusterResources_ColName;
   public String ClusterResources_ColVIP;
   public String ClusterResources_Delete;
   public String ClusterResources_JobError;
   public String ClusterResources_JobName;
   public String ClusterResources_Modify;
   public String Comments_JobError;
   public String Comments_JobName;
   public String Communication_Authentication;
   public String Communication_AuthMD5;
   public String Communication_AuthMethod;
   public String Communication_AuthNone;
   public String Communication_AuthPassword;
   public String Communication_AuthPlain;
   public String Communication_AuthSHA1;
   public String Communication_Community;
   public String Communication_EncAES;
   public String Communication_EncDES;
   public String Communication_EncNone;
   public String Communication_EncPassword;
   public String Communication_Encryption;
   public String Communication_ForceEncryption;
   public String Communication_GroupAgent;
   public String Communication_GroupGeneral;
   public String Communication_GroupSNMP;
   public String Communication_ICMP;
   public String Communication_JobError;
   public String Communication_JobName;
   public String Communication_PrimaryHostName;
   public String Communication_Proxy;
   public String Communication_RemoteAgent;
   public String Communication_SharedSecret;
   public String Communication_TCPPort;
   public String Communication_UDPPort;
   public String Communication_UserName;
   public String Communication_Version;
   public String Communication_Warning;
   public String Communication_WarningInvalidAgentPort;
   public String Communication_WarningInvalidHostname;
   public String Communication_WarningInvalidSNMPPort;
   public String ConditionData_Add;
   public String ConditionData_ColFunction;
   public String ConditionData_ColNode;
   public String ConditionData_ColParameter;
   public String ConditionData_ColPosition;
   public String ConditionData_Delete;
   public String ConditionData_Down;
   public String ConditionData_JobError;
   public String ConditionData_JobName;
   public String ConditionData_Modify;
   public String ConditionData_Up;
   public String ConditionDciEditDialog_FuncAvg;
   public String ConditionDciEditDialog_FuncDeviation;
   public String ConditionDciEditDialog_FuncDiff;
   public String ConditionDciEditDialog_FuncError;
   public String ConditionDciEditDialog_FuncLast;
   public String ConditionDciEditDialog_FuncSum;
   public String ConditionDciEditDialog_Function;
   public String ConditionDciEditDialog_Node;
   public String ConditionDciEditDialog_Parameter;
   public String ConditionDciEditDialog_Polls;
   public String ConditionDciEditDialog_Title;
   public String ConditionEvents_ActivationEvent;
   public String ConditionEvents_ActiveStatus;
   public String ConditionEvents_DeactivationEvent;
   public String ConditionEvents_Events;
   public String ConditionEvents_InactiveStatus;
   public String ConditionEvents_JobError;
   public String ConditionEvents_JobName;
   public String ConditionEvents_SelectionServer;
   public String ConditionEvents_SourceObject;
   public String ConditionEvents_Status;
   public String ConditionScript_JobError;
   public String ConditionScript_JobName;
   public String ConditionScript_Script;
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
   public String CreateInterfaceDialog_Error;
   public String CreateInterfaceDialog_IPAddr;
   public String CreateInterfaceDialog_IPNetMak;
   public String CreateInterfaceDialog_IsPhysicalPort;
   public String CreateInterfaceDialog_MACAddr;
   public String CreateInterfaceDialog_Name;
   public String CreateInterfaceDialog_Port;
   public String CreateInterfaceDialog_Slot;
   public String CreateInterfaceDialog_Title;
   public String CreateMobileDevice_JobError;
   public String CreateMobileDevice_JobTitle;
   public String CreateMobileDeviceDialog_DeviceID;
   public String CreateMobileDeviceDialog_Name;
   public String CreateMobileDeviceDialog_Title;
   public String CreateMobileDeviceDialog_Warning;
   public String CreateMobileDeviceDialog_WarningEmptyName;
   public String CreateNetworkService_JobError;
   public String CreateNetworkService_JobTitle;
   public String CreateNetworkServiceDialog_CreateStatusDCI;
   public String CreateNetworkServiceDialog_Name;
   public String CreateNetworkServiceDialog_Port;
   public String CreateNetworkServiceDialog_Request;
   public String CreateNetworkServiceDialog_Response;
   public String CreateNetworkServiceDialog_ServiceType;
   public String CreateNetworkServiceDialog_Title;
   public String CreateNetworkServiceDialog_TypeFTP;
   public String CreateNetworkServiceDialog_TypeHTTP;
   public String CreateNetworkServiceDialog_TypeHTTPS;
   public String CreateNetworkServiceDialog_TypePOP3;
   public String CreateNetworkServiceDialog_TypeSMTP;
   public String CreateNetworkServiceDialog_TypeSSH;
   public String CreateNetworkServiceDialog_TypeTelnet;
   public String CreateNetworkServiceDialog_TypeUserDef;
   public String CreateNetworkServiceDialog_Warning;
   public String CreateNetworkServiceDialog_WarningEmptyName;
   public String CreateNetworkServiceDialog_WarningInvalidPort;
   public String CreateNode_JobError;
   public String CreateNode_JobTitle;
   public String CreateNodeDialog_AgentPort;
   public String CreateNodeDialog_AgentProxy;
   public String CreateNodeDialog_CreateUnmanaged;
   public String CreateNodeDialog_DisableAgent;
   public String CreateNodeDialog_DisableICMP;
   public String CreateNodeDialog_DisableSNMP;
   public String CreateNodeDialog_Name;
   public String CreateNodeDialog_Options;
   public String CreateNodeDialog_PrimaryHostName;
   public String CreateNodeDialog_Resolve;
   public String CreateNodeDialog_ResolveJobError;
   public String CreateNodeDialog_ResolveJobName;
   public String CreateNodeDialog_ShowAgain;
   public String CreateNodeDialog_SNMPPort;
   public String CreateNodeDialog_SNMPProxy;
   public String CreateNodeDialog_Title;
   public String CreateNodeDialog_Warning;
   public String CreateNodeDialog_WarningInvalidHostname;
   public String CreateNodeDialog_Zone;
   public String CreateRack_JobError;
   public String CreateRack_JobTitle;
   public String CreateRack_Rack;
   public String CreateVpnConnector_JobError;
   public String CreateVpnConnector_JobName;
   public String CreateVpnConnector_ObjectType;
   public String CreateZone_JobError;
   public String CreateZone_JobTitle;
   public String CreateZoneDialog_Name;
   public String CreateZoneDialog_Title;
   public String CreateZoneDialog_Warning;
   public String CreateZoneDialog_WarningEmptyName;
   public String CreateZoneDialog_WarningInvalidZoneId;
   public String CreateZoneDialog_ZoneId;
   public String CustomAttributes_Add;
   public String CustomAttributes_Delete;
   public String CustomAttributes_JobError;
   public String CustomAttributes_JobName;
   public String CustomAttributes_Modify;
   public String CustomAttributes_Name;
   public String CustomAttributes_Value;
   public String CustomAttributes_Warning;
   public String CustomAttributes_WarningAlreadyExist;
   public String Dashboards_Dashboard;
   public String DciListLabelProvider_JobError;
   public String DciListLabelProvider_JobName;
   public String DciListLabelProvider_Unresolved;
   public String DeleteObject_ConfirmDelete;
   public String DeleteObject_ConfirmQuestionPlural;
   public String DeleteObject_ConfirmQuestionSingular;
   public String DeleteObject_JobError;
   public String DeleteObject_JobName;
   public String EditClusterResourceDialog_ResName;
   public String EditClusterResourceDialog_TitleCreate;
   public String EditClusterResourceDialog_TitleEdit;
   public String EditClusterResourceDialog_VIP;
   public String EditClusterResourceDialog_Warning;
   public String EditClusterResourceDialog_WarningEmptyName;
   public String EditClusterResourceDialog_WarningInvalidIP;
   public String FullConfigurationPoll_FullConfigPollConfirmation;
   public String General_JobError;
   public String General_JobName;
   public String General_ObjectClass;
   public String General_ObjectID;
   public String General_ObjectName;
   public String InterfacePolling_ExcludeFromTopology;
   public String InterfacePolling_ExpectedState;
   public String InterfacePolling_JobError;
   public String InterfacePolling_JobName;
   public String InterfacePolling_RequiredPollCount;
   public String InterfacePolling_StateDOWN;
   public String InterfacePolling_StateIGNORE;
   public String InterfacePolling_StateUP;
   public String Location_Automatic;
   public String Location_City;
   public String Location_Country;
   public String Location_Error;
   public String Location_FormatError;
   public String Location_JobError;
   public String Location_JobName;
   public String Location_Latitude;
   public String Location_LocationType;
   public String Location_Longitude;
   public String Location_Manual;
   public String Location_Postcode;
   public String Location_StreetAddress;
   public String Location_Undefined;
   public String MaintanenceScheduleDialog_EndDate;
   public String MaintanenceScheduleDialog_StartDate;
   public String MaintanenceScheduleDialog_Title;
   public String MaintanenceScheduleDialog_Warning;
   public String MaintanenceScheduleDialog_WarningText;
   public String Manage_JobDescription;
   public String Manage_JobError;
   public String MapAppearance_Image;
   public String MapAppearance_JobError;
   public String MapAppearance_JobName;
   public String MapAppearance_Submap;
   public String NetworkServicePolling_JobError;
   public String NetworkServicePolling_JobName;
   public String NetworkServicePolling_PollerNode;
   public String NetworkServicePolling_Port;
   public String NetworkServicePolling_Request;
   public String NetworkServicePolling_RequiredPolls;
   public String NetworkServicePolling_Response;
   public String NetworkServicePolling_SelectionDefault;
   public String NetworkServicePolling_ServiceType;
   public String NetworkServicePolling_TypeFTP;
   public String NetworkServicePolling_TypeHTTP;
   public String NetworkServicePolling_TypeHTTPS;
   public String NetworkServicePolling_TypePOP3;
   public String NetworkServicePolling_TypeSMTP;
   public String NetworkServicePolling_TypeSSH;
   public String NetworkServicePolling_TypeTelnet;
   public String NetworkServicePolling_TypeUserDef;
   public String NetworkServicePolling_Warning;
   public String NetworkServicePolling_WarningInvalidPort;
   public String NodePollerView_ActionClear;
   public String NodePollerView_ActionRestart;
   public String NodePollerView_ConfigPoll;
   public String NodePollerView_FullConfigPoll;
   public String NodePollerView_InstanceDiscovery;
   public String NodePollerView_InterfacePoll;
   public String NodePollerView_InvalidObjectID;
   public String NodePollerView_JobName;
   public String NodePollerView_StatusPoll;
   public String NodePollerView_TopologyPoll;
   public String NodePolling_AgentCacheMode;
   public String NodePolling_Default;
   public String NodePolling_Disable;
   public String NodePolling_EmptySelectionServer;
   public String NodePolling_Enable;
   public String NodePolling_GroupIfXTable;
   public String NodePolling_GroupNetSrv;
   public String NodePolling_GroupOptions;
   public String NodePolling_JobError;
   public String NodePolling_JobName;
   public String NodePolling_Off;
   public String NodePolling_On;
   public String NodePolling_OptDisableAgent;
   public String NodePolling_OptDisableConfigPoll;
   public String NodePolling_OptDisableDataCollection;
   public String NodePolling_OptDisableDiscoveryPoll;
   public String NodePolling_OptDisableICMP;
   public String NodePolling_OptDisableRTPoll;
   public String NodePolling_OptDisableSNMP;
   public String NodePolling_OptDisableStatusPoll;
   public String NodePolling_OptDisableTopoPoll;
   public String NodePolling_PollerNode;
   public String NodePolling_PollerNodeDescription;
   public String RackPlacement_Height;
   public String RackPlacement_Position;
   public String RackPlacement_Rack;
   public String RackPlacement_RackImage;
   public String RackPlacement_UpdatingRackPlacement;
   public String RackProperties_BottomTop;
   public String RackProperties_Height;
   public String RackProperties_Numbering;
   public String RackProperties_TopBottom;
   public String RackProperties_UpdatingRackProperties;
   public String RemoveClusterNode_JobError;
   public String RemoveClusterNode_JobTitle;
   public String SensorPolling_JobError;
   public String SensorPolling_JobName;
   public String SensorWizard_General_CommMethod;
   public String SensorWizard_General_Description;
   public String SensorWizard_General_DescrLabel;
   public String SensorWizard_General_DeviceAddr;
   public String SensorWizard_General_DeviceClass;
   public String SensorWizard_General_MacAddr;
   public String SensorWizard_General_MetaType;
   public String SensorWizard_General_Proxy;
   public String SensorWizard_General_Serial;
   public String SensorWizard_General_Title;
   public String SensorWizard_General_Vendor;
   public String SensorWizard_LoRaWAN_Title;
   public String SensorWizard_LoRaWAN_Description;
   public String SensorWizard_LoRaWAN_Decoder;
   public String SensorWizard_LoRaWAN_RegType;
   public String SensorWizard_LoRaWAN_AppEUI;
   public String SensorWizard_LoRaWAN_AppKey;
   public String SensorWizard_LoRaWAN_AppSKey;
   public String SensorWizard_LoRaWAN_NwkSkey;
   public String SetInterfaceExpStateDlg_Label;
   public String SetInterfaceExpStateDlg_StateDown;
   public String SetInterfaceExpStateDlg_StateIgnore;
   public String SetInterfaceExpStateDlg_StateUp;
   public String SetInterfaceExpStateDlg_Title;
   public String SetObjectManagementState_JobError;
   public String SetObjectManagementState_JobTitle;
   public String StatusCalculation_CalculateAs;
   public String StatusCalculation_Default;
   public String StatusCalculation_Default2;
   public String StatusCalculation_FixedTo;
   public String StatusCalculation_JobError;
   public String StatusCalculation_JobName;
   public String StatusCalculation_MostCritical;
   public String StatusCalculation_MultipleThresholds;
   public String StatusCalculation_PropagateAs;
   public String StatusCalculation_Relative;
   public String StatusCalculation_SeverityBased;
   public String StatusCalculation_SingleThreshold;
   public String StatusCalculation_Unchanged;
   public String StatusCalculation_Validate_RelativeStatus;
   public String StatusCalculation_Validate_SingleThreshold;
   public String StatusCalculation_Validate_Threshold;
   public String TrustedNodes_Add;
   public String TrustedNodes_Delete;
   public String TrustedNodes_JobError;
   public String TrustedNodes_JobName;
   public String TrustedNodes_Node;
   public String UnbindObject_JobError;
   public String UnbindObject_JobTitle;
   public String Unmanage_JobDescription;
   public String Unmanage_JobError;
   public String VPNSubnets_JobError;
   public String VPNSubnets_JobName;
   public String VPNSubnets_LocalNetworks;
   public String VPNSubnets_PeerGateway;
   public String VPNSubnets_RemoteNetworks;
   public String ZoneCommunications_JobError;
   public String ZoneCommunications_JobName;
   public String ZoneCommunications_ProxyNodes;
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
