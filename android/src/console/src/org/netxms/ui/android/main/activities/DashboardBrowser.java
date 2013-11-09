/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.Stack;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ObjectListAdapter;

import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

/**
 * Dashboard browser
 */
public class DashboardBrowser extends AbstractClientActivity
{
	private static final String TAG = "nxclient/DashboardBrowser";
	private ListView listView;
	private ObjectListAdapter adapter;
	private final long initialParent = 7;
	private AbstractObject currentParent = null;
	private final Stack<AbstractObject> containerPath = new Stack<AbstractObject>();
	private long[] savedPath = null;
	private ProgressDialog dialog;

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		dialog = new ProgressDialog(this);
		setContentView(R.layout.node_view);

		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.dashboard_title);

		// keeps current list of nodes as datasource for listview
		adapter = new ObjectListAdapter(this);

		listView = (ListView)findViewById(R.id.NodeList);
		listView.setAdapter(adapter);
		listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
		{
			@Override
			@SuppressWarnings("rawtypes")
			public void onItemClick(AdapterView parent, View v, int position, long id)
			{
				AbstractObject obj = (AbstractObject)adapter.getItem(position);
				if (obj.getChildIdList().length > 0)
				{
					containerPath.push(currentParent);
					currentParent = obj;
					refreshList();
				}
				else if (obj.getObjectClass() == AbstractObject.OBJECT_DASHBOARD)
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

	/* (non-Javadoc)
	 * @see android.app.Activity#onSaveInstanceState(android.os.Bundle)
	 */
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
		Intent newIntent = new Intent(this, DashboardActivity.class);
		newIntent.putExtra("objectId", objectId);
		startActivity(newIntent);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);

		service.registerDashboardBrowser(this);

		// Restore to saved path if available
		if ((savedPath != null) && (savedPath.length > 0))
		{
			for (int i = 0; i < savedPath.length - 1; i++)
			{
				AbstractObject object = service.findObjectById(savedPath[i]);
				if (object == null)
					break;
				containerPath.push(object);
			}

			currentParent = service.findObjectById(savedPath[savedPath.length - 1]);
		}
		savedPath = null;

		refreshList();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View, android.view.ContextMenu.ContextMenuInfo)
	 */
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.node_actions, menu);

//		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
//		AbstractObject object = (AbstractObject)adapter.getItem(info.position);

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

	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		// get selected item
//		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
//		final GenericObject object = (GenericObject)adapter.getItem(info.position);

		// process menu selection
		switch (item.getItemId())
		{
			default:
		}

		return super.onContextItemSelected(item);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onBackPressed()
	 */
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
		for (AbstractObject o : containerPath)
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
		for (AbstractObject o : containerPath)
			path[i++] = o.getObjectId();

		if (currentParent != null)
			path[i++] = currentParent.getObjectId();

		return path;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerDashboardBrowser(null);
		super.onDestroy();
	}

	/**
	 * Update dashboard list, force refresh as necessary
	 */
	public void updateDashboardList()
	{
		if (adapter != null)
		{
			if (currentParent != null)
			{
				AbstractObject[] list = currentParent.getChildsAsArray();
				if (list != null)
				{
					adapter.setNodes(list);
					adapter.notifyDataSetChanged();
					return;
				}
			}
			refreshList();
		}
	}

	/**
	 * Internal task for synching missing objects
	 */
	private class SyncMissingObjectsTask extends AsyncTask<Object, Void, Exception>
	{
		private final long currentRoot;

		protected SyncMissingObjectsTask(long currentRoot)
		{
			this.currentRoot = currentRoot;
		}

		@Override
		protected void onPreExecute()
		{
			dialog.setMessage(getString(R.string.progress_gathering_data));
			dialog.setIndeterminate(true);
			dialog.setCancelable(false);
			dialog.show();
		}

		@Override
		protected Exception doInBackground(Object... params)
		{
			try
			{
				service.getSession().syncMissingObjects((long[])params[0], false, NXCSession.OBJECT_SYNC_WAIT);
			}
			catch (Exception e)
			{
				Log.e(TAG, "Exception while executing service.getSession().syncMissingObjects", e);
				return e;
			}
			return null;
		}

		@Override
		protected void onPostExecute(Exception result)
		{
			dialog.cancel();
			if ((result == null) && (DashboardBrowser.this.currentParent.getObjectId() == currentRoot))
			{
				adapter.setNodes(currentParent.getChildsAsArray());
				adapter.notifyDataSetChanged();
			}
		}
	}
}
