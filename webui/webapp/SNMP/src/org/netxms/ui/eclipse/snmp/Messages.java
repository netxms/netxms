package org.netxms.ui.eclipse.snmp;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.snmp.messages"; //$NON-NLS-1$
	public String LoginListener_JobError;
	public String LoginListener_JobTitle;
	public String MibExplorer_CopyName;
	public String MibExplorer_CopyToClipboard;
	public String MibExplorer_CopyType;
	public String MibExplorer_CopyValue;
	public String MibExplorer_OID;
	public String MibExplorer_SetNodeObject;
	public String MibExplorer_Type;
	public String MibExplorer_Value;
	public String MibExplorer_Walk;
	public String MibExplorer_WalkJob_Error;
	public String MibExplorer_WalkJob_Title;
	public String MibObjectDetails_8;
	public String MibObjectDetails_Access;
	public String MibObjectDetails_OID;
	public String MibObjectDetails_Status;
	public String MibObjectDetails_Type;
	public String MibSelectionDialog_MIBTree;
	public String MibSelectionDialog_OID;
	public String MibSelectionDialog_Title;
	public String MibSelectionDialog_Warning;
	public String MibSelectionDialog_WarningText;
	public String OpenMibExplorer_Error;
	public String OpenMibExplorer_ErrorText;
	public String OpenMibExplorerForNode_Error;
	public String OpenMibExplorerForNode_ErrorText;
	public String OpenSnmpTrapMonitor_Error;
	public String OpenSnmpTrapMonitor_ErrorText;
	public String OpenSnmpTrapMonitor_JobError;
	public String OpenSnmpTrapMonitor_JobTitle;
	public String OpenTrapEditor_Error;
	public String OpenTrapEditor_ErrorText;
	public String ParamMappingEditDialog_ByOID;
	public String ParamMappingEditDialog_ByPos;
	public String ParamMappingEditDialog_Description;
	public String ParamMappingEditDialog_EnterVarbindPos;
	public String ParamMappingEditDialog_NeverConvertToHex;
	public String ParamMappingEditDialog_Options;
	public String ParamMappingEditDialog_Select;
	public String ParamMappingEditDialog_Title;
	public String ParamMappingEditDialog_Varbind;
	public String ParamMappingEditDialog_Warning;
	public String ParamMappingEditDialog_WarningInvalidOID;
	public String ParamMappingLabelProvider_PositionPrefix;
	public String SnmpTrapComparator_Unknown;
	public String SnmpTrapEditor_ColDescription;
	public String SnmpTrapEditor_ColEvent;
	public String SnmpTrapEditor_ColID;
	public String SnmpTrapEditor_ColOID;
	public String SnmpTrapEditor_CreateJob_Error;
	public String SnmpTrapEditor_CreateJob_Title;
	public String SnmpTrapEditor_Delete;
	public String SnmpTrapEditor_DeleteJob_Error;
	public String SnmpTrapEditor_DeleteJob_Title;
	public String SnmpTrapEditor_LoadJob_Error;
	public String SnmpTrapEditor_LoadJob_Title;
	public String SnmpTrapEditor_ModifyJob_Error;
	public String SnmpTrapEditor_ModifyJob_Title;
	public String SnmpTrapEditor_NewMapping;
	public String SnmpTrapEditor_Properties;
	public String SnmpTrapLabelProvider_Unknown;
	public String SnmpTrapMonitor_ColOID;
	public String SnmpTrapMonitor_ColSourceIP;
	public String SnmpTrapMonitor_ColSourceNode;
	public String SnmpTrapMonitor_ColTime;
	public String SnmpTrapMonitor_ColVarbinds;
	public String SnmpTrapMonitor_SubscribeJob_Error;
	public String SnmpTrapMonitor_SubscribeJob_Title;
	public String SnmpTrapMonitor_UnsubscribeJob_Error;
	public String SnmpTrapMonitor_UnsubscribeJob_Title;
	public String SnmpTrapMonitorLabelProvider_Unknown;
	public String TrapConfigurationDialog_Add;
	public String TrapConfigurationDialog_Delete;
	public String TrapConfigurationDialog_Description;
	public String TrapConfigurationDialog_Edit;
	public String TrapConfigurationDialog_Event;
	public String TrapConfigurationDialog_MoveDown;
	public String TrapConfigurationDialog_MoveUp;
	public String TrapConfigurationDialog_Number;
	public String TrapConfigurationDialog_Parameter;
	public String TrapConfigurationDialog_Parameters;
	public String TrapConfigurationDialog_Select;
	public String TrapConfigurationDialog_Title;
	public String TrapConfigurationDialog_TrapOID;
	public String TrapConfigurationDialog_UserTag;
	public String TrapConfigurationDialog_Warning;
	public String TrapConfigurationDialog_WarningInvalidOID;
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


