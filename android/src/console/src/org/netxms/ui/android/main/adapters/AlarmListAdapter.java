/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.views.CheckableLinearLayout;
import org.netxms.ui.android.service.ClientConnectorService;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.res.Resources;
import android.os.AsyncTask;
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
	public static final int SORT_SEVERITY_ASC = 0;
	public static final int SORT_SEVERITY_DESC = 1;
	public static final int SORT_DATE_ASC = 2;
	public static final int SORT_DATE_DESC = 3;
	public static final int SORT_NAME_ASC = 4;
	public static final int SORT_NAME_DESC = 5;
	public static final int ACKNOWLEDGE = 0;
	public static final int STICKY_ACKNOWLEDGE = 1;
	public static final int RESOLVE = 2;
	public static final int TERMINATE = 3;

	private final Context context;
	private final List<Alarm> alarms = new ArrayList<Alarm>(0);
	private ArrayList<Integer> nodeIdList = null;
	private ClientConnectorService service = null;
	private int sortBy = SORT_SEVERITY_DESC;
	private String NODE_UNKNOWN = "";
	private final ProgressDialog dialog;
	private final Resources r;

	/**
	 * 
	 * @param context
	 */
	public AlarmListAdapter(Context context)
	{
		this.context = context;
		r = context.getResources();
		NODE_UNKNOWN = r.getString(R.string.node_unknown);
		dialog = new ProgressDialog(context);
	}

	/**
	 * 
	 * @param sortBy
	 */
	public void setSortBy(int sortBy)
	{
		this.sortBy = sortBy;
	}

	/**
	 * 
	 * @return
	 */
	public int getSortBy()
	{
		return sortBy;
	}

	/**
	 * Set alarms filter
	 * 
	 * @param list List of nodes to be used to filter alarms
	 */
	public void setFilter(ArrayList<Integer> list)
	{
		nodeIdList = list;
	}

	/**
	 * Set values to the adapter
	 * 
	 * @param values List of alarms
	 */
	public void setValues(Alarm[] values)
	{
		alarms.clear();
		if (values != null)
		{
			if (nodeIdList == null || nodeIdList.size() == 0)
				alarms.addAll(Arrays.asList(values));
			else
			{ // Filter on specific node
				for (int i = 0; i < values.length; i++)
					for (int j = 0; j < nodeIdList.size(); j++)
						if (values[i].getSourceObjectId() == nodeIdList.get(j))
							alarms.add(values[i]);
			}
			sort();
		}
	}

	/**
	 * Sort the list of alarms
	 * 
	 * @param values List of alarms
	 */
	public void sort()
	{
		Collections.sort(alarms, new Comparator<Alarm>()
		{
			@Override
			public int compare(Alarm alarm1, Alarm alarm2)
			{
				switch (sortBy)
				{
					case SORT_SEVERITY_ASC:
						return compareSeverity(alarm1, alarm2);
					case SORT_SEVERITY_DESC:
						return compareSeverity(alarm2, alarm1);
					case SORT_DATE_ASC:
						return compareDate(alarm1, alarm2);
					case SORT_DATE_DESC:
						return compareDate(alarm2, alarm1);
					case SORT_NAME_ASC:
						return compareName(alarm1, alarm2);
					case SORT_NAME_DESC:
						return compareName(alarm2, alarm1);
				}
				return 1;
			}
		});
	}

	/**
	 * Compare by alarm severity
	 * 
	 * @param alarm1	First obj to compare
	 * @param alarm2	Second obj to compare
	 */
	private int compareSeverity(Alarm alarm1, Alarm alarm2)
	{
		int rc = 0;
		if (alarm1 != null && alarm2 != null)
		{
			rc = alarm1.getCurrentSeverity() - alarm2.getCurrentSeverity();
			if (rc == 0)
			{
				rc = (int)(alarm1.getSourceObjectId() - alarm2.getSourceObjectId());
				if (rc == 0)
				{
					rc = compareDate(alarm1, alarm2);
					if (rc == 0)
						rc = alarm1.getState() - alarm2.getState();
				}
			}
		}
		return rc;
	}

	/**
	 * Compare by alarm date
	 * 
	 * @param alarm1	First obj to compare
	 * @param alarm2	Second obj to compare
	 */
	private int compareDate(Alarm alarm1, Alarm alarm2)
	{
		int rc = 0;
		if (alarm1 != null && alarm2 != null)
			rc = alarm1.getLastChangeTime().compareTo(alarm2.getLastChangeTime());
		return rc;
	}

	/**
	 * Compare by alarm's node name
	 * 
	 * @param alarm1	First obj to compare
	 * @param alarm2	Second obj to compare
	 */
	private int compareName(Alarm alarm1, Alarm alarm2)
	{
		int rc = 0;
		if (alarm1 != null && alarm2 != null)
			rc = getObjectName(alarm1.getSourceObjectId()).compareTo(getObjectName(alarm2.getSourceObjectId()));
		return rc;
	}

	private String getObjectName(long objectId)
	{
		GenericObject object = null;
		if (service != null)
			object = service.findObjectById(objectId);
		return object == null ? NODE_UNKNOWN : object.getObjectName();
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
		if (position >= 0 && position < getCount())
			return alarms.get(position);
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItemId(int)
	 */
	@Override
	public long getItemId(int position)
	{
		if (position >= 0 && position < getCount())
			return alarms.get(position).getId();
		return 0;
	}

	/**
	 * @param action	Action to be executed
	 * @param ids	List of id to be processed
	 */

	@SuppressWarnings("unchecked")
	public void doAction(int action, ArrayList<Long> ids)
	{
		if (ids.size() > 0)
			new AlarmActionTask(action).execute(ids);
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

		if (convertView == null) // new alarm, create fields
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
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
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
			lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			texts.setLayoutParams(lp);
			texts.addView(info);
			texts.addView(message);

			view = new CheckableLinearLayout(context);
			view.addView(icons);
			view.addView(texts);
		}
		else
		{
			// get reference to existing alarm
			view = (CheckableLinearLayout)convertView;
			icons = (LinearLayout)view.getChildAt(1);
			severity = (ImageView)icons.getChildAt(0);
			state = (ImageView)icons.getChildAt(1);
			texts = (LinearLayout)view.getChildAt(2);
			info = (LinearLayout)texts.getChildAt(0);
			source = (TextView)info.getChildAt(0);
			date = (TextView)info.getChildAt(1);
			message = (TextView)texts.getChildAt(1);
		}

		// get node name
		Alarm alarm = alarms.get(position);
		source.setText(getObjectName(alarm.getSourceObjectId()));
		date.setText(DateFormat.getDateTimeInstance().format(alarm.getLastChangeTime()));
		message.setText(alarm.getMessage());
		severity.setImageResource(getAlarmIconSeverity(alarm));
		state.setImageResource(getAlarmIconState(alarm));

		return view;
	}

	private int getAlarmIconSeverity(Alarm alarm)
	{
		final int[] severityImageId = { R.drawable.status_normal, R.drawable.status_warning,
				R.drawable.status_minor, R.drawable.status_major, R.drawable.status_critical };
		return severityImageId[alarm.getCurrentSeverity()];

	}
	private int getAlarmIconState(Alarm alarm)
	{
		final int[] stateImageId = { R.drawable.alarm_outstanding, R.drawable.alarm_acknowledged,
				R.drawable.alarm_resolved, R.drawable.alarm_terminated };
		return alarm.isSticky() ? R.drawable.alarm_sticky_acknowledged : stateImageId[alarm.getState()];
	}

	/**
	 * Internal task for taking action on list of alarms selected
	 */
	private class AlarmActionTask extends AsyncTask<ArrayList<Long>, Void, Void>
	{
		private final int action;

		protected AlarmActionTask(int action)
		{
			this.action = action;
		}

		@Override
		protected void onPreExecute()
		{
			dialog.setMessage(r.getString(R.string.progress_processing_data));
			dialog.setIndeterminate(true);
			dialog.setCancelable(false);
			dialog.show();
		}

		@Override
		protected Void doInBackground(ArrayList<Long>... params)
		{
			if (service != null)
				switch (action)
				{
					case ACKNOWLEDGE:
						service.acknowledgeAlarm(params[0], false);
						break;
					case STICKY_ACKNOWLEDGE:
						service.acknowledgeAlarm(params[0], true);
						break;
					case RESOLVE:
						service.resolveAlarm(params[0]);
						break;
					case TERMINATE:
						service.terminateAlarm(params[0]);
						break;
				}
			return null;
		}

		@Override
		protected void onPostExecute(Void result)
		{
			if (service != null)
			{
				setValues(service.getAlarms());
				notifyDataSetChanged();
			}
			dialog.cancel();
		}
	}
}
