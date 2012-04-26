/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.objects.GenericObject;
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
 * Adapter for alarm list
 * 
 */

public class OverviewAdapter extends BaseAdapter
{
	private Context context;
	private List<String> labels = new ArrayList<String>(0);
	private List<String> values = new ArrayList<String>(0);
	private Resources r;

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
	 * Set alarms
	 * 
	 * @param alarms
	 */
	public void setValues(Node node)
	{
		labels.add(r.getString(R.string.overview_id));
		values.add(Integer.toString((int)node.getObjectId()));
		labels.add(r.getString(R.string.overview_class));
		values.add(node.getClass().getSimpleName());
		labels.add(r.getString(R.string.overview_status));
		values.add(getNodeStatus(node.getStatus()));
		labels.add(r.getString(R.string.overview_primary_ip));
		values.add(node.getPrimaryIP().getHostAddress().toString());
		labels.add(r.getString(R.string.overview_system_description));
		values.add(node.getSystemDescription());
		if (!node.getAgentVersion().isEmpty())
		{
			labels.add(r.getString(R.string.overview_netxms_agent));
			values.add(node.getAgentVersion());
			labels.add(r.getString(R.string.overview_agent_version));
			values.add(node.getAgentVersion());
		}
		if (!node.getPlatformName().isEmpty())
		{
			labels.add(r.getString(R.string.overview_platform_name));
			values.add(node.getPlatformName());
		}
		if (!node.getSnmpSysName().isEmpty())
		{
			labels.add(r.getString(R.string.overview_snmp_agent));
			values.add(node.getSnmpSysName());
			labels.add(r.getString(R.string.overview_snmp_oid));
			values.add(node.getSnmpOID());
		}
	}

	private String getNodeStatus(int status)
	{
		switch (status)
		{
			case GenericObject.STATUS_NORMAL:
				return r.getString(R.string.status_normal);
			case GenericObject.STATUS_WARNING:
				return r.getString(R.string.status_warning);
			case GenericObject.STATUS_MINOR:
				return r.getString(R.string.status_minor);
			case GenericObject.STATUS_MAJOR:
				return r.getString(R.string.status_major);
			case GenericObject.STATUS_CRITICAL:
				return r.getString(R.string.status_critical);
			case GenericObject.STATUS_UNKNOWN:
				return r.getString(R.string.status_unknown);
			case GenericObject.STATUS_UNMANAGED:
				return r.getString(R.string.status_unmanaged);
			case GenericObject.STATUS_DISABLED:
				return r.getString(R.string.status_disabled);
			case GenericObject.STATUS_TESTING:        
				return r.getString(R.string.status_testing);
		}
		return r.getString(R.string.status_unknown);
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
			lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
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
}
