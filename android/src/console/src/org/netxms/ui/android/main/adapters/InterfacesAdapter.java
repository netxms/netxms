package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.Interface;
import org.netxms.ui.android.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
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

	/**
	 * 
	 * @param context
	 */
	public InterfacesAdapter(Context context, ArrayList<String> groups, ArrayList<ArrayList<Interface>> children)
	{
		this.context = context;
		this.groups = new ArrayList<String>(0);
		this.children = new ArrayList<ArrayList<Interface>>(0);
	}

	/**
	 * Set graphs
	 * 
	 * @param graphs
	 */
	public void setValues(List<Interface> interfaces)
	{
		groups.clear();
		children.clear();
		if (interfaces != null)
		{
			Collections.sort(interfaces, new Comparator<Interface>()
			{
				@Override
				public int compare(Interface object1, Interface object2)
				{
					return object1.getObjectName().compareTo(object2.getObjectName());
				}
			});
			for (int i = 0; i < interfaces.size(); i++)
			{
				groups.add(interfaces.get(i).getObjectName());
				children.add(new ArrayList<Interface>());
				children.get(i).add(interfaces.get(i));
			}
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

	@SuppressLint("InflateParams")
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
		TextView tv = (TextView)parent.findViewById(R.id.interface_text);
		int totalHeight = tv != null ? tv.getMeasuredHeight() : 0;
		for (int i = 0; i < idAdapter.getCount(); i++)
		{
			View listItem = idAdapter.getView(i, null, label);
			listItem.measure(0, 0);
			totalHeight += listItem.getMeasuredHeight();
		}
		ViewGroup.LayoutParams params = label.getLayoutParams();
		params.height = totalHeight + (label.getDividerHeight() * (idAdapter.getCount() - 1));
		label.setLayoutParams(params);
		label.requestLayout();
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

	@SuppressWarnings("deprecation")
	@SuppressLint("InflateParams")
	@Override
	public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent)
	{
		if (convertView == null)
		{
			LayoutInflater layoutInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			convertView = layoutInflater.inflate(R.layout.interfaces_group, null);
		}
		Interface i = (Interface)getChild(groupPosition, 0);
		TextView tv = (TextView)convertView.findViewById(R.id.interface_text);
		tv.setText(" " + i.getObjectName());
		tv.setCompoundDrawablesWithIntrinsicBounds(parent.getResources().getDrawable(getInterfaceStatusIcon(i.getStatus())), null, null, null);

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

	private int getInterfaceStatusIcon(ObjectStatus status)
	{
		switch (status)
		{
			case NORMAL:
				return R.drawable.status_normal;
			case WARNING:
				return R.drawable.status_warning;
			case MINOR:
				return R.drawable.status_minor;
			case MAJOR:
				return R.drawable.status_major;
			case CRITICAL:
				return R.drawable.status_critical;
			case UNKNOWN:
				return R.drawable.status_unknown;
			case UNMANAGED:
				return R.drawable.status_unmanaged;
			case DISABLED:
				return R.drawable.status_disabled;
			case TESTING:
				return R.drawable.status_unknown;	// TODO: 2014Oct10 Implement corresponding settings section
		}
		return R.drawable.status_unknown;
	}
}
