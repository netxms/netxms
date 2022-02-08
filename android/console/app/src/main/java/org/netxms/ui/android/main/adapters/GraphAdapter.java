package org.netxms.ui.android.main.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.TextView;

import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.ui.android.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * <p>
 * Expandable list adapter for browsing predefined graphics
 */
public class GraphAdapter extends BaseExpandableListAdapter {
    private final Context context;
    private final ArrayList<String> groups;
    private final ArrayList<ArrayList<GraphDefinition>> children;

    /**
     * @param context
     * @param groups
     * @param children
     */
    public GraphAdapter(Context context, ArrayList<String> groups, ArrayList<ArrayList<GraphDefinition>> children) {
        this.context = context;
        this.groups = groups;
        this.children = children;
    }

    /**
     * Set graphs
     *
     * @param graphs
     */
    public void setGraphs(List<GraphDefinition> graphs) {
        Collections.sort(graphs, new Comparator<GraphDefinition>() {
            @Override
            public int compare(GraphDefinition object1, GraphDefinition object2) {
                return object1.getName().compareTo(object2.getName());
            }
        });

        groups.clear();
        children.clear();
        groups.add(context.getString(R.string.graph_uncategorized)); // To have it on top
        children.add(new ArrayList<GraphDefinition>());
        for (int i = 0; i < graphs.size(); i++) {
            String title = "";
            String[] splitStr = graphs.get(i).getName().split("->");
            title = splitStr.length == 1 ? context.getString(R.string.graph_uncategorized) : splitStr[0];
            if (!groups.contains(title))
                groups.add(title);
            int index = groups.indexOf(title);
            if (children.size() < index + 1)
                children.add(new ArrayList<GraphDefinition>());
            children.get(index).add(graphs.get(i));
        }
        if (children.get(0).size() == 0) // If <Uncategorized> is empty, remove it
        {
            groups.remove(0);
            children.remove(0);
        }
    }

    @Override
    public Object getChild(int groupPosition, int childPosition) {
        if (groupPosition >= 0 && groupPosition < children.size())
            if (childPosition >= 0 && childPosition < children.get(groupPosition).size())
                return children.get(groupPosition).get(childPosition);
        return null;
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return childPosition;
    }

    @Override
    public View getChildView(int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent) {
        String name = TrimGroup(((GraphDefinition) getChild(groupPosition, childPosition)).getName());
        if (convertView == null) {
            LayoutInflater layoutInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = layoutInflater.inflate(R.layout.graph_view_child_layout, null);
        }
        TextView tv = convertView.findViewById(R.id.tvChild);
        tv.setTextColor(context.getResources().getColor(R.color.text_color));
        tv.setText(name);
        return convertView;
    }

    @Override
    public int getChildrenCount(int groupPosition) {
        if (groupPosition < children.size())
            return children.get(groupPosition).size();
        return 0;
    }

    @Override
    public Object getGroup(int groupPosition) {
        return groups.get(groupPosition);
    }

    @Override
    public int getGroupCount() {
        return groups.size();
    }

    @Override
    public long getGroupId(int groupPosition) {
        return groupPosition;
    }

    @Override
    public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent) {
        String name = (String) getGroup(groupPosition);
        if (convertView == null) {
            LayoutInflater layoutInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = layoutInflater.inflate(R.layout.graph_view_group_layout, null);
        }
        TextView tv = convertView.findViewById(R.id.tvGroup);
        tv.setTextColor(context.getResources().getColor(R.color.text_color));
        tv.setText(name);
        return convertView;
    }

    @Override
    public boolean hasStableIds() {
        return true;
    }

    @Override
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return true;
    }

    /**
     * Trim group name from name string. Group name is considered starting before
     * "->" marker
     *
     * @param name
     */
    public String TrimGroup(String name) {
        String[] splitStr = name.split("->");
        if (splitStr.length == 1)
            return name;
        return splitStr[1];
    }
}
