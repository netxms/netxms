package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.android.R;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

/**
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 * Expandable list adapter for browsing interfaces
 * 
 */
public class InterfacesAdapter extends BaseExpandableListAdapter
{
	private final Context context;
	private final ArrayList<String> groups;
	private final ArrayList<ArrayList<Interface>> children;
	private final Resources r;

	/**
	 * 
	 * @param context
	 */
	public InterfacesAdapter(Context context, ArrayList<String> groups, ArrayList<ArrayList<Interface>> children)
	{
		this.context = context;
		this.groups = new ArrayList<String>(0);
		this.children = new ArrayList<ArrayList<Interface>>(0);
		r = context.getResources();
	}

	/**
	 * Set graphs
	 * 
	 * @param graphs
	 */
	public void setValues(List<Interface> interfaces)
	{
		Collections.sort(interfaces, new Comparator<Interface>()
		{
			@Override
			public int compare(Interface object1, Interface object2)
			{
				return object1.getObjectName().compareTo(object2.getObjectName());
			}
		});
		groups.clear();
		children.clear();
		for (int i = 0; i < interfaces.size(); i++)
		{
			groups.add(interfaces.get(i).getObjectName());
			children.add(new ArrayList<Interface>());
			children.get(i).add(interfaces.get(i));
		}
	}

	@Override
	public Object getChild(int groupPosition, int childPosition)
	{
		if (groupPosition >= 0 && groupPosition < children.size())
			if (childPosition >= 0 && childPosition < children.get(groupPosition).size())
				return children.get(groupPosition).get(childPosition);
		return null;
	}

	@Override
	public long getChildId(int groupPosition, int childPosition)
	{
		return childPosition;
	}

	private ViewGroup getViewGroupChild(View convertView, ViewGroup parent)
	{
		// The parent will be our ListView from the ListActivity 
		if (convertView instanceof ViewGroup)
		{
			return (ViewGroup)convertView;
		}
		Context context = parent.getContext();
		LayoutInflater inflater = LayoutInflater.from(context);
		ViewGroup item = (ViewGroup)inflater.inflate(R.layout.interfaces_child, null);
		return item;
	}

	@Override
	public View getChildView(int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent)
	{
		ViewGroup item = getViewGroupChild(convertView, parent);
		ListView label = (ListView)item.findViewById(R.id.interfacesChild);
		InterfaceDetailsAdapter idAdapter = new InterfaceDetailsAdapter(parent.getContext());
		idAdapter.setValues((Interface)getChild(groupPosition, childPosition));
		label.setAdapter(idAdapter);

		//////// TODO: FIX ASAP, REMOVE HARDCODED VALUES!!!
		final int rowHeightDp = 24;
		final int rowCount = 13;
		// convert the dp values to pixels 
		final float ROW_HEIGHT = context.getResources().getDisplayMetrics().density * rowHeightDp;
		// set the height of the current grid 
		label.getLayoutParams().height = Math.round(rowCount * ROW_HEIGHT);
		//////// TODO: FIX ASAP, REMOVE HARDCODED VALUES!!!

		return item;
	}

	@Override
	public int getChildrenCount(int groupPosition)
	{
		if (groupPosition < children.size())
			return children.get(groupPosition).size();
		return 0;
	}

	@Override
	public Object getGroup(int groupPosition)
	{
		return groups.get(groupPosition);
	}

	@Override
	public int getGroupCount()
	{
		return groups.size();
	}

	@Override
	public long getGroupId(int groupPosition)
	{
		return groupPosition;
	}

	@Override
	public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent)
	{
		if (convertView == null)
		{
			LayoutInflater layoutInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			convertView = layoutInflater.inflate(R.layout.interfaces_group, null);
		}
		Interface i = (Interface)getChild(groupPosition, 0);
		ImageView iv = (ImageView)convertView.findViewById(R.id.interface_image);
		iv.setImageResource(getInterfaceStatusIcon(i.getStatus()));
		TextView tv = (TextView)convertView.findViewById(R.id.interface_text);
		tv.setText(i.getObjectName());
		return convertView;
	}

	@Override
	public boolean hasStableIds()
	{
		return true;
	}

	@Override
	public boolean isChildSelectable(int groupPosition, int childPosition)
	{
		return true;
	}

	private int getInterfaceStatusIcon(int status)
	{
		switch (status)
		{
			case GenericObject.STATUS_NORMAL:
				return R.drawable.status_normal;
			case GenericObject.STATUS_WARNING:
				return R.drawable.status_warning;
			case GenericObject.STATUS_MINOR:
				return R.drawable.status_minor;
			case GenericObject.STATUS_MAJOR:
				return R.drawable.status_major;
			case GenericObject.STATUS_CRITICAL:
				return R.drawable.status_critical;
			case GenericObject.STATUS_UNKNOWN:
				return R.drawable.status_unknown;
			case GenericObject.STATUS_UNMANAGED:
				return R.drawable.status_unmanaged;
			case GenericObject.STATUS_DISABLED:
				return R.drawable.status_disabled;
		}
		return R.drawable.status_unknown;
	}
}
