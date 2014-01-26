/**
 * 
 */
package org.netxms.ui.android.main.adapters;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.netxms.client.constants.Severity;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.R;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Adapter for object list
 */
public class ObjectListAdapter extends BaseAdapter
{
	private final Context context;
	private final List<AbstractObject> objectList = new ArrayList<AbstractObject>(0);

	private static final int[] statusImageId = {
			R.drawable.status_normal, // STATUS_NORMAL = 0;
			R.drawable.status_warning, // STATUS_WARNING = 1;
			R.drawable.status_minor, // STATUS_MINOR = 2;
			R.drawable.status_major, // STATUS_MAJOR = 3;
			R.drawable.status_critical, // STATUS_CRITICAL = 4;
			R.drawable.status_unknown, // STATUS_UNKNOWN = 5;
			R.drawable.status_unmanaged, // STATUS_UNMANAGED = 6;
			R.drawable.status_disabled, // STATUS_DISABLED = 7;
			R.drawable.status_testing // STATUS_TESTING = 8;
	};
	private static final int[] statusTextId = {
			R.string.status_normal, // STATUS_NORMAL = 0;
			R.string.status_warning, // STATUS_WARNING = 1;
			R.string.status_minor, // STATUS_MINOR = 2;
			R.string.status_major, // STATUS_MAJOR = 3;
			R.string.status_critical, // STATUS_CRITICAL = 4;
			R.string.status_unknown, // STATUS_UNKNOWN = 5;
			R.string.status_unmanaged, // STATUS_UNMANAGED = 6;
			R.string.status_disabled, // STATUS_DISABLED = 7;
			R.string.status_testing // STATUS_TESTING = 8;
	};

	/**
	 * 
	 * @param context
	 */
	public ObjectListAdapter(Context context)
	{
		this.context = context;
	}

	/**
	 * Set nodes
	 * 
	 * @param objecs
	 */
	public void setNodes(AbstractObject[] objects)
	{
		objectList.clear();
		if (objects != null)
		{
			objectList.addAll(Arrays.asList(objects));
			Collections.sort(objectList, new Comparator<AbstractObject>()
			{
				@Override
				public int compare(AbstractObject object1, AbstractObject object2)
				{
					int category1 = (object1.getObjectClass() == AbstractObject.OBJECT_CONTAINER) ? 0 : 1;
					int category2 = (object2.getObjectClass() == AbstractObject.OBJECT_CONTAINER) ? 0 : 1;
					if (category1 != category2)
						return category1 - category2;
					return object1.getObjectName().compareToIgnoreCase(object2.getObjectName());
				}
			});
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

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Adapter#getView(int, android.view.View,
	 * android.view.ViewGroup)
	 */
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		TextView objectName;
		TextView objectStatusText;
		LinearLayout view, texts;
		ImageView objectIcon;
		Resources r = context.getResources();

		if (convertView == null)
		{
			// new object, create fields
			objectName = new TextView(context);
			objectName.setPadding(5, 2, 5, 2);
			objectName.setTextColor(r.getColor(R.color.text_color));
			objectName.setTextSize(TypedValue.COMPLEX_UNIT_PX, objectName.getTextSize() * 1.1f);

			objectStatusText = new TextView(context);
			objectStatusText.setPadding(5, 2, 5, 2);
			objectStatusText.setTextColor(r.getColor(R.color.text_color));

			objectIcon = new ImageView(context);
			objectIcon.setPadding(5, 5, 5, 2);

			texts = new LinearLayout(context);
			texts.setOrientation(LinearLayout.VERTICAL);
			texts.addView(objectName);
			texts.addView(objectStatusText);

			view = new LinearLayout(context);
			view.addView(objectIcon);
			view.addView(texts);
		}
		else
		{
			// get reference to existing object
			view = (LinearLayout)convertView;
			objectIcon = (ImageView)view.getChildAt(0);
			texts = (LinearLayout)view.getChildAt(1);
			objectName = (TextView)texts.getChildAt(0);
			objectStatusText = (TextView)texts.getChildAt(1);
		}

		// set fields in view
		int objectStatus;
		int objectIconId = R.drawable.object_unknown;
		AbstractObject object = objectList.get(position);
		if (object == null)
		{
			objectName.setText(r.getString(R.string.node_unknown));
			objectStatusText.setText(r.getString(R.string.status_unknown));
			objectStatus = Severity.UNKNOWN;
		}
		else
		{
			objectName.setText(object.getObjectName());
			objectStatusText.setText(statusTextId[object.getStatus()]);
			objectStatus = object.getStatus();
			switch (object.getObjectClass())
			{
				case AbstractObject.OBJECT_CONTAINER:
					objectIconId = R.drawable.object_container;
					break;
				case AbstractObject.OBJECT_NODE:
					objectIconId = R.drawable.object_node;
					break;
				case AbstractObject.OBJECT_CLUSTER:
					objectIconId = R.drawable.object_cluster;
					break;
				case AbstractObject.OBJECT_DASHBOARD:
					objectIconId = R.drawable.dashboard;
					break;
				case AbstractObject.OBJECT_MOBILEDEVICE:
					objectIconId = R.drawable.object_mobiledevice;
					break;
				case AbstractObject.OBJECT_SUBNET:
					objectIconId = R.drawable.object_subnet;
					break;
				case AbstractObject.OBJECT_ZONE:
					objectIconId = R.drawable.object_zone;
					break;
			}
		}

		Drawable[] layers = new Drawable[2];
		layers[0] = parent.getResources().getDrawable(objectIconId);
		layers[1] = parent.getResources().getDrawable(ObjectListAdapter.statusImageId[objectStatus]);
		LayerDrawable drawable = new LayerDrawable(layers);
		drawable.setLayerInset(1, layers[0].getIntrinsicWidth() - layers[1].getIntrinsicWidth(),
				layers[0].getIntrinsicHeight() - layers[1].getIntrinsicHeight(), 0, 0);
		objectIcon.setImageDrawable(drawable);

		return view;
	}
}
