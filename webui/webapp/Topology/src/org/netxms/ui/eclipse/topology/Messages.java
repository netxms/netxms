package org.netxms.ui.eclipse.topology;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.topology.messages"; //$NON-NLS-1$
   public String ConnectionPointLabelProvider_Direct;
   public String ConnectionPointLabelProvider_Indirect;
   public String ConnectionPointLabelProvider_Unknown;
   public String ConnectionPointLabelProvider_Wireless;
   public String DeviceView_SlotName;
   public String EnterIpAddressDlg_EnterValidAddress;
   public String EnterIpAddressDlg_IpAddress;
   public String EnterIpAddressDlg_SelectZone;
   public String EnterIpAddressDlg_Title;
   public String EnterIpAddressDlg_Warning;
   public String EnterIpAddressDlg_Zone;
   public String EnterMacAddressDlg_Error;
   public String EnterMacAddressDlg_IncorrectMacAddress;
   public String EnterMacAddressDlg_MacAddress;
   public String EnterMacAddressDlg_Title;
   public String FindConnectionPoint_JobError;
   public String FindConnectionPoint_JobTitle;
   public String FindIpAddress_JobError;
   public String FindIpAddress_JobTitle;
   public String FindMacAddress_JobError;
   public String FindMacAddress_JobTitle;
   public String HostSearchResults_ClearLog;
   public String HostSearchResults_ColIface;
   public String HostSearchResults_ColIp;
   public String HostSearchResults_ColMac;
   public String HostSearchResults_ColNode;
   public String HostSearchResults_ColPort;
   public String HostSearchResults_ColSeq;
   public String HostSearchResults_ColSwitch;
   public String HostSearchResults_ColType;
   public String HostSearchResults_ConnectionPoint;
   public String HostSearchResults_Copy;
   public String HostSearchResults_CopyIp;
   public String HostSearchResults_CopyMac;
   public String HostSearchResults_ModeDirectly;
   public String HostSearchResults_ModeIndirectly;
   public String HostSearchResults_NodeConnected;
   public String HostSearchResults_NodeConnectedToWiFi;
   public String HostSearchResults_NodeIpMacConnected;
   public String HostSearchResults_NodeIpMacConnectedToWiFi;
   public String HostSearchResults_NodeMacConnected;
   public String HostSearchResults_NodeMacConnectedToWiFi;
   public String HostSearchResults_NotFound;
   public String HostSearchResults_ShowError;
   public String HostSearchResults_Warning;
   public String RadioInterfaces_ColApMac;
   public String RadioInterfaces_ColApModel;
   public String RadioInterfaces_ColApName;
   public String RadioInterfaces_ColApSerial;
   public String RadioInterfaces_ColApVendor;
   public String RadioInterfaces_ColChannel;
   public String RadioInterfaces_ColRadioIndex;
   public String RadioInterfaces_ColRadioMac;
   public String RadioInterfaces_ColRadioName;
   public String RadioInterfaces_ColTxPowerDbm;
   public String RadioInterfaces_ColTxPowerMw;
   public String RadioInterfaces_PartName;
   public String RoutingTableView_Destination;
   public String RoutingTableView_Interface;
   public String RoutingTableView_JobError;
   public String RoutingTableView_JobTitle;
   public String RoutingTableView_NextHop;
   public String RoutingTableView_Title;
   public String RoutingTableView_Type;
   public String ShowRadioInterfaces_CannotOpenView;
   public String ShowRadioInterfaces_Error;
   public String ShowRoutingTable_CannotOpenView;
   public String ShowRoutingTable_Error;
   public String ShowSwitchForwardingDatabase_CannotOpenView;
   public String ShowSwitchForwardingDatabase_Error;
   public String ShowVlans_CannotOpenView;
   public String ShowVlans_Error;
   public String ShowVlans_JobError;
   public String ShowVlans_JobTitle;
   public String ShowWirelessStations_CannotOpenView;
   public String ShowWirelessStations_Error;
   public String SubnetAddressMap_BroadcastAddress;
   public String SubnetAddressMap_Free;
   public String SubnetAddressMap_JobError;
   public String SubnetAddressMap_JobTitle;
   public String SubnetAddressMap_SubnetAddress;
   public String SwitchForwardingDatabaseView_ColMacAddr;
   public String SwitchForwardingDatabaseView_ColNode;
   public String SwitchForwardingDatabaseView_ColPort;
   public String SwitchForwardingDatabaseView_ColVlan;
   public String SwitchForwardingDatabaseView_ConIface;
   public String SwitchForwardingDatabaseView_JobError;
   public String SwitchForwardingDatabaseView_JobTitle;
   public String SwitchForwardingDatabaseView_Title;
   public String VlanView_ColumnID;
   public String VlanView_ColumnName;
   public String VlanView_ColumnPorts;
   public String VlanView_Error;
   public String VlanView_JobError;
   public String VlanView_JobTitle;
   public String VlanView_OpenMapError;
   public String VlanView_PartName;
   public String VlanView_ShowVlanMap;
   public String WirelessStations_ColAp;
   public String WirelessStations_ColIpAddr;
   public String WirelessStations_ColMacAddr;
   public String WirelessStations_ColNode;
   public String WirelessStations_ColRadio;
   public String WirelessStations_ColSSID;
   public String WirelessStations_JobError;
   public String WirelessStations_JobTitle;
   public String WirelessStations_PartName;
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
