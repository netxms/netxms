/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.Stack;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.NodeListAdapter;
import android.content.ComponentName;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;
import android.widget.TextView;

/**
 * Node browser
 * 
 */
public class DashboardBrowser extends AbstractClientActivity
{
	private ListView listView;
	private NodeListAdapter adapter;
	private long initialParent = 7;
	private GenericObject currentParent = null;
	private Stack<GenericObject> containerPath = new Stack<GenericObject>();
	private long[] savedPath = null;

	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.node_view);

		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.dashboard_title);

		// keeps current list of nodes as datasource for listview
		adapter = new NodeListAdapter(this);

		listView = (ListView)findViewById(R.id.NodeList);
		listView.setAdapter(adapter);
		listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
		{
			@SuppressWarnings("rawtypes")
			public void onItemClick(AdapterView parent, View v, int position, long id)
			{
				GenericObject obj = (GenericObject)adapter.getItem(position);
				if (obj.getChildIdList().length > 0)
				{
					containerPath.push(currentParent);
					currentParent = obj;
					refreshList();
				}
				else if (obj.getObjectClass() == GenericObject.OBJECT_DASHBOARD)
				{
					showDashboard(obj.getObjectId());
				}
			}
		});

		registerForContextMenu(listView);

		// Restore saved state
		if (savedInstanceState != null)
		{
			savedPath = savedInstanceState.getLongArray("currentPath");
		}
	}

	@Override
	protected void onSaveInstanceState(Bundle outState)
	{
		outState.putLongArray("currentPath", getFullPathAsId());
		super.onSaveInstanceState(outState);
	}

	/**
	 * @param objectId
	 */
	public void showDashboard(long objectId)
	{
		// Intent newIntent = new Intent(this, Dashboard.class);
		// newIntent.putExtra("objectId", objectId);
		// startActivity(newIntent);
	}

	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);

		adapter.setService(service);
		service.registerDashboardBrowser(this);

		// Restore to saved path if available
		if ((savedPath != null) && (savedPath.length > 0))
		{
			for(int i = 0; i < savedPath.length - 1; i++)
			{
				GenericObject object = service.findObjectById(savedPath[i]);
				if (object == null)
					break;
				containerPath.push(object);
			}

			currentParent = service.findObjectById(savedPath[savedPath.length - 1]);
		}
		savedPath = null;

		refreshList();
	}

	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
		adapter.setService(null);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.node_actions, menu);

		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
		GenericObject object = (GenericObject)adapter.getItem(info.position);

		/*
		 * if (object instanceof Node) { // add available tools to context menu
		 * List<ObjectTool> tools = service.getTools(); if (tools != null) {
		 * Iterator<ObjectTool> tl = tools.iterator(); ObjectTool tool;
		 * while(tl.hasNext()) { tool = tl.next(); if ((tool.getType() ==
		 * ObjectTool.TYPE_ACTION || tool.getType() ==
		 * ObjectTool.TYPE_SERVER_COMMAND) &&
		 * tool.isApplicableForNode((Node)object)) { menu.add(Menu.NONE,
		 * (int)tool.getId(), 0, tool.getDisplayName()); } } } } else
		 */
		menu.getItem(0).setVisible(false);

	}

	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		// get selected item
		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		final GenericObject object = (GenericObject)adapter.getItem(info.position);

		// process menu selection
		switch(item.getItemId())
		{
			default:
		}

		return super.onContextItemSelected(item);
	}

	@Override
	public void onBackPressed()
	{
		if (this.currentParent == null)
		{
			super.onBackPressed();
			return;
		}
		if (this.currentParent.getObjectId() == this.initialParent)
		{
			super.onBackPressed();
			return;
		}
		if (containerPath.empty())
		{
			super.onBackPressed();
			return;
		}

		this.currentParent = containerPath.pop();
		this.refreshList();
		return;
	}

	/**
	 * Refresh dashboards list
	 */
	public void refreshList()
	{
		if (currentParent == null)
		{
			currentParent = service.findObjectById(initialParent);
		}
		if (currentParent == null)
		{
			// if still null - problem with root node, stop loading
			return;
		}

		TextView curPath = (TextView)findViewById(R.id.ScreenTitleSecondary);
		curPath.setText(getFullPath());
		adapter.setNodes(currentParent.getChildsAsArray());
		adapter.notifyDataSetChanged();
		new SyncMissingObjectsTask(currentParent.getObjectId()).execute(new Object[] { currentParent.getChildIdList() });
	}

	/**
	 * Get full path to current position in object tree
	 * 
	 * @return
	 */
	private String getFullPath()
	{
		StringBuilder sb = new StringBuilder();
		for(GenericObject o : containerPath)
		{
			sb.append('/');
			sb.append(o.getObjectName());
		}
		if (currentParent != null)
		{
			sb.append('/');
			sb.append(currentParent.getObjectName());
		}
		return sb.toString();
	}

	/**
	 * Get full path to current position in object tree, as object identifiers
	 * 
	 * @return
	 */
	private long[] getFullPathAsId()
	{
		long[] path = new long[containerPath.size() + ((currentParent != null) ? 1 : 0)];
		int i = 0;
		for(GenericObject o : containerPath)
			path[i++] = o.getObjectId();

		if (currentParent != null)
			path[i++] = currentParent.getObjectId();

		return path;
	}

	@Override
	protected void onDestroy()
	{
		service.registerDashboardBrowser(null);
		unbindService(this);
		super.onDestroy();
	}

	/**
	 * Internal task for synching missing objects
	 */
	private class SyncMissingObjectsTask extends AsyncTask<Object, Void, Exception>
	{
		private long currentRoot;

		protected SyncMissingObjectsTask(long currentRoot)
		{
			this.currentRoot = currentRoot;
		}

		@Override
		protected Exception doInBackground(Object... params)
		{
			try
			{
				service.getSession().syncMissingObjects((long[])params[0], false, NXCSession.OBJECT_SYNC_WAIT);
			}
			catch(Exception e)
			{
				Log.d("nxclient/SyncMissingObjectsTask", "Exception while executing service.getSession().syncMissingObjects", e);
				return e;
			}
			return null;
		}

		@Override
		protected void onPostExecute(Exception result)
		{
			if ((result == null) && (DashboardBrowser.this.currentParent.getObjectId() == currentRoot))
			{
				adapter.setNodes(currentParent.getChildsAsArray());
				adapter.notifyDataSetChanged();
			}
		}
	}
}
