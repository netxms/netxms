package org.netxms.ui.eclipse.topology;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.topology.messages"; //$NON-NLS-1$
   public static String ConnectionPointLabelProvider_Direct;
   public static String ConnectionPointLabelProvider_Indirect;
   public static String ConnectionPointLabelProvider_Unknown;
   public static String ConnectionPointLabelProvider_Wireless;
   public static String DeviceView_SlotName;
   public static String EnterIpAddressDlg_EnterValidAddress;
   public static String EnterIpAddressDlg_IpAddress;
   public static String EnterIpAddressDlg_SelectZone;
   public static String EnterIpAddressDlg_Title;
   public static String EnterIpAddressDlg_Warning;
   public static String EnterIpAddressDlg_Zone;
   public static String EnterMacAddressDlg_Error;
   public static String EnterMacAddressDlg_IncorrectMacAddress;
   public static String EnterMacAddressDlg_MacAddress;
   public static String EnterMacAddressDlg_Title;
   public static String FindConnectionPoint_JobError;
   public static String FindConnectionPoint_JobTitle;
   public static String FindIpAddress_JobError;
   public static String FindIpAddress_JobTitle;
   public static String FindMacAddress_JobError;
   public static String FindMacAddress_JobTitle;
   public static String HostSearchResults_ClearLog;
   public static String HostSearchResults_ColIface;
   public static String HostSearchResults_ColIp;
   public static String HostSearchResults_ColMac;
   public static String HostSearchResults_ColNode;
   public static String HostSearchResults_ColPort;
   public static String HostSearchResults_ColSeq;
   public static String HostSearchResults_ColSwitch;
   public static String HostSearchResults_ColType;
   public static String HostSearchResults_ConnectionPoint;
   public static String HostSearchResults_Copy;
   public static String HostSearchResults_CopyIp;
   public static String HostSearchResults_CopyMac;
   public static String HostSearchResults_ModeDirectly;
   public static String HostSearchResults_ModeIndirectly;
   public static String HostSearchResults_NodeConnected;
   public static String HostSearchResults_NodeConnectedToWiFi;
   public static String HostSearchResults_NodeIpMacConnected;
   public static String HostSearchResults_NodeIpMacConnectedToWiFi;
   public static String HostSearchResults_NodeMacConnected;
   public static String HostSearchResults_NodeMacConnectedToWiFi;
   public static String HostSearchResults_NotFound;
   public static String HostSearchResults_ShowError;
   public static String HostSearchResults_Warning;
   public static String RadioInterfaces_ColApMac;
   public static String RadioInterfaces_ColApModel;
   public static String RadioInterfaces_ColApName;
   public static String RadioInterfaces_ColApSerial;
   public static String RadioInterfaces_ColApVendor;
   public static String RadioInterfaces_ColChannel;
   public static String RadioInterfaces_ColRadioIndex;
   public static String RadioInterfaces_ColRadioMac;
   public static String RadioInterfaces_ColRadioName;
   public static String RadioInterfaces_ColTxPowerDbm;
   public static String RadioInterfaces_ColTxPowerMw;
   public static String RadioInterfaces_PartName;
   public static String RoutingTableView_Destination;
   public static String RoutingTableView_Interface;
   public static String RoutingTableView_JobError;
   public static String RoutingTableView_JobTitle;
   public static String RoutingTableView_NextHop;
   public static String RoutingTableView_Title;
   public static String RoutingTableView_Type;
   public static String ShowRadioInterfaces_CannotOpenView;
   public static String ShowRadioInterfaces_Error;
   public static String ShowRoutingTable_CannotOpenView;
   public static String ShowRoutingTable_Error;
   public static String ShowSwitchForwardingDatabase_CannotOpenView;
   public static String ShowSwitchForwardingDatabase_Error;
   public static String ShowVlans_CannotOpenView;
   public static String ShowVlans_Error;
   public static String ShowVlans_JobError;
   public static String ShowVlans_JobTitle;
   public static String ShowWirelessStations_CannotOpenView;
   public static String ShowWirelessStations_Error;
   public static String SubnetAddressMap_BroadcastAddress;
   public static String SubnetAddressMap_Free;
   public static String SubnetAddressMap_JobError;
   public static String SubnetAddressMap_JobTitle;
   public static String SubnetAddressMap_SubnetAddress;
   public static String SwitchForwardingDatabaseView_ColMacAddr;
   public static String SwitchForwardingDatabaseView_ColNode;
   public static String SwitchForwardingDatabaseView_ColPort;
   public static String SwitchForwardingDatabaseView_ColVlan;
   public static String SwitchForwardingDatabaseView_ConIface;
   public static String SwitchForwardingDatabaseView_JobError;
   public static String SwitchForwardingDatabaseView_JobTitle;
   public static String SwitchForwardingDatabaseView_Title;
   public static String VlanView_ColumnID;
   public static String VlanView_ColumnName;
   public static String VlanView_ColumnPorts;
   public static String VlanView_Error;
   public static String VlanView_JobError;
   public static String VlanView_JobTitle;
   public static String VlanView_OpenMapError;
   public static String VlanView_PartName;
   public static String VlanView_ShowVlanMap;
   public static String WirelessStations_ColAp;
   public static String WirelessStations_ColIpAddr;
   public static String WirelessStations_ColMacAddr;
   public static String WirelessStations_ColNode;
   public static String WirelessStations_ColRadio;
   public static String WirelessStations_ColSSID;
   public static String WirelessStations_JobError;
   public static String WirelessStations_JobTitle;
   public static String WirelessStations_PartName;
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
