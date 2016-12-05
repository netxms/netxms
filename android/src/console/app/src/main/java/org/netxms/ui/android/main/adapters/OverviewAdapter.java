/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.netxms.base.GeoLocation;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.ui.android.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ListView;
import android.widget.TextView;

/**
 * Adapter for overview info
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class OverviewAdapter extends BaseExpandableListAdapter
{
	private class Details
	{
		public String name = "";
		public Map<String, String> info = null;

		public Details()
		{
			info = new HashMap<String, String>();
		}
	}

	private final Context context;
	private final ArrayList<String> groups;
	private final ArrayList<ArrayList<Details>> children;
	private final Resources r;

	/**
	 * 
	 * @param context
	 */
	public OverviewAdapter(Context context, ArrayList<String> groups, ArrayList<ArrayList<Details>> children)
	{
		this.context = context;
		this.groups = new ArrayList<String>(0);
		this.children = new ArrayList<ArrayList<Details>>(0);
		r = context.getResources();
	}

	/**
	 * Set values for overview adapter
	 * 
	 * @param object from which to extract overview data
	 */
	public void setValues(AbstractObject obj)
	{
		groups.clear();
		children.clear();
		if (obj != null)
		{
			Details general = new Details();
			general.name = "General";
			Details comments = new Details();
			comments.name = "Comments";
			Details capabilities = new Details();
			capabilities.name = "Capabilities";
			// Gather "General" info
			addPair(general.info, r.getString(R.string.overview_id), Integer.toString((int)obj.getObjectId()));
			addPair(general.info, r.getString(R.string.overview_guid), toString(obj.getGuid()));
			addPair(general.info, r.getString(R.string.overview_class), obj.getClass().getSimpleName());
			addPair(general.info, r.getString(R.string.overview_status), getNodeStatus(obj.getStatus()));
			switch (obj.getObjectClass())
			{
				case AbstractObject.OBJECT_NODE:
					addPair(general.info, r.getString(R.string.overview_zone_id), Long.toString(((Node)obj).getZoneId()));
					addPair(general.info, r.getString(R.string.overview_primary_hostname), ((Node)obj).getPrimaryName());
					addPair(general.info, r.getString(R.string.overview_primary_ip), ((Node)obj).getPrimaryIP().getHostAddress());
					if (((Node)obj).hasAgent())
						addPair(general.info, r.getString(R.string.overview_netxms_agent_version), ((Node)obj).getAgentVersion());
					addPair(general.info, r.getString(R.string.overview_system_description), ((Node)obj).getSystemDescription(), false);
					addPair(general.info, r.getString(R.string.overview_platform_name), ((Node)obj).getPlatformName(), false);
					addPair(general.info, r.getString(R.string.overview_snmp_sysname), ((Node)obj).getSnmpSysName(), false);
					addPair(general.info, r.getString(R.string.overview_snmp_oid), ((Node)obj).getSnmpOID(), false);
					if ((((Node)obj).getFlags() & Node.NF_IS_BRIDGE) != 0)
						addPair(general.info, r.getString(R.string.overview_bridge_base_address), toString(((Node)obj).getBridgeBaseAddress()));
					addPair(general.info, r.getString(R.string.overview_driver), ((Node)obj).getDriverName(), false);
					addPair(general.info, r.getString(R.string.overview_boot_time), toString(((Node)obj).getBootTime()), false);
					// Gather "Capabilities" info
					addPair(capabilities.info, r.getString(R.string.overview_is_native_agent), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_NATIVE_AGENT) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_bridge), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_BRIDGE) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_cdp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_CDP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is8021x), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_8021X) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_lldp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_LLDP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_sonmp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_SONMP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_printer), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_PRINTER) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_router), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_ROUTER) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_smclp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_SMCLP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_snmp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_SNMP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_stp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_STP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_is_vrrp), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_IS_VRRP) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_has_entity_mib), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_HAS_ENTITY_MIB) != 0));
					addPair(capabilities.info, r.getString(R.string.overview_has_ifxtable), getYesNo((((Node)obj).getFlags() & AbstractNode.NF_HAS_IFXTABLE) != 0));
					if ((((Node)obj).getFlags() & AbstractNode.NF_IS_SNMP) != 0)
					{
						addPair(capabilities.info, r.getString(R.string.overview_snmp_port), Integer.toString(((Node)obj).getSnmpPort()));
						addPair(capabilities.info, r.getString(R.string.overview_snmp_version), getSnmpVersionName(((Node)obj).getSnmpVersion()));
					}

					break;
				case AbstractObject.OBJECT_MOBILEDEVICE:
					addPair(general.info, r.getString(R.string.overview_last_report), DateFormat.getDateTimeInstance().format(((MobileDevice)obj).getLastReportTime()));
					addPair(general.info, r.getString(R.string.overview_device_id), ((MobileDevice)obj).getDeviceId());
					addPair(general.info, r.getString(R.string.overview_vendor), ((MobileDevice)obj).getVendor());
					addPair(general.info, r.getString(R.string.overview_model), ((MobileDevice)obj).getModel());
					addPair(general.info, r.getString(R.string.overview_serial_number), ((MobileDevice)obj).getSerialNumber());
					addPair(general.info, r.getString(R.string.overview_os_name), ((MobileDevice)obj).getOsName());
					addPair(general.info, r.getString(R.string.overview_os_version), ((MobileDevice)obj).getOsVersion());
					addPair(general.info, r.getString(R.string.overview_user), ((MobileDevice)obj).getUserId());
					addPair(general.info, r.getString(R.string.overview_battery_level), Integer.toString(((MobileDevice)obj).getBatteryLevel()) + "%");
					break;
			}
			addPair(general.info, r.getString(R.string.overview_location), obj.getGeolocation());
			groups.add(general.name);
			children.add(new ArrayList<Details>());
			children.get(0).add(general);
			// Gather "comments" info
			addPair(comments.info, r.getString(R.string.overview_comments), obj.getComments());
			groups.add(comments.name);
			children.add(new ArrayList<Details>());
			children.get(1).add(comments);
			groups.add(capabilities.name);
			children.add(new ArrayList<Details>());
			children.get(2).add(capabilities);
		}
	}

	@Override
	public Object getChild(int groupPosition, int childPosition)
	{
		if (groupPosition >= 0 && groupPosition < children.size())
			if (childPosition >= 0 && childPosition < children.get(groupPosition).size())
				return children.get(groupPosition).get(childPosition);
		return null;
	}

	@Override
	public long getChildId(int groupPosition, int childPosition)
	{
		return childPosition;
	}

	@SuppressLint("InflateParams")
	private ViewGroup getViewGroupChild(View convertView, ViewGroup parent)
	{
		// The parent will be our ListView from the ListActivity 
		if (convertView instanceof ViewGroup)
		{
			return (ViewGroup)convertView;
		}
		Context context = parent.getContext();
		LayoutInflater inflater = LayoutInflater.from(context);
		ViewGroup item = (ViewGroup)inflater.inflate(R.layout.overview_child, null);
		return item;
	}

	@Override
	public View getChildView(int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent)
	{
		ViewGroup item = getViewGroupChild(convertView, parent);
		ListView label = (ListView)item.findViewById(R.id.overviewChild);
		OverviewDetailsAdapter idAdapter = new OverviewDetailsAdapter(parent.getContext());
		Details details = (Details)getChild(groupPosition, childPosition);
		if (details != null)
			idAdapter.setValues(details.info);
		label.setAdapter(idAdapter);
		TextView tv = (TextView)parent.findViewById(R.id.overview_text);
		int totalHeight = tv != null ? tv.getMeasuredHeight() : 0;
		for (int i = 0; i < idAdapter.getCount(); i++)
		{
			View listItem = idAdapter.getView(i, null, label);
			listItem.measure(0, 0);
			totalHeight += listItem.getMeasuredHeight();
		}
		ViewGroup.LayoutParams params = label.getLayoutParams();
		params.height = totalHeight + (label.getDividerHeight() * (idAdapter.getCount() - 1));
		label.setLayoutParams(params);
		label.requestLayout();
		return item;
	}

	@Override
	public int getChildrenCount(int groupPosition)
	{
		if (groupPosition < children.size())
			return children.get(groupPosition).size();
		return 0;
	}

	@Override
	public Object getGroup(int groupPosition)
	{
		return groups.get(groupPosition);
	}

	@Override
	public int getGroupCount()
	{
		return groups.size();
	}

	@Override
	public long getGroupId(int groupPosition)
	{
		return groupPosition;
	}

	@SuppressLint("InflateParams")
	@Override
	public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent)
	{
		if (convertView == null)
		{
			LayoutInflater layoutInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			convertView = layoutInflater.inflate(R.layout.overview_group, null);
		}
		TextView tv = (TextView)convertView.findViewById(R.id.overview_text);
		Details d = (Details)getChild(groupPosition, 0);
		if (d != null)
			tv.setText(d.name);

		return convertView;
	}

	@Override
	public boolean hasStableIds()
	{
		return true;
	}

	@Override
	public boolean isChildSelectable(int groupPosition, int childPosition)
	{
		return true;
	}

	/**
	 * Add a pair label/geolocation value
	 * 
	 * @param label Label to print
	 * @param value GeoLocation value to print 
	 */
	private void addPair(Map<String, String> map, String label, GeoLocation value)
	{
		if (value != null && value.getType() != GeoLocation.UNSET)
		{
			String provider = "";
			if (getProvider(value.getType()).length() > 0)
				provider = " - " + r.getString(R.string.overview_provider, getProvider(value.getType()));
			String accuracy = "";
			if (value.getAccuracy() > 0)
				accuracy = " - " + r.getString(R.string.overview_accuracy, Integer.toString(value.getAccuracy()));
			String timestamp = "";
			if (value.getTimestamp() != null)
				timestamp = " - " + r.getString(R.string.overview_timestamp, DateFormat.getDateTimeInstance().format(value.getTimestamp()));
			String latlon = value.getLatitudeAsString() + " " + value.getLongitudeAsString();
			addPair(map, label, latlon + provider + accuracy + timestamp);
		}
	}

	/**
	 * Add a pair label/value with possibly empty values
	 * 
	 * @param label Label to print
	 * @param value String value to print
	 * @param allowEmpty True to allow empty field (show only label) 
	 */
	private void addPair(Map<String, String> map, String label, String value, boolean allowEmpty)
	{
		if (allowEmpty || (value != null && value.length() != 0))
			addPair(map, label, value);
	}

	/**
	 * Add a pair label/value
	 * 
	 * @param label	Label to print
	 * @param value	String value to print 
	 */
	private void addPair(Map<String, String> map, String label, String value)
	{
		map.put(label, value);
	}

	/**
	 * Get provider type in human readable form
	 * 
	 * @param type NetXMS type of provider
	 * @return Human readable type of provider 
	 */
	private String getProvider(int type)
	{
		switch (type)
		{
			case GeoLocation.MANUAL:
				return r.getString(R.string.provider_manual);
			case GeoLocation.GPS:
				return r.getString(R.string.provider_gps);
			case GeoLocation.NETWORK:
				return r.getString(R.string.provider_network);
		}
		return "";
	}

	/**
	 * Get node status in human readable form
	 * 
	 * @param status NetXMS type of status
	 * @return Human readable status 
	 */
	private String getNodeStatus(ObjectStatus status)
	{
		switch (status)
		{
			case NORMAL:
				return r.getString(R.string.status_normal);
			case WARNING:
				return r.getString(R.string.status_warning);
			case MINOR:
				return r.getString(R.string.status_minor);
			case MAJOR:
				return r.getString(R.string.status_major);
			case CRITICAL:
				return r.getString(R.string.status_critical);
			case UNKNOWN:
				return r.getString(R.string.status_unknown);
			case UNMANAGED:
				return r.getString(R.string.status_unmanaged);
			case DISABLED:
				return r.getString(R.string.status_disabled);
			case TESTING:
				return r.getString(R.string.status_testing);
		}
		return r.getString(R.string.status_unknown);
	}

	/**
	 * Covert object to string, protecting from null value
	 * 
	 * @param obj Object to convert to string
	 * @return Object converted to string 
	 */
	private String toString(Object obj)
	{
		return obj != null ? obj.toString() : "";
	}

	/**
	 * Get flag value in Yes/No form
	 * 
	 * @param status NetXMS type of status
	 * @return Human readable flag
	 */
	private String getYesNo(boolean flag)
	{
		return r.getString(flag ? R.string.yes : R.string.no);
	}

	/**
	 * Get SNMP version name
	 * 
	 * @param status NetXMS type of status
	 * @return Human readable flag
	 */
	private String getSnmpVersionName(int version)
	{
		switch (version)
		{
			case AbstractNode.SNMP_VERSION_1:
				return "1"; //$NON-NLS-1$
			case AbstractNode.SNMP_VERSION_2C:
				return "2c"; //$NON-NLS-1$
			case AbstractNode.SNMP_VERSION_3:
				return "3"; //$NON-NLS-1$
			default:
				return "???"; //$NON-NLS-1$
		}
	}
}
