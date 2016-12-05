/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;

import org.netxms.ui.android.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Adapter for overview text details inside a list adapter
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class OverviewDetailsAdapter extends BaseAdapter
{
	private final Context context;
	private final List<String> labels = new ArrayList<String>();
	private final List<String> values = new ArrayList<String>();
	private final Resources r;

	/**
	 * 
	 * @param context
	 */
	public OverviewDetailsAdapter(Context context)
	{
		this.context = context;
		r = context.getResources();
	}

	/**
	 * Set values
	 * 
	 * @param map	list of labels/values
	 */
	public void setValues(Map<String, String> map)
	{
		if (map != null)
		{
			ArrayList<String> keys = new ArrayList<String>();
			for (String key : map.keySet())
				keys.add(key);
			Collections.sort(keys, new Comparator<String>()
			{
				@Override
				public int compare(String object1, String object2)
				{
					return object1.compareTo(object2);
				}
			});
			for (String key : keys)
				addRow(key, map.get(key));
		}
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
	@SuppressLint("RtlHardcoded")
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
		return view;
	}

	private void addRow(String label, String value)
	{
		labels.add(label);
		values.add(value);
	}
}
