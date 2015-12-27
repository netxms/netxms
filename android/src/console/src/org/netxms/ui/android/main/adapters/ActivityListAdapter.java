/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;

import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.main.views.ActivityListElement;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout.LayoutParams;

/**
 * Adapter for list of activities displayed on home screen
 * 
 */
public class ActivityListAdapter extends BaseAdapter
{
	private static final int[] activityId = { HomeScreen.ACTIVITY_ALARMS, HomeScreen.ACTIVITY_DASHBOARDS, HomeScreen.ACTIVITY_NODES, HomeScreen.ACTIVITY_ENTIRENETWORK, HomeScreen.ACTIVITY_GRAPHS, HomeScreen.ACTIVITY_MACADDRESS };
//		HomeScreen.ACTIVITY_TEST };
	private static final int[] imageId = { R.drawable.alarms, R.drawable.dashboard, R.drawable.nodes, R.drawable.entire_network, R.drawable.graphs, R.drawable.macaddress, R.drawable.icon };
	private static final int[] textId = { R.string.home_screen_alarms, R.string.home_screen_dashboards, R.string.home_screen_nodes, R.string.home_screen_entire_network, R.string.home_screen_graphs, R.string.home_screen_macaddress, R.string.home };
	private static final int[] statusImageId = { R.drawable.status_normal, // STATUS_NORMAL = 0;
	R.drawable.status_warning, // STATUS_WARNING = 1;
	R.drawable.status_minor, // STATUS_MINOR = 2;
	R.drawable.status_major, // STATUS_MAJOR = 3;
	R.drawable.status_critical, // STATUS_CRITICAL = 4;
	R.drawable.status_unknown, // STATUS_UNKNOWN = 5;
	R.drawable.status_unmanaged, // STATUS_UNMANAGED = 6;
	R.drawable.status_disabled, // STATUS_DISABLED = 7;
	R.drawable.status_testing// STATUS_TESTING = 8;
	};
	private static final int[] alarmId = { R.drawable.alarm_pg, R.drawable.alarm_p1, R.drawable.alarm_p2, R.drawable.alarm_p3, R.drawable.alarm_p4, R.drawable.alarm_p5, R.drawable.alarm_p6, R.drawable.alarm_p7, R.drawable.alarm_p8, R.drawable.alarm_p9 };
	private static final int MAX_ALARMS = alarmId.length;
	private SparseArray<AbstractObject> topNodes = null;
	private final Context context;
	private int numAlarms = 0;

	/**
	 * Create activity list adapter.
	 * 
	 * @param context
	 */
	public ActivityListAdapter(Context context)
	{
		this.context = context;
	}

	/**
	 * Set top nodes list
	 * 
	 * @param objList
	 */
	public void setTopNodes(ArrayList<AbstractObject> objList)
	{
		topNodes = new SparseArray<AbstractObject>();
		for (int i = 0; i < objList.size(); i++)
		{
			if (objList.get(i) != null)
				switch (objList.get(i).getObjectClass())
				{
					case AbstractObject.OBJECT_NETWORK:
						topNodes.put(HomeScreen.ACTIVITY_ENTIRENETWORK, objList.get(i));
						break;
					case AbstractObject.OBJECT_DASHBOARDROOT:
						topNodes.put(HomeScreen.ACTIVITY_DASHBOARDS, objList.get(i));
						break;
					case AbstractObject.OBJECT_SERVICEROOT:
						topNodes.put(HomeScreen.ACTIVITY_NODES, objList.get(i));
						break;
				}
		}
	}

	public void setPendingAlarms(int num)
	{
		numAlarms = num;
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
		if (position >= 0 && position < getCount())
			return activityId[position];
		return 0;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getView(int, android.view.View,
	 * android.view.ViewGroup)
	 */
	@SuppressWarnings("deprecation")
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		ActivityListElement view = null;
		if (position >= 0 && position < getCount())
		{
			if (convertView == null)
			{
				view = new ActivityListElement(context, imageId[position], textId[position]);
				view.setLayoutParams(new GridView.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
			}
			else
			{
				view = (ActivityListElement)convertView;
			}

			Drawable infoLayer = null;
			switch (activityId[position])
			{
				case HomeScreen.ACTIVITY_ALARMS:
					if (numAlarms > 0)
						infoLayer = parent.getResources().getDrawable(numAlarms >= MAX_ALARMS ? alarmId[0] : alarmId[numAlarms]);
					break;
				case HomeScreen.ACTIVITY_NODES:
				case HomeScreen.ACTIVITY_ENTIRENETWORK:
				case HomeScreen.ACTIVITY_DASHBOARDS:
					if (topNodes != null)
					{
						AbstractObject obj = topNodes.get(activityId[position]);
						if (obj != null)
						{
							switch (obj.getStatus())
							{
								case WARNING:
								case MINOR:
								case MAJOR:
								case CRITICAL:
									infoLayer = parent.getResources().getDrawable(statusImageId[obj.getStatus().getValue()]);
									break;
								case NORMAL:
								case UNKNOWN:
								case UNMANAGED:
								case DISABLED:
								case TESTING:
								default:
									break;
							}
						}
					}
					break;
			}
			Drawable[] layers;
			if (infoLayer != null)
			{
				layers = new Drawable[2];
				layers[1] = infoLayer;
			}
			else
				layers = new Drawable[1];
			layers[0] = parent.getResources().getDrawable(imageId[position]);
			ImageView objectIcon = (ImageView)view.getChildAt(0);
			LayerDrawable drawable = new LayerDrawable(layers);
			if (layers.length > 1)
				drawable.setLayerInset(1, layers[0].getIntrinsicWidth() - layers[1].getIntrinsicWidth(), layers[0].getIntrinsicHeight() - layers[1].getIntrinsicHeight(), 0, 0);
			objectIcon.setImageDrawable(drawable);
		}
		return view;
	}
}
