/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.main.views.ActivityListElement;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
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
	private static final int[] activityId = { HomeScreen.ACTIVITY_ALARMS, HomeScreen.ACTIVITY_DASHBOARDS,
			HomeScreen.ACTIVITY_NODES, HomeScreen.ACTIVITY_GRAPHS, HomeScreen.ACTIVITY_MACADDRESS };
	private static final int[] imageId = { R.drawable.alarms, R.drawable.dashboard,
			R.drawable.nodes, R.drawable.graphs, R.drawable.macaddress };
	private static final int[] textId = { R.string.home_screen_alarms, R.string.home_screen_dashboards,
			R.string.home_screen_nodes, R.string.home_screen_graphs, R.string.home_screen_macaddress };
	private static final int[] statusImageId = {
			R.drawable.status_normal,     // STATUS_NORMAL = 0;
			R.drawable.status_warning,    // STATUS_WARNING = 1;
			R.drawable.status_minor,      // STATUS_MINOR = 2;
			R.drawable.status_major,      // STATUS_MAJOR = 3;
			R.drawable.status_critical,   // STATUS_CRITICAL = 4;
			R.drawable.status_unknown,    // STATUS_UNKNOWN = 5;
			R.drawable.status_unmanaged,  // STATUS_UNMANAGED = 6;
			R.drawable.status_disabled,   // STATUS_DISABLED = 7;
			R.drawable.status_testing     // STATUS_TESTING = 8;
	};

	private GenericObject topNode = null;
	private final Context context;

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
	 * Set top node
	 * 
	 * @param topNode
	 */
	public void setTopNode(GenericObject topNode)
	{
		this.topNode = topNode;
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
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		ActivityListElement view;
		if (convertView == null)
		{
			view = new ActivityListElement(context, imageId[position], textId[position]);
			view.setLayoutParams(new GridView.LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
		}
		else
		{
			view = (ActivityListElement)convertView;
		}

		if (topNode != null && activityId[position] == HomeScreen.ACTIVITY_NODES)
		{
			Drawable[] layers;
			switch (topNode.getStatus())
			{
				case GenericObject.STATUS_WARNING:
				case GenericObject.STATUS_MINOR:
				case GenericObject.STATUS_MAJOR:
				case GenericObject.STATUS_CRITICAL:
					layers = new Drawable[2];
					layers[0] = parent.getResources().getDrawable(imageId[position]);
					layers[1] = parent.getResources().getDrawable(statusImageId[topNode.getStatus()]);
					break;
				case GenericObject.STATUS_NORMAL:
				case GenericObject.STATUS_UNKNOWN:
				case GenericObject.STATUS_UNMANAGED:
				case GenericObject.STATUS_DISABLED:
				case GenericObject.STATUS_TESTING:
				default:
					layers = new Drawable[1];
					layers[0] = parent.getResources().getDrawable(imageId[position]);
					break;
			}
			ImageView objectIcon = (ImageView)view.getChildAt(0);
			LayerDrawable drawable = new LayerDrawable(layers);
			if (layers.length > 1)
				drawable.setLayerInset(1, layers[0].getIntrinsicWidth() - layers[1].getIntrinsicWidth(),
						layers[0].getIntrinsicHeight() - layers[1].getIntrinsicHeight(), 0, 0);
			objectIcon.setImageDrawable(drawable);
		}
		return view;
	}
}
