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

public class MacAddressListAdapter extends BaseAdapter
{
	public class MacAddressInfo
	{
		public MacAddressInfo() {};
		
		public void setValid(Boolean valid) { this.valid = valid; };
		public void setFound(Boolean found) { this.found = found; };
		public void setMacAddress(String address) { macAddress = address; };
		public void setNodeName(String name) { nodeName = name; };
		public void setBridgeName(String name) { bridgeName = name; };
		public void setInterfaceName(String name) { ifaceName = name; };

		public Boolean isValid() { return valid; };
		public Boolean isFound() { return found; };
		public String getMacAddress() { return macAddress; };
		public String getNodeName() { return nodeName; };
		public String getBridgeName() { return bridgeName; };
		public String getInterfaceName() { return ifaceName; };
		
		private Boolean valid = false;
		private Boolean found = false;
		private String macAddress = "";
		private String nodeName = "";
		private String bridgeName = "";
		private String ifaceName = "";
	}

	private Context context;
	private List<MacAddressInfo> searchList = new ArrayList<MacAddressInfo>(0);
	private Resources r;

	/**
	 * 
	 * @param context
	 */
	public MacAddressListAdapter(Context context)
	{
		this.context = context;
		r = context.getResources();
	}

	/**
	 * Set addresses
	 * 
	 * @param addresses list of addresses
	 */
	public void addMacAddressInfo(MacAddressInfo string)
	{
		if (string != null)
			searchList.add(0, string);
	}

	@Override
	public int getCount()
	{
		return searchList.size();
	}

	@Override
	public Object getItem(int position)
	{
		return searchList.get(position);
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
		String string;
		MacAddressInfo info = searchList.get(position);
		if (!info.isValid())
			string = r.getString(R.string.macaddress_invalid, info.getMacAddress());
		else if (!info.isFound())
			string = r.getString(R.string.macaddress_notfound, info.getMacAddress());
		else if (info.getNodeName().length() == 0)
			string = r.getString(R.string.macaddress_nonodeinfo, info.getMacAddress(), info.getBridgeName(), info.getInterfaceName());
		else
			string = r.getString(R.string.macaddress_nodeinfo, info.getNodeName(), info.getMacAddress(), info.getBridgeName(), info.getInterfaceName());
		message.setText(string);
		searchResult.setImageResource(info.isFound() ? R.drawable.mac_search_ok : R.drawable.mac_search_ko);
		return view;
	}
}
