/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

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
	private List<Alarm> alarms = new ArrayList<Alarm>(0);
	private ClientConnectorService service;

	private static final int[] imageId = { R.drawable.status_normal, R.drawable.status_warning, R.drawable.status_minor, 
	                                       R.drawable.status_major, R.drawable.status_critical };

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
	 * 
	 * @param alarms
	 */
	public void setAlarms(Alarm[] alarms)
	{
		this.alarms = Arrays.asList(alarms);
		Collections.sort(this.alarms, new Comparator<Alarm>() {
			@Override
			public int compare(Alarm object1, Alarm object2)
			{
				int rc = object2.getCurrentSeverity() - object1.getCurrentSeverity();
				if (rc == 0)
				{
					rc = (int)(object1.getSourceObjectId() - object2.getSourceObjectId());
					if (rc == 0)
					{
						rc = object1.getState() - object2.getState();
					}
				}
				return rc;
			}
		});
	}

	/**
	 * @param service
	 */
	public void setService(ClientConnectorService service)
	{
		this.service = service;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getCount()
	 */
	@Override
	public int getCount()
	{
		return alarms.size();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
		return alarms.get(position);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItemId(int)
	 */
	@Override
	public long getItemId(int position)
	{
		return alarms.get(position).getId();
	}

	/**
	 * @param id
	 */
	public void acknowledgeItem(long id)
	{
		service.acknowledgeAlarm(id);
	}

	/**
	 * @param id
	 */
	public void terminateItem(long id)
	{
		service.teminateAlarm(id);
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
		TextView message, source;
		LinearLayout view, texts;
		ImageView severity;

		if (convertView == null)
		{
			// new alarm, create fields
			severity = new ImageView(context);
			severity.setPadding(5, 5, 5, 2);

			source = new TextView(context);
			source.setPadding(5, 2, 5, 2);
			source.setTextColor(0xFF404040);

			message = new TextView(context);
			message.setPadding(5, 2, 5, 2);
			message.setTextColor(0xFF404040);

			texts = new LinearLayout(context);
			texts.setOrientation(LinearLayout.VERTICAL);
			texts.addView(source);
			texts.addView(message);

			view = new LinearLayout(context);
			view.addView(severity);
			view.addView(texts);
		}
		else
		{ 
			// get reference to existing alarm
			view = (LinearLayout)convertView;
			severity = (ImageView)view.getChildAt(0);
			texts = (LinearLayout)view.getChildAt(1);
			source = (TextView)texts.getChildAt(0);
			message = (TextView)texts.getChildAt(1);
		}

		// get node name
		Alarm alarm = alarms.get(position);
		GenericObject object = service.findObjectById(alarm.getSourceObjectId());
		if (object == null)
		{
			source.setText("<Unknown>");
		}
		else
		{
			source.setText(object.getObjectName());
		}

		message.setText(alarm.getMessage());
		severity.setImageResource(AlarmListAdapter.imageId[alarm.getCurrentSeverity()]);

		return view;
	}
}
