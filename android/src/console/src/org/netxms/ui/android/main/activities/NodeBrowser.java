/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Stack;

import org.netxms.client.NXCSession;
import org.netxms.client.constants.NodePoller;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ObjectListAdapter;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;
import android.widget.TextView;

/**
 * Node browser
 */
public class NodeBrowser extends AbstractClientActivity
{
	private static final String TAG = "nxclient/SyncMissingObjectsTask";
	private ListView listView;
	private ObjectListAdapter adapter;
	private final long initialParent = 2;
	private GenericObject currentParent = null;
	private final Stack<GenericObject> containerPath = new Stack<GenericObject>();
	private long[] savedPath = null;
	private GenericObject selectedObject = null;

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.node_view);

		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.nodes_title);

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
				GenericObject obj = (GenericObject)adapter.getItem(position);
				if ((obj.getObjectClass() == GenericObject.OBJECT_CONTAINER) || (obj.getObjectClass() == GenericObject.OBJECT_CLUSTER))
				{
					containerPath.push(currentParent);
					currentParent = obj;
					refreshList();
				}
				else if (obj.getObjectClass() == GenericObject.OBJECT_NODE)
				{
					showNodeInfo(obj.getObjectId());
				}
			}
		});

		registerForContextMenu(listView);

		// Restore saved state
		if (savedInstanceState != null)
			savedPath = savedInstanceState.getLongArray("currentPath");
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
	public void showNodeInfo(long objectId)
	{
		Intent newIntent = new Intent(this, NodeInfo.class);
		newIntent.putExtra("objectId", objectId);
		startActivity(newIntent);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.
	 * ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);

		service.registerNodeBrowser(this);

		// Restore to saved path if available
		if ((savedPath != null) && (savedPath.length > 0))
		{
			for (int i = 0; i < savedPath.length - 1; i++)
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

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View, android.view.ContextMenu.ContextMenuInfo)
	 */
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.node_actions, menu);

		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
		selectedObject = (GenericObject)adapter.getItem(info.position);

		if (selectedObject instanceof Node)
		{
			// add available tools to context menu
			List<ObjectTool> tools = service.getTools();
			if (tools != null)
			{
				Iterator<ObjectTool> tl = tools.iterator();
				ObjectTool tool;
				while (tl.hasNext())
				{
					tool = tl.next();
					if ((tool.getType() == ObjectTool.TYPE_ACTION || tool.getType() == ObjectTool.TYPE_SERVER_COMMAND) &&
							tool.isApplicableForNode((Node)selectedObject))
					{
						menu.add(Menu.NONE, (int)tool.getId(), 0, tool.getDisplayName());
					}
				}
			}
		}
		else
		{
			hideMenuItem(menu, R.id.find_switch_port);
			hideMenuItem(menu, R.id.poll);
		}
	}

	/**
	 * @param menu
	 * @param id
	 */
	private void hideMenuItem(ContextMenu menu, int id)
	{
		MenuItem item = menu.findItem(id);
		if (item != null)
			item.setVisible(false);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (selectedObject == null)
			return super.onContextItemSelected(item);

		if (item.getItemId() == R.id.find_switch_port)
		{
			Intent fspIntent = new Intent(this, ConnectionPointBrowser.class);
			fspIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
			startActivity(fspIntent);
			return true;
		}
		else if (item.getItemId() == R.id.view_alarms)
		{
			new SyncMissingChildsTask().execute(new Integer[] { (int)selectedObject.getObjectId() });
			return true;
		}
		else if (item.getItemId() == R.id.unmanage)
		{
			service.setObjectMgmtState(selectedObject.getObjectId(), false);
			refreshList();
			return true;
		}
		else if (item.getItemId() == R.id.manage)
		{
			service.setObjectMgmtState(selectedObject.getObjectId(), true);
			refreshList();
			return true;
		}
		else if (item.getItemId() == R.id.poll_status)
		{
			Intent psIntent = new Intent(this, NodePollerActivity.class);
			psIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
			psIntent.putExtra("pollType", NodePoller.STATUS_POLL);
			startActivity(psIntent);
			return true;
		}
		else if (item.getItemId() == R.id.poll_configuration)
		{
			Intent pcIntent = new Intent(this, NodePollerActivity.class);
			pcIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
			pcIntent.putExtra("pollType", NodePoller.CONFIGURATION_POLL);
			startActivity(pcIntent);
			return true;
		}
		else if (item.getItemId() == R.id.poll_topology)
		{
			Intent ptIntent = new Intent(this, NodePollerActivity.class);
			ptIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
			ptIntent.putExtra("pollType", NodePoller.TOPOLOGY_POLL);
			startActivity(ptIntent);
			return true;
		}
		else if (item.getItemId() == R.id.poll_interfaces)
		{
			Intent piIntent = new Intent(this, NodePollerActivity.class);
			piIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
			piIntent.putExtra("pollType", NodePoller.INTERFACE_POLL);
			startActivity(piIntent);
			return true;
		}

		// if we didn't match static menu, check if it was some of tools
		List<ObjectTool> tools = service.getTools();
		if (tools != null)
		{
			for (final ObjectTool tool : tools)
			{
				if ((int)tool.getId() == item.getItemId())
				{
					if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
					{
						String message = tool.getConfirmationText()
								.replaceAll("%OBJECT_NAME%", selectedObject.getObjectName())
								.replaceAll("%OBJECT_IP_ADDR%", selectedObject.getPrimaryIP().getHostAddress());
						new AlertDialog.Builder(this)
								.setIcon(android.R.drawable.ic_dialog_alert)
								.setTitle(R.string.confirm_tool_execution)
								.setMessage(message)
								.setCancelable(true)
								.setPositiveButton(R.string.yes, new OnClickListener()
								{
									@Override
									public void onClick(DialogInterface dialog, int which)
									{
										service.executeAction(selectedObject.getObjectId(), tool.getData());
									}
								})
								.setNegativeButton(R.string.no, null)
								.show();
						return true;
					}
					service.executeAction(selectedObject.getObjectId(), tool.getData());
					return true;
				}
			}
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
	 * Refresh node list
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

		Log.i(TAG, "currentParent.getObjectId(): " + currentParent.getObjectId());
		String childList = "";
		for (int i = 0; i < currentParent.getChildIdList().length; i++)
			childList += currentParent.getChildIdList()[i] + " ,";
		Log.i(TAG, "currentParent.getChildIdList(): " + childList);

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
		for (GenericObject o : containerPath)
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
		for (GenericObject o : containerPath)
			path[i++] = o.getObjectId();

		if (currentParent != null)
			path[i++] = currentParent.getObjectId();

		return path;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerNodeBrowser(null);
		super.onDestroy();
	}

	/**
	 * @param nodeIdList
	 */
	private void viewAlarms(ArrayList<Integer> nodeIdList)
	{
		Intent newIntent = new Intent(this, AlarmBrowser.class);
		newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
		startActivity(newIntent);
	}

	/**
	 * Internal task for synching missing objects
	 */
	private class SyncMissingObjectsTask extends AsyncTask<Object, Void, GenericObject[]>
	{
		private final long currentRoot;

		protected SyncMissingObjectsTask(long currentRoot)
		{
			this.currentRoot = currentRoot;
		}

		@Override
		protected GenericObject[] doInBackground(Object... params)
		{
			try
			{
				service.getSession().syncMissingObjects((long[])params[0], false, NXCSession.OBJECT_SYNC_WAIT);
				return currentParent.getChildsAsArray();
			}
			catch (Exception e)
			{
				Log.d(TAG, "Exception while executing service.getSession().syncMissingObjects in SyncMissingObjectsTask", e);
			}
			return null;
		}

		@Override
		protected void onPostExecute(GenericObject[] result)
		{
			if ((result != null) && (currentParent.getObjectId() == currentRoot))
			{
				adapter.setNodes(result);
				adapter.notifyDataSetChanged();
			}
		}
	}

	/**
	 * Internal task for synching missing objects
	 */
	private class SyncMissingChildsTask extends AsyncTask<Integer, Void, Integer>
	{
		private final ArrayList<Integer> childIdList;

		protected SyncMissingChildsTask()
		{
			childIdList = new ArrayList<Integer>();
		}

		protected void getChildsList(long[] list)
		{
			for (int i = 0; i < list.length; i++)
			{
				childIdList.add((int)list[i]);
				GenericObject obj = service.findObjectById(list[i]);
				if (obj != null && (obj.getObjectClass() == GenericObject.OBJECT_CONTAINER ||
						obj.getObjectClass() == GenericObject.OBJECT_CLUSTER))
				{
					try
					{
						service.getSession().syncMissingObjects(obj.getChildIdList(), false, NXCSession.OBJECT_SYNC_WAIT);
						getChildsList(obj.getChildIdList());
					}
					catch (Exception e)
					{
						Log.d(TAG, "Exception while executing service.getSession().syncMissingObjects in SyncMissingChildsTask", e);
					}
				}
			}
		}

		@Override
		protected Integer doInBackground(Integer... params)
		{
			long[] list = new long[params.length];
			for (int i = 0; i < params.length; i++)
				list[i] = params[i].longValue();
			getChildsList(list);
			return 0;
		}

		@Override
		protected void onPostExecute(Integer result)
		{
			viewAlarms(childIdList);
		}
	}
}
