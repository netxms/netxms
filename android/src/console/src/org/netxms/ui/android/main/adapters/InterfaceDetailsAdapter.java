/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.List;

import org.netxms.client.objects.Interface;
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
 * Adapter for interface details inside interface list
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class InterfaceDetailsAdapter extends BaseAdapter
{
	private final Context context;
	private final List<String> labels = new ArrayList<String>();
	private final List<String> values = new ArrayList<String>();
	private final List<Integer> colors = new ArrayList<Integer>();
	private final Resources r;

	/**
	 * 
	 * @param context
	 */
	public InterfaceDetailsAdapter(Context context)
	{
		this.context = context;
		r = context.getResources();
	}

	/**
	 * Set alarms
	 * 
	 * @param alarms
	 */
	public void setValues(Interface i)
	{
		addRow(r.getString(R.string.if_id), Long.toString(i.getObjectId()));
		addRow(r.getString(R.string.if_name), i.getObjectName());
		addRow(r.getString(R.string.if_type), Integer.toString(i.getIfType()));
		addRow(r.getString(R.string.if_index), Integer.toString(i.getIfIndex()));
		addRow(r.getString(R.string.if_slot), Integer.toString(i.getSlot()));
		addRow(r.getString(R.string.if_port), Integer.toString(i.getPort()));
		addRow(r.getString(R.string.if_description), i.getDescription());
		addRow(r.getString(R.string.if_mac_address), i.getMacAddress().toString());
		addRow(r.getString(R.string.if_ip_address), i.getPrimaryIP().getHostAddress());
		addRow(r.getString(R.string.if_admin_state), i.getAdminStateAsText(), getAdminStateColor(i.getAdminState()));
		addRow(r.getString(R.string.if_oper_state), i.getOperStateAsText(), getOperStateColor(i.getOperState()));
		addRow(r.getString(R.string.if_exp_state), getExpStateText(i.getExpectedState()));
		addRow(r.getString(R.string.if_status), getNodeStatusText(i.getStatus()), getNodeStatusColor(i.getStatus()));
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

		// get row info
		itemName.setText(labels.get(position));
		itemValue.setText(values.get(position));
		itemValue.setTextColor(colors.get(position));
		return view;
	}

	private void addRow(String label, String value)
	{
		labels.add(label);
		values.add(value);
		colors.add(r.getColor(R.color.text_color));
	}

	private void addRow(String label, String value, int color)
	{
		labels.add(label);
		values.add(value);
		colors.add(color);
	}

	private int getAdminStateColor(int state)
	{
		switch (state)
		{
			case Interface.ADMIN_STATE_DOWN:
				return r.getColor(R.color.if_down);
			case Interface.ADMIN_STATE_UP:
				return r.getColor(R.color.if_up);
			case Interface.ADMIN_STATE_UNKNOWN:
			case Interface.ADMIN_STATE_TESTING:
				return r.getColor(R.color.if_ignore);

		}
		return r.getColor(R.color.text_color);
	}

	private int getOperStateColor(int state)
	{
		switch (state)
		{
			case Interface.OPER_STATE_DOWN:
				return r.getColor(R.color.if_down);
			case Interface.OPER_STATE_UP:
				return r.getColor(R.color.if_up);
			case Interface.OPER_STATE_UNKNOWN:
			case Interface.OPER_STATE_TESTING:
				return r.getColor(R.color.if_ignore);

		}
		return r.getColor(R.color.text_color);
	}

	private int getNodeStatusColor(int status)
	{
		final int[] statuses =
		{
				R.color.status_normal,
				R.color.status_warning,
				R.color.status_minor,
				R.color.status_major,
				R.color.status_critical,
				R.color.status_unknown,
				R.color.status_unmanaged,
				R.color.status_disabled,
				R.color.status_testing
		};

		return r.getColor(status >= 0 && status < statuses.length ? statuses[status] : R.color.status_unknown);
	}

	private String getExpStateText(int state)
	{
		switch (state)
		{
			case 0:
				return r.getString(R.string.if_up);
			case 1:
				return r.getString(R.string.if_down);
			case 2:
				return r.getString(R.string.if_ignore);
		}
		return r.getString(R.string.status_unknown);
	}

	private String getNodeStatusText(int status)
	{
		final int[] statuses =
		{
				R.string.status_normal,
				R.string.status_warning,
				R.string.status_minor,
				R.string.status_major,
				R.string.status_critical,
				R.string.status_unknown,
				R.string.status_unmanaged,
				R.string.status_disabled,
				R.string.status_testing
		};

		return r.getString(status >= 0 && status < statuses.length ? statuses[status] : R.string.status_unknown);
	}
}
