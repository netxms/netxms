/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
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
 */
public class NodeListAdapter extends BaseAdapter
{
	private Context context;
	private List<GenericObject> objectList = new ArrayList<GenericObject>(0);
	private ClientConnectorService service;

	private static final int[] imageId = { 
		R.drawable.normal, 		// STATUS_NORMAL         = 0;
		R.drawable.warning, 		//	STATUS_WARNING        = 1;
		R.drawable.minor, 		//	STATUS_MINOR          = 2;
		R.drawable.major,			//	STATUS_MAJOR          = 3;
		R.drawable.critical,		//	STATUS_CRITICAL       = 4;
		R.drawable.unknown, 		// STATUS_UNKNOWN        = 5;
		R.drawable.unmanaged,	//	STATUS_UNMANAGED      = 6;
		R.drawable.disabled,		//	STATUS_DISABLED       = 7;
		R.drawable.testing		//	STATUS_TESTING        = 8;
	};

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
	public void setNodes(GenericObject[] objects)
	{
		objectList = Arrays.asList(objects);
		Collections.sort(objectList, new Comparator<GenericObject>() {
			@Override
			public int compare(GenericObject object1, GenericObject object2)
			{
				int category1 = (object1.getObjectClass() == GenericObject.OBJECT_CONTAINER) ? 0 : 1;
				int category2 = (object2.getObjectClass() == GenericObject.OBJECT_CONTAINER) ? 0 : 1;
				if (category1 != category2)
					return category1 - category2;
				return object1.getObjectName().compareToIgnoreCase(object2.getObjectName());
			}
		});
	}

	/**
	 * @param service
	 */
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
		return objectList.size();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItem(int)
	 */
	@Override
	public Object getItem(int position)
	{
		return objectList.get(position);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getItemId(int)
	 */
	@Override
	public long getItemId(int position)
	{
		return objectList.get(position).getObjectId();
	}

	/**
	 * @param id
	 */
	public void unmanageObject(long id)
	{
		service.setObjectMgmtState(id, false);
	}

	/**
	 * @param id
	 */
	public void manageObject(long id)
	{
		service.setObjectMgmtState(id, true);
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
			// new object, create fields
			nodeName = new TextView(context);
			nodeName.setPadding(5, 2, 5, 2);
			nodeName.setTextColor(0xFF404040);

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
		{ 
			// get reference to existing object
			view = (LinearLayout)convertView;
			severity = (ImageView)view.getChildAt(0);
			type = (ImageView)view.getChildAt(1);
			texts = (LinearLayout)view.getChildAt(2);
			nodeName = (TextView)texts.getChildAt(0);
		}

		// get object name
		node = objectList.get(position);
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
