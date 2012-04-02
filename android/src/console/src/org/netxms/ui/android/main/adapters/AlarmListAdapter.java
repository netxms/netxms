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
import android.content.res.Resources;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Adapter for alarm list
 *
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class AlarmListAdapter extends BaseAdapter
{
	public enum AlarmSortBy { SORT_SEVERITY_ASC, SORT_SEVERITY_DESC, SORT_DATE_ASC, SORT_DATE_DESC };

	private Context context;
	private List<Alarm> alarms = new ArrayList<Alarm>(0);
	private ClientConnectorService service;
	private AlarmSortBy sortBy = AlarmSortBy.SORT_SEVERITY_DESC;

	private static final int[] severityImageId = { R.drawable.status_normal, R.drawable.status_warning, R.drawable.status_minor, 
        R.drawable.status_major, R.drawable.status_critical };
	private static final int[] stateImageId = { R.drawable.alarm_outstanding, R.drawable.alarm_acknowledged, R.drawable.alarm_terminated };

	/**
	 * 
	 * @param context
	 */
	public AlarmListAdapter(Context context)
	{
		this.context = context;
	}

	/**
	 * 
	 * @param sortBy
	 */
	public void setSortBy(AlarmSortBy sortBy)
	{
		this.sortBy = sortBy;
	}

	/**
	 * 
	 * @return
	 */
	public AlarmSortBy getSortBy()
	{
		return sortBy;
	}

	/**
	 * Set alarms
	 * 
	 * @param alarms List of alarms
	 * @param nodeIdList List of nodes to be used to filter alarms
	 */
	public void setAlarms(Alarm[] alarms, ArrayList<Integer> nodeIdList)
	{
		if (nodeIdList == null || nodeIdList.size() == 0)
			this.alarms = Arrays.asList(alarms);
		else	// Filter on specific node
		{
			this.alarms.clear();
			for (int i = 0; i < alarms.length; i++)
				for (int j = 0; j < nodeIdList.size(); j++)
					if (alarms[i].getSourceObjectId() == nodeIdList.get(j))
						this.alarms.add(alarms[i]);
		}
		Collections.sort(this.alarms, new Comparator<Alarm>() {
			@Override
			public int compare(Alarm object1, Alarm object2)
			{
				switch (sortBy)
				{
					case SORT_SEVERITY_ASC:
						return compareSeverity(object1, object2);
					case SORT_SEVERITY_DESC:
						return compareSeverity(object2, object1);
					case SORT_DATE_ASC:
						return compareDate(object1, object2);
					case SORT_DATE_DESC:
						return compareDate(object2, object1);
				}
				return 1;
			}
		});
	}

	private int compareSeverity(Alarm object1, Alarm object2)
	{
		int rc = object1.getCurrentSeverity() - object2.getCurrentSeverity();
		if (rc == 0)
		{
			rc = (int)(object1.getSourceObjectId() - object2.getSourceObjectId());
			if (rc == 0)
			{
				rc = compareDate(object1, object2);
				if (rc == 0)
					rc = object1.getState() - object2.getState();
			}
		}
		return rc;
	}
	
	private int compareDate(Alarm object1, Alarm object2)
	{
		return object1.getLastChangeTime().compareTo(object2.getLastChangeTime());
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
		TextView message, source, date;
		LinearLayout view, texts, info, icons;
		ImageView severity, state;
		Resources r = context.getResources();

		if (convertView == null)	// new alarm, create fields
		{
			severity = new ImageView(context);
			severity.setPadding(5, 5, 5, 2);
			state = new ImageView(context);
			state.setPadding(5, 5, 5, 2);
			icons = new LinearLayout(context);
			icons.setOrientation(LinearLayout.VERTICAL);
			icons.addView(severity);
			icons.addView(state);
			
			source = new TextView(context);
			source.setPadding(5, 2, 5, 2);
			source.setTextColor(r.getColor(R.color.text_color));
			date = new TextView(context);
			date.setPadding(5, 2, 5, 2);
			date.setTextColor(r.getColor(R.color.text_color));
			date.setGravity(Gravity.RIGHT);
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.RIGHT;
			date.setLayoutParams(lp);
			message = new TextView(context);
			message.setPadding(5, 2, 5, 2);
			message.setTextColor(r.getColor(R.color.text_color));
			info = new LinearLayout(context);
			info.setOrientation(LinearLayout.HORIZONTAL);
			info.addView(source);
			info.addView(date);
			texts = new LinearLayout(context);
			texts.setOrientation(LinearLayout.VERTICAL);
			lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			texts.setLayoutParams(lp);
			texts.addView(info);
			texts.addView(message);

			view = new LinearLayout(context);
			view.addView(icons);
			view.addView(texts);
		}
		else
		{ 
			// get reference to existing alarm
			view = (LinearLayout)convertView;
			icons = (LinearLayout)view.getChildAt(0);
			severity = (ImageView)icons.getChildAt(0);
			state = (ImageView)icons.getChildAt(1);
			texts = (LinearLayout)view.getChildAt(1);
			info = (LinearLayout)texts.getChildAt(0);
			source = (TextView)info.getChildAt(0);
			date = (TextView)info.getChildAt(1);
			message = (TextView)texts.getChildAt(1);
		}

		// get node name
		Alarm alarm = alarms.get(position);
		GenericObject object = service.findObjectById(alarm.getSourceObjectId());
		source.setText(object == null ? r.getString(R.string.node_unknown) : object.getObjectName());
		date.setText(alarm.getLastChangeTime().toLocaleString());
		message.setText(alarm.getMessage());
		severity.setImageResource(AlarmListAdapter.severityImageId[alarm.getCurrentSeverity()]);
		state.setImageResource(AlarmListAdapter.stateImageId[alarm.getState()]);

		return view;
	}
}
