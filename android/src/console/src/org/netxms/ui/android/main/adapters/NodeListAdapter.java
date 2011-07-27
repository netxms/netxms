/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
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

public class NodeListAdapter extends BaseAdapter
{
	private Context context;
	private GenericObject[] currentNodes = new GenericObject[0];
	private ClientConnectorService service;

	private static final int[] imageId = { R.drawable.normal, R.drawable.warning, R.drawable.minor, R.drawable.major,
			R.drawable.critical };

	/**
	 * 
	 * @param context
	 */
	public NodeListAdapter(Context context)
	{
		this.context = context;
	}

	/**
	 * Set alarms
	 * 
	 * @param alarms
	 */
	public void setNodes(GenericObject[] nodes)
	{
		this.currentNodes = nodes;
	}

	public void setService(ClientConnectorService service)
	{
		this.service = service;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getCount()
	 */
	@Override
	public int getCount()
	{
		return currentNodes.length;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
		return currentNodes[position];
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItemId(int)
	 */
	@Override
	public long getItemId(int position)
	{
		return currentNodes[position].getObjectId();
	}

	public void unmanageObject(long id)
	{
		service.unmanageObject(id);
	}

	public void manageObject(long id)
	{
		service.manageObject(id);
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
		TextView nodeName;
		LinearLayout view, texts;
		ImageView severity, type;
		GenericObject node;

		if (convertView == null)
		{
			// new alarm, create fields
			nodeName = new TextView(context);
			nodeName.setPadding(5, 2, 5, 2);

			severity = new ImageView(context);
			severity.setPadding(5, 5, 5, 2);
			type = new ImageView(context);
			type.setPadding(5, 5, 5, 2);

			texts = new LinearLayout(context);
			texts.setOrientation(LinearLayout.VERTICAL);
			texts.addView(nodeName);

			view = new LinearLayout(context);
			view.addView(severity);
			view.addView(type);
			view.addView(texts);
		}
		else
		{ // get reference to existing alarm
			view = (LinearLayout)convertView;
			severity = (ImageView)view.getChildAt(0);
			type = (ImageView)view.getChildAt(1);
			texts = (LinearLayout)view.getChildAt(2);
			nodeName = (TextView)texts.getChildAt(0);
		}

		// get node name
		node = currentNodes[position];
		if (node == null)
		{
			nodeName.setText("<Unknown>");
		}
		else
		{
			nodeName.setText(node.getObjectName());
			if (node.getObjectClass() == GenericObject.OBJECT_CONTAINER)
			{
				type.setImageResource(R.drawable.container);
			}
			if (node.getObjectClass() == GenericObject.OBJECT_NODE)
			{
				type.setImageResource(R.drawable.node_small);
			}

		}

		severity.setImageResource(NodeListAdapter.imageId[node.getStatus()]);
		return view;
	}
}
