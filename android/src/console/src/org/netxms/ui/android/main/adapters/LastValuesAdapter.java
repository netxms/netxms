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
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.views.CheckableLinearLayout;

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
 */

public class LastValuesAdapter extends BaseAdapter
{
	private Context context;
	private List<DciValue> currentValues = new ArrayList<DciValue>(0);

	private static final int[] severityImageId = { R.drawable.status_normal, R.drawable.status_warning, R.drawable.status_minor, 
		R.drawable.status_major, R.drawable.status_critical };
	private static final int[] stateImageId = { R.drawable.state_active, R.drawable.state_disabled, R.drawable.state_unsupported};

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
		TextView threshold, date, name, value;
		CheckableLinearLayout view;
		LinearLayout texts, info, info2, icons;
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
			
			threshold = new TextView(context);
			threshold.setPadding(5, 2, 5, 2);
			threshold.setTextColor(r.getColor(R.color.text_color));
			date = new TextView(context);
			date.setPadding(5, 2, 5, 2);
			date.setTextColor(r.getColor(R.color.text_color));
			date.setGravity(Gravity.RIGHT);
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.RIGHT;
			date.setLayoutParams(lp);
			info = new LinearLayout(context);
			info.setOrientation(LinearLayout.HORIZONTAL);
			info.addView(threshold);
			info.addView(date);

			name = new TextView(context);
			name.setPadding(5, 2, 5, 2);
			name.setTextColor(r.getColor(R.color.text_color));
			value = new TextView(context);
			value.setPadding(5, 2, 5, 2);
			value.setTextColor(r.getColor(R.color.text_color));
			value.setGravity(Gravity.RIGHT);
			lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.RIGHT;
			value.setLayoutParams(lp);
			info2 = new LinearLayout(context);
			info2.setOrientation(LinearLayout.HORIZONTAL);
			info2.addView(name);
			info2.addView(value);
			
			texts = new LinearLayout(context);
			texts.setOrientation(LinearLayout.VERTICAL);
			lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			texts.setLayoutParams(lp);
			texts.addView(info);
			texts.addView(info2);

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
			threshold = (TextView)info.getChildAt(0);
			date = (TextView)info.getChildAt(1);
			info2 = (LinearLayout)texts.getChildAt(1);
			name = (TextView)info2.getChildAt(0);
			value = (TextView)info2.getChildAt(1);
		}
		
		// get node current name/value pair
		DciValue item = currentValues.get(position);
		if (item == null)
		{
			name.setText(r.getString(R.string.node_unknown));
			value.setText(r.getString(R.string.last_values_na));
		}
		else
		{
			Threshold t = item.getActiveThreshold();
			severity.setImageResource(getThresholdIcon(t));
			threshold.setText(getThresholdText(t));
			state.setImageResource(LastValuesAdapter.stateImageId[item.getStatus()]);
			date.setText(item.getTimestamp().toLocaleString());
			name.setText(item.getDescription());
			value.setText(item.getValue());
		}

		return view;
	}
	
	private int getThresholdIcon(Threshold t)
	{
		int s = t != null ? t.getCurrentSeverity() : 0;
		return severityImageId[s < severityImageId.length ? s : 0];
	}
	
	private String getThresholdText(Threshold t)
	{
		final int[] fns = { R.string.ts_fn_last, R.string.ts_fn_average, R.string.ts_fn_deviation, 
				R.string.ts_fn_diff, R.string.ts_fn_error, R.string.ts_fn_sum };
		final int[] ops = { R.string.ts_op_less, R.string.ts_op_lessequal, R.string.ts_op_equal, 
				R.string.ts_op_greatherequal, R.string.ts_op_greather, R.string.ts_op_different, 
				R.string.ts_op_like, R.string.ts_op_unlike };

		if (t != null)
		{
			Resources r = context.getResources();
			int f = t.getFunction();
			StringBuilder text = new StringBuilder(r.getString(fns[f]));
			text.append("(");
			if (f != Threshold.F_DIFF)
				text.append(t.getArg1());
			text.append(") ");
			text.append(r.getString(ops[t.getOperation()]));
			text.append(' ');
			text.append(t.getValue());
			return text.toString();
		}		
		return "OK";
	}
}
