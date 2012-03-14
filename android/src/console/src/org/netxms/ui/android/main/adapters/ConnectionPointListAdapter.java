package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.List;

import org.netxms.ui.android.R;

import android.content.Context;
import android.content.res.Resources;
import android.view.View;
import android.view.ViewGroup;
import  android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Adapter for MAC address list search result
 *
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class ConnectionPointListAdapter extends BaseAdapter
{
	private Context context;
	private List<String> cpList = new ArrayList<String>(0);
	private Resources r;

	/**
	 * 
	 * @param context
	 */
	public ConnectionPointListAdapter(Context context)
	{
		this.context = context;
		r = context.getResources();
	}

	/**
	 * Set addresses
	 * 
	 * @param addresses list of addresses
	 */
	public void setConnectionPoint(List<String> list)
	{
		cpList = list;
	}

	@Override
	public int getCount()
	{
		return cpList.size();
	}

	@Override
	public Object getItem(int position)
	{
		return cpList.get(position);
	}

	@Override
	public long getItemId(int position)
	{
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		TextView message;
		ImageView searchResult;
		LinearLayout view, texts;

		if (convertView == null)
		{
			// new alarm, create fields
			searchResult = new ImageView(context);
			searchResult.setPadding(5, 5, 5, 2);

			message = new TextView(context);
			message.setPadding(5, 2, 5, 2);
			message.setTextColor(r.getColor(R.color.text_color));

			texts = new LinearLayout(context);
			texts.setOrientation(LinearLayout.VERTICAL);
			texts.addView(message);

			view = new LinearLayout(context);
			view.addView(searchResult);
			view.addView(texts);
		}
		else
		{ 
			// get reference to existing item
			view = (LinearLayout)convertView;
			searchResult = (ImageView)view.getChildAt(0);
			texts = (LinearLayout)view.getChildAt(1);
			message = (TextView)texts.getChildAt(0);
		}
		message.setText(cpList.get(position));
		//searchResult.setImageResource(true ? R.drawable.mac_search_ok : R.drawable.mac_search_ko);
		return view;
	}
}
