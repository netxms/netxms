/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.netxms.client.datacollection.DciValue;
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

public class LastValuesAdapter extends BaseAdapter
{
	private Context context;
	private List<DciValue> currentValues = new ArrayList<DciValue>(0);

	/**
	 * 
	 * @param context
	 */
	public LastValuesAdapter(Context context)
	{
		this.context = context;
	}

	/**
	 * Set alarms
	 * 
	 * @param alarms
	 */
	public void setValues(DciValue[] values)
	{
		currentValues = Arrays.asList(values);
		Collections.sort(currentValues, new Comparator<DciValue>() {
			@Override
			public int compare(DciValue object1, DciValue object2)
			{
				return object1.getDescription().compareToIgnoreCase(object2.getDescription());
			}
		});
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getCount()
	 */
	@Override
	public int getCount()
	{
		return currentValues.size();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
		return currentValues.get(position);
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
		Resources r = context.getResources();

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
		DciValue item = currentValues.get(position);
		if (item == null)
		{
			itemName.setText(r.getString(R.string.node_unknown));
			itemValue.setText(r.getString(R.string.last_values_na));
		}
		else
		{
			itemName.setText(item.getDescription());
			itemValue.setText(item.getValue());
		}

		return view;
	}
}
