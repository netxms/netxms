/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.List;

import org.netxms.base.GeoLocation;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.ui.android.R;

import android.content.Context;
import android.content.res.Resources;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Adapter for overview info
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class OverviewAdapter extends BaseAdapter
{
	private final Context context;
	private final List<String> labels = new ArrayList<String>(0);
	private final List<String> values = new ArrayList<String>(0);
	private final Resources r;

	/**
	 * 
	 * @param context
	 */
	public OverviewAdapter(Context context)
	{
		this.context = context;
		r = context.getResources();
	}

	/**
	 * Set values for overview adapter
	 * 
	 * @param object from which to extract overview data
	 */
	public void setValues(AbstractObject obj)
	{
		labels.clear();
		values.clear();
		if (obj != null)
		{
			addPair(r.getString(R.string.overview_id), Integer.toString((int)obj.getObjectId()));
			addPair(r.getString(R.string.overview_guid), obj.getGuid().toString());
			addPair(r.getString(R.string.overview_class), obj.getClass().getSimpleName());
			addPair(r.getString(R.string.overview_status), getNodeStatus(obj.getStatus()));
			addPair(r.getString(R.string.overview_primary_ip), obj.getPrimaryIP().getHostAddress().toString());
			switch (obj.getObjectClass())
			{
				case AbstractObject.OBJECT_NODE:
					addPair(r.getString(R.string.overview_zone_id), Long.toString(((Node)obj).getZoneId()));
					addPair(r.getString(R.string.overview_primary_hostname), ((Node)obj).getPrimaryName());
					if (((Node)obj).hasAgent())
						addPair(r.getString(R.string.overview_netxms_agent_version), ((Node)obj).getAgentVersion());
					addPair(r.getString(R.string.overview_system_description), ((Node)obj).getSystemDescription(), false);
					addPair(r.getString(R.string.overview_platform_name), ((Node)obj).getPlatformName(), false);
					addPair(r.getString(R.string.overview_snmp_sysname), ((Node)obj).getSnmpSysName(), false);
					addPair(r.getString(R.string.overview_snmp_oid), ((Node)obj).getSnmpOID(), false);
					if ((((Node)obj).getFlags() & Node.NF_IS_BRIDGE) != 0)
						addPair(r.getString(R.string.overview_bridge_base_address), ((Node)obj).getBridgeBaseAddress().toString());
					addPair(r.getString(R.string.overview_driver), ((Node)obj).getDriverName(), false);
					break;
				case AbstractObject.OBJECT_MOBILEDEVICE:
					addPair(r.getString(R.string.overview_last_report), DateFormat.getDateTimeInstance().format(((MobileDevice)obj).getLastReportTime()));
					addPair(r.getString(R.string.overview_device_id), ((MobileDevice)obj).getDeviceId());
					addPair(r.getString(R.string.overview_vendor), ((MobileDevice)obj).getVendor());
					addPair(r.getString(R.string.overview_model), ((MobileDevice)obj).getModel());
					addPair(r.getString(R.string.overview_serial_number), ((MobileDevice)obj).getSerialNumber());
					addPair(r.getString(R.string.overview_os_name), ((MobileDevice)obj).getOsName());
					addPair(r.getString(R.string.overview_os_version), ((MobileDevice)obj).getOsVersion());
					addPair(r.getString(R.string.overview_user), ((MobileDevice)obj).getUserId());
					addPair(r.getString(R.string.overview_battery_level), Integer.toString(((MobileDevice)obj).getBatteryLevel()) + "%");
					break;
			}
			addPair(r.getString(R.string.overview_location), obj.getGeolocation());
		}
	}
	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getCount()
	 */
	@Override
	public int getCount()
	{
		return values.size();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
		return values.get(position);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItemId(int)
	 */
	@Override
	public long getItemId(int position)
	{
		return position;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getView(int, android.view.View,
	 * android.view.ViewGroup)
	 */
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		TextView itemName, itemValue;
		LinearLayout view;

		if (convertView == null)
		{
			itemName = new TextView(context);
			itemName.setPadding(5, 2, 5, 2);
			itemName.setTextColor(r.getColor(R.color.text_color));
			itemName.setGravity(Gravity.LEFT);
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.LEFT;
			itemName.setLayoutParams(lp);

			itemValue = new TextView(context);
			itemValue.setPadding(5, 2, 5, 2);
			itemValue.setTextColor(r.getColor(R.color.text_color));
			itemValue.setGravity(Gravity.RIGHT);
			lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.RIGHT;
			itemValue.setLayoutParams(lp);

			view = new LinearLayout(context);
			//view.setWeightSum((float)1.0);
			view.addView(itemName);
			view.addView(itemValue);
		}
		else
		{
			// get reference to existing view
			view = (LinearLayout)convertView;
			itemName = (TextView)view.getChildAt(0);
			itemValue = (TextView)view.getChildAt(1);
		}

		// get node current name/value pair
		itemName.setText(labels.get(position));
		itemValue.setText(values.get(position));
		return view;
	}

	/**
	 * Add a pair label/geolocation value
	 * 
	 * @param label Label to print
	 * @param value GeoLocation value to print 
	 */
	private void addPair(String label, GeoLocation value)
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
			addPair(label, latlon + provider + accuracy + timestamp);
		}
	}

	/**
	 * Add a pair label/value with possibly empty values
	 * 
	 * @param label Label to print
	 * @param value String value to print
	 * @param allowEmpty True to allow empty field (show only label) 
	 */
	private void addPair(String label, String value, boolean allowEmpty)
	{
		if (allowEmpty || (value != null && value.length() != 0))
			addPair(label, value);
	}

	/**
	 * Add a pair label/value
	 * 
	 * @param label	Label to print
	 * @param value	String value to print 
	 */
	private void addPair(String label, String value)
	{
		labels.add(label);
		values.add(value);
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
	private String getNodeStatus(int status)
	{
		switch (status)
		{
			case Severity.NORMAL:
				return r.getString(R.string.status_normal);
			case Severity.WARNING:
				return r.getString(R.string.status_warning);
			case Severity.MINOR:
				return r.getString(R.string.status_minor);
			case Severity.MAJOR:
				return r.getString(R.string.status_major);
			case Severity.CRITICAL:
				return r.getString(R.string.status_critical);
			case Severity.UNKNOWN:
				return r.getString(R.string.status_unknown);
			case Severity.UNMANAGED:
				return r.getString(R.string.status_unmanaged);
			case Severity.DISABLED:
				return r.getString(R.string.status_disabled);
			case Severity.TESTING:
				return r.getString(R.string.status_testing);
		}
		return r.getString(R.string.status_unknown);
	}
}
