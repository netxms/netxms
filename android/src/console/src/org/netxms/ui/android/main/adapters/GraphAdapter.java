package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.android.R;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.TextView;
import android.content.Context;

/**
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 *         Expandable list adapter for browsing predefined graphics
 * 
 */
public class GraphAdapter extends BaseExpandableListAdapter
{
	private Context context;
	private List<GraphSettings> graphList = new ArrayList<GraphSettings>(0);
	private ArrayList<String> groups;
	private ArrayList<ArrayList<GraphSettings>> children;

	/**
	 * 
	 * @param context
	 * @param groups
	 * @param children
	 */
	public GraphAdapter(Context context, ArrayList<String> groups, ArrayList<ArrayList<GraphSettings>> children)
	{
		this.context = context;
		this.groups = groups;
		this.children = children;
	}

	/**
	 * Set graphs
	 * 
	 * @param graphs
	 */
	public void setGraphs(List<GraphSettings> graphs)
	{
		graphList = graphs;
		groups.clear();
		children.clear();
		groups.add(context.getString(R.string.graph_uncategorized)); // To have it
																							// on top
		children.add(new ArrayList<GraphSettings>());
		for(int i = 0; i < graphList.size(); i++)
		{
			String title = "";
			String[] splitStr = graphList.get(i).getName().split("->");
			title = splitStr.length == 1 ? context.getString(R.string.graph_uncategorized) : splitStr[0];
			if (!groups.contains(title))
				groups.add(title);
			int index = groups.indexOf(title);
			if (children.size() < index + 1)
				children.add(new ArrayList<GraphSettings>());
			children.get(index).add(graphList.get(i));
		}
		if (children.get(0).size() == 0) // If <Uncategorized> is empty, remove it
		{
			groups.remove(0);
			children.remove(0);
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

	@Override
	public View getChildView(int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent)
	{
		String name = TrimGroup(((GraphSettings)getChild(groupPosition, childPosition)).getName());
		if (convertView == null)
		{
			LayoutInflater layoutInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			convertView = layoutInflater.inflate(R.layout.graph_view_child_layout, null);
		}
		TextView tv = (TextView)convertView.findViewById(R.id.tvChild);
		tv.setTextColor(context.getResources().getColor(R.color.text_color));
		tv.setText(name);
		return convertView;
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
		String name = (String)getGroup(groupPosition);
		if (convertView == null)
		{
			LayoutInflater layoutInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			convertView = layoutInflater.inflate(R.layout.graph_view_group_layout, null);
		}
		TextView tv = (TextView)convertView.findViewById(R.id.tvGroup);
		tv.setTextColor(context.getResources().getColor(R.color.text_color));
		tv.setText(name);
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

	/**
	 * Trim group name from name string. Group name is considered starting before
	 * "->" marker
	 * 
	 * @param name
	 */
	public String TrimGroup(String name)
	{
		String[] splitStr = name.split("->");
		if (splitStr.length == 1)
			return name;
		return splitStr[1];
	}
}
