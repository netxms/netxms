/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Adapter for alarm list
 *
 */

public class AlarmListAdapter extends BaseAdapter
{
	private Context context;
	private Alarm[] alarms = new Alarm[0];
	private ClientConnectorService service;

	private static final int[] imageId = { R.drawable.normal, R.drawable.warning, R.drawable.minor, R.drawable.major, R.drawable.critical }; 
	
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
	}
	
	public void setService(ClientConnectorService service)
	{
		this.service = service;
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
	
	public void terminateItem(long id){
		service.teminateAlarm(id);
	}

	/* (non-Javadoc)
	 * @see android.widget.Adapter#getView(int, android.view.View, android.view.ViewGroup)
	 */
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		TextView message,source;
		LinearLayout view; 
		ImageView severity;
		Alarm al;
		GenericObject obj;
		
		if (convertView == null)
		{	// new alarm, create fields
			message = new TextView(context); 
			message.setPadding(5, 2, 5, 2);
			
			severity = new ImageView(context); 
			severity.setPadding(5, 5, 5, 2);

			source = new TextView(context);
			source.setPadding(5, 2, 5, 2);
			
			view = new LinearLayout(context);
			view.addView(severity);
			view.addView(source);
			view.addView(message);
		}
		else
		{	// get reference to existing alarm
			view = (LinearLayout)convertView;
			severity = (ImageView) view.getChildAt(0);
			source = (TextView) view.getChildAt(1);
			message = (TextView) view.getChildAt(2);
		}

		// get node name
		al = alarms[position];
		obj = service.findObjectById(al.getSourceObjectId());
		if (obj == null) {
			source.setText("<Unknown>");
		}
		else {
			source.setText(obj.getObjectName());			
		}

		message.setText(al.getMessage());
		severity.setImageResource(AlarmListAdapter.imageId[al.getCurrentSeverity()]);
		
		return view;
	}

}
