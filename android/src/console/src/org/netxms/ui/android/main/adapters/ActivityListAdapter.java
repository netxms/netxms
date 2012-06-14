/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import org.netxms.ui.android.R;
import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.main.views.ActivityListElement;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.LinearLayout.LayoutParams;

/**
 * Adapter for list of activities displayed on home screen
 * 
 */
public class ActivityListAdapter extends BaseAdapter
{
	private static final int[] activityId = { HomeScreen.ACTIVITY_ALARMS, HomeScreen.ACTIVITY_NODES, HomeScreen.ACTIVITY_GRAPHS, HomeScreen.ACTIVITY_MACADDRESS, HomeScreen.ACTIVITY_DASHBOARDS  };
	private static final int[] imageId = { R.drawable.alarms, R.drawable.nodes, R.drawable.graphs, R.drawable.macaddress, R.drawable.dashboard };
	private static final int[] textId = { R.string.home_screen_alarms, R.string.home_screen_nodes, R.string.home_screen_graphs,
			R.string.home_screen_macaddress, R.string.home_screen_dashboards };

	private Context context;

	/**
	 * Create activity list adapter.
	 * 
	 * @param context
	 */
	public ActivityListAdapter(Context context)
	{
		this.context = context;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getCount()
	 */
	@Override
	public int getCount()
	{
		return activityId.length;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
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
		return activityId[position];
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
		ActivityListElement view;
		if (convertView == null)
		{
			view = new ActivityListElement(context, imageId[position], textId[position]);
			view.setLayoutParams(new GridView.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		}
		else
		{
			view = (ActivityListElement)convertView;
		}
		return view;
	}
}
