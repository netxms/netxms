/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import org.netxms.client.events.Alarm;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

/**
 * Adapter for alarm list
 *
 */
public class AlarmListAdapter extends BaseAdapter
{
	private Context context;
	private Alarm[] alarms = new Alarm[0];
	
	/**
	 * 
	 * @param context
	 */
	public AlarmListAdapter(Context context)
	{
		this.context = context;
	}
	
	/**
	 * Set alarms
	 * @param alarms
	 */
	public void setAlarms(Alarm[] alarms)
	{
		this.alarms = alarms;
		notifyDataSetChanged();
	}
	
	/* (non-Javadoc)
	 * @see android.widget.Adapter#getCount()
	 */
	@Override
	public int getCount()
	{
		return alarms.length;
	}

	/* (non-Javadoc)
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
		return alarms[position];
	}

	/* (non-Javadoc)
	 * @see android.widget.Adapter#getItemId(int)
	 */
	@Override
	public long getItemId(int position)
	{
		return alarms[position].getId();
	}

	/* (non-Javadoc)
	 * @see android.widget.Adapter#getView(int, android.view.View, android.view.ViewGroup)
	 */
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		TextView view;
		if (convertView == null)
		{
			view = new TextView(context);
		}
		else
		{
			view = (TextView)convertView;
		}
		view.setText(alarms[position].getMessage());
		return view;
	}
}
