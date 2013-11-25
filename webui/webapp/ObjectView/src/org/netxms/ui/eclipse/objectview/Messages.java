package org.netxms.ui.eclipse.objectview;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objectview.messages"; //$NON-NLS-1$
   public String AvailabilityChart_Down;
   public String AvailabilityChart_Downtime;
   public String AvailabilityChart_ThisMonth;
   public String AvailabilityChart_ThisWeek;
   public String AvailabilityChart_Title;
   public String AvailabilityChart_Today;
   public String AvailabilityChart_Up;
   public String AvailabilityChart_Uptime;
   public String Capabilities_FlagHasEntityMIB;
   public String Capabilities_FlagHasIfXTable;
   public String Capabilities_FlagIsAgent;
   public String Capabilities_FlagIsBridge;
   public String Capabilities_FlagIsCDP;
   public String Capabilities_FlagIsDot1x;
   public String Capabilities_FlagIsLLDP;
   public String Capabilities_FlagIsNDP;
   public String Capabilities_FlagIsPrinter;
   public String Capabilities_FlagIsRouter;
   public String Capabilities_FlagIsSMCLP;
   public String Capabilities_FlagIsSNMP;
   public String Capabilities_FlagIsSTP;
   public String Capabilities_FlagIsVRRP;
   public String Capabilities_No;
   public String Capabilities_SNMPPort;
   public String Capabilities_SNMPVersion;
   public String Capabilities_Title;
   public String Capabilities_Yes;
   public String ClusterResourceListLabelProvider_None;
   public String ClusterTab_Owner;
   public String ClusterTab_Resource;
   public String ClusterTab_VirtualIP;
   public String Commands_ActionRestartAgent;
   public String Commands_ActionRestartNode;
   public String Commands_ActionShutdown;
   public String Commands_ActionWakeup;
   public String Commands_AgentRestartConfirmation;
   public String Commands_AgentRestartJobError;
   public String Commands_AgentRestartJobName;
   public String Commands_Confirmation;
   public String Commands_RestartNodeConfirmation;
   public String Commands_RestartNodeJobError;
   public String Commands_RestartNodeJobName;
   public String Commands_ShutdownConfirmation;
   public String Commands_ShutdownJobError;
   public String Commands_ShutdownJobName;
   public String Commands_Title;
   public String Commands_WakeupJobError;
   public String Commands_WakeupJobName;
   public String Comments_Title;
   public String ComponentsTab_ActionCollapseAll;
   public String ComponentsTab_ActionCopy;
   public String ComponentsTab_ActionCopyModel;
   public String ComponentsTab_ActionCopyName;
   public String ComponentsTab_ActionCopySerial;
   public String ComponentsTab_ActionExpandAll;
   public String ComponentsTab_ColClass;
   public String ComponentsTab_ColFirmware;
   public String ComponentsTab_ColModel;
   public String ComponentsTab_ColName;
   public String ComponentsTab_ColSerial;
   public String ComponentsTab_ColVendor;
   public String ComponentsTab_JobError;
   public String ComponentsTab_JobName;
   public String ComponentTreeLabelProvider_ClassBackplane;
   public String ComponentTreeLabelProvider_ClassChassis;
   public String ComponentTreeLabelProvider_ClassContainer;
   public String ComponentTreeLabelProvider_ClassFan;
   public String ComponentTreeLabelProvider_ClassModule;
   public String ComponentTreeLabelProvider_ClassOther;
   public String ComponentTreeLabelProvider_ClassPort;
   public String ComponentTreeLabelProvider_ClassPS;
   public String ComponentTreeLabelProvider_ClassSensor;
   public String ComponentTreeLabelProvider_ClassStack;
   public String ComponentTreeLabelProvider_ClassUnknown;
   public String Connection_NA;
   public String Connection_Title;
   public String GeneralInfo_8021xBackend;
   public String GeneralInfo_8021xPAE;
   public String GeneralInfo_AdmState;
   public String GeneralInfo_AgentVersion;
   public String GeneralInfo_BatteryLevel;
   public String GeneralInfo_BootTime;
   public String GeneralInfo_BridgeBaseAddress;
   public String GeneralInfo_Class;
   public String GeneralInfo_Description;
   public String GeneralInfo_DeviceId;
   public String GeneralInfo_Driver;
   public String GeneralInfo_GUID;
   public String GeneralInfo_ID;
   public String GeneralInfo_IfIndex;
   public String GeneralInfo_IfType;
   public String GeneralInfo_IPAddr;
   public String GeneralInfo_IPNetMask;
   public String GeneralInfo_IsTemplate;
   public String GeneralInfo_LastReport;
   public String GeneralInfo_LinkedNode;
   public String GeneralInfo_Location;
   public String GeneralInfo_MACAddr;
   public String GeneralInfo_Model;
   public String GeneralInfo_Never;
   public String GeneralInfo_No;
   public String GeneralInfo_OperState;
   public String GeneralInfo_OS;
   public String GeneralInfo_OSVersion;
   public String GeneralInfo_PlatformName;
   public String GeneralInfo_PrimaryHostName;
   public String GeneralInfo_PrimaryIP;
   public String GeneralInfo_Serial;
   public String GeneralInfo_SlotPort;
   public String GeneralInfo_Status;
   public String GeneralInfo_SysDescr;
   public String GeneralInfo_SysName;
   public String GeneralInfo_SysOID;
   public String GeneralInfo_Template;
   public String GeneralInfo_Title;
   public String GeneralInfo_UptimeDay;
   public String GeneralInfo_UptimeMonth;
   public String GeneralInfo_UptimeWeek;
   public String GeneralInfo_User;
   public String GeneralInfo_Vendor;
   public String GeneralInfo_Yes;
   public String GeneralInfo_ZoneId;
   public String InterfaceListLabelProvider_StateDown;
   public String InterfaceListLabelProvider_StateIgnore;
   public String InterfaceListLabelProvider_StateUp;
   public String InterfacesTab_ActionCopy;
   public String InterfacesTab_ActionCopyIP;
   public String InterfacesTab_ActionCopyMAC;
   public String InterfacesTab_ActionCopyPeerIP;
   public String InterfacesTab_ActionCopyPeerMAC;
   public String InterfacesTab_ActionCopyPeerName;
   public String InterfacesTab_Col8021xBackend;
   public String InterfacesTab_Col8021xPAE;
   public String InterfacesTab_ColAdminState;
   public String InterfacesTab_ColDescription;
   public String InterfacesTab_ColExpState;
   public String InterfacesTab_ColId;
   public String InterfacesTab_ColIfIndex;
   public String InterfacesTab_ColIfType;
   public String InterfacesTab_ColIpAddr;
   public String InterfacesTab_ColMacAddr;
   public String InterfacesTab_ColName;
   public String InterfacesTab_ColOperState;
   public String InterfacesTab_ColPeerIP;
   public String InterfacesTab_ColPeerMAC;
   public String InterfacesTab_ColPeerNode;
   public String InterfacesTab_ColPort;
   public String InterfacesTab_ColSlot;
   public String InterfacesTab_ColStatus;
   public String ObjectStatusMapView_ActionGroupNodes;
   public String ObjectStatusMapView_PartName;
   public String ShowObjectDetailsView_Error;
   public String ShowObjectDetailsView_ErrorOpeningView;
   public String ShowSoftwareInventory_Error;
   public String ShowSoftwareInventory_ErrorOpeningView;
   public String ShowStatusMap_Error;
   public String ShowStatusMap_ErrorOpeningView;
   public String SoftwareInventory_Description;
   public String SoftwareInventory_InstallDate;
   public String SoftwareInventory_JobError;
   public String SoftwareInventory_JobName;
   public String SoftwareInventory_Name;
   public String SoftwareInventory_URL;
   public String SoftwareInventory_Vendor;
   public String SoftwareInventory_Version;
   public String SoftwareInventoryView_PartName;
   public String TableElement_ActionCopy;
   public String TableElement_ActionCopyName;
   public String TableElement_ActionCopyValue;
   public String TableElement_Name;
   public String TableElement_Value;
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
