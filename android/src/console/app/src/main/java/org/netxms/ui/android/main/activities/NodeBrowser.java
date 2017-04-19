/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Stack;

import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.NodePollType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.NXApplication;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ObjectListAdapter;
import org.netxms.ui.android.main.fragments.AlarmBrowserFragment;
import org.netxms.ui.android.main.fragments.NodeInfoFragment;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Node browser
 * 
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class NodeBrowser extends AbstractClientActivity
{
	private static final String TAG = "nxclient/NodeBrowser";
	private ListView listView;
	private ObjectListAdapter adapter;
	private long initialParent;
	private AbstractObject currentParent = null;
	private final Stack<AbstractObject> containerPath = new Stack<AbstractObject>();
	private long[] savedPath = null;
	private AbstractObject selectedObject = null;
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
		title.setText(R.string.nodes_title);

		initialParent = getIntent().getIntExtra("parentId", GenericObject.SERVICEROOT);

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
				if ((obj.getObjectClass() == AbstractObject.OBJECT_CONTAINER) || (obj.getObjectClass() == AbstractObject.OBJECT_SUBNET) || (obj.getObjectClass() == AbstractObject.OBJECT_CLUSTER) || (obj.getObjectClass() == AbstractObject.OBJECT_ZONE))
				{
					containerPath.push(currentParent);
					currentParent = obj;
					refreshList();
				}
				else if (obj.getObjectClass() == AbstractObject.OBJECT_NODE || obj.getObjectClass() == AbstractObject.OBJECT_MOBILEDEVICE)
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
		savedPath = getFullPathAsId();
		outState.putLongArray("currentPath", savedPath);
		super.onSaveInstanceState(outState);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onResume()
	 */
	@Override
	protected void onResume()
	{
		super.onResume();
		NXApplication.activityResumed();
		if (service != null)
		{
			service.reconnect(false);
			rescanSavedPath();
		}
	}

	/**
	 * @param objectId
	 */
	public void showNodeInfo(long objectId)
	{
		Intent newIntent = new Intent(this, NodeInfoFragment.class);
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
		rescanSavedPath();
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
		selectedObject = (AbstractObject)adapter.getItem(info.position);

		GeoLocation gl = selectedObject.getGeolocation();
		if ((gl == null) || (gl.getType() == GeoLocation.UNSET))
		{
			hideMenuItem(menu, R.id.navigate_to);
		}

		if (selectedObject instanceof Node)
		{
			// add available tools to context menu
			List<ObjectTool> tools = service.getTools();
			if (tools != null)
			{
				SubMenu subMenu = menu.addSubMenu(Menu.NONE, 0, 0, getString(R.string.menu_tools));
				Iterator<ObjectTool> tl = tools.iterator();
				ObjectTool tool;
				while (tl.hasNext())
				{
					tool = tl.next();
					switch (tool.getType())
					{
						case ObjectTool.TYPE_INTERNAL:
						case ObjectTool.TYPE_ACTION:
						case ObjectTool.TYPE_SERVER_COMMAND:
						case ObjectTool.TYPE_SERVER_SCRIPT:
							if (tool.isApplicableForNode((Node)selectedObject))
								subMenu.add(Menu.NONE, (int)tool.getId(), 0, tool.getDisplayName());
							break;
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
	@SuppressWarnings("deprecation")
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (selectedObject == null)
			return super.onContextItemSelected(item);

		switch (item.getItemId())
		{
			case R.id.find_switch_port:
				Intent fspIntent = new Intent(this, ConnectionPointBrowser.class);
				fspIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
				startActivity(fspIntent);
				break;
			case R.id.view_alarms:
				new SyncMissingChildsTask().execute(new Integer[] { (int)selectedObject.getObjectId() });
				break;
			case R.id.unmanage:
				service.setObjectMgmtState(selectedObject.getObjectId(), false);
				refreshList();
				break;
			case R.id.manage:
				service.setObjectMgmtState(selectedObject.getObjectId(), true);
				refreshList();
				break;
			case R.id.poll_status:
				Intent psIntent = new Intent(this, NodePollerActivity.class);
				psIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
				psIntent.putExtra("pollType", NodePollType.STATUS);
				startActivity(psIntent);
				break;
			case R.id.poll_configuration:
				Intent pcIntent = new Intent(this, NodePollerActivity.class);
				pcIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
				pcIntent.putExtra("pollType", NodePollType.CONFIGURATION_NORMAL);
				startActivity(pcIntent);
				break;
			case R.id.poll_topology:
				Intent ptIntent = new Intent(this, NodePollerActivity.class);
				ptIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
				ptIntent.putExtra("pollType", NodePollType.TOPOLOGY);
				startActivity(ptIntent);
				break;
			case R.id.poll_interfaces:
				Intent piIntent = new Intent(this, NodePollerActivity.class);
				piIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
				piIntent.putExtra("pollType", NodePollType.INTERFACES);
				startActivity(piIntent);
				break;
			case R.id.navigate_to:
				GeoLocation gl = selectedObject.getGeolocation();
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("google.navigation:q=" + gl.getLatitude() + "," + gl.getLongitude()));
				intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
				try
				{
					startActivity(intent);
				}
				catch (ActivityNotFoundException e)
				{
					Toast.makeText(getApplicationContext(), "Navigation unavailable", Toast.LENGTH_LONG);
				}
				break;
			default:
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
								String message = tool.getConfirmationText().replaceAll("%n", selectedObject.getObjectName()).replaceAll("%a", ((Node)selectedObject).getPrimaryIP().getHostAddress());
								new AlertDialog.Builder(this).setIcon(android.R.drawable.ic_dialog_alert).setTitle(R.string.confirm_tool_execution).setMessage(message).setCancelable(true).setPositiveButton(R.string.yes, new OnClickListener()
								{
									@Override
									public void onClick(DialogInterface dialog, int which)
									{
										service.executeObjectTool(selectedObject.getObjectId(), tool);
									}
								}).setNegativeButton(R.string.no, null).show();
								break;
							}
							service.executeObjectTool(selectedObject.getObjectId(), tool);
							break;
						}
					}
				}
				break;
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
	 * Refresh node list, force reload from server
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
		new SyncMissingObjectsTask(currentParent.getObjectId()).execute(new Object[] { currentParent.getChildIdList() });
	}

	/**
	 * Rescan saved path
	 */
	private void rescanSavedPath()
	{
		// Restore to saved path if available
		if ((savedPath != null) && (savedPath.length > 0))
		{
			containerPath.clear();
			for (int i = 0; i < savedPath.length - 1; i++)
			{
				AbstractObject object = service.findObjectById(savedPath[i]);
				if (object == null)
					break;
				containerPath.push(object);
				Log.i(TAG, "object.getObjectId(): " + object.getObjectId());
			}
			currentParent = service.findObjectById(savedPath[savedPath.length - 1]);
			savedPath = null;
		}
		refreshList();
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
	 * Update node list, force refresh as necessary
	 */
	public void updateNodeList()
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
	 * @param nodeIdList
	 */
	private void viewAlarms(ArrayList<Integer> nodeIdList)
	{
		Intent newIntent = new Intent(this, AlarmBrowserFragment.class);
		newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
		startActivity(newIntent);
	}

	/**
	 * Internal task for synching missing objects
	 */
	private class SyncMissingObjectsTask extends AsyncTask<Object, Void, AbstractObject[]>
	{
		private final long currentRoot;

		protected SyncMissingObjectsTask(long currentRoot)
		{
			this.currentRoot = currentRoot;
		}

		@Override
		protected void onPreExecute()
		{
			if (dialog != null)
			{
				dialog.setMessage(getString(R.string.progress_gathering_data));
				dialog.setIndeterminate(true);
				dialog.setCancelable(false);
				dialog.show();
			}
		}

		@Override
		protected AbstractObject[] doInBackground(Object... params)
		{
			try
			{
				service.getSession().syncMissingObjects((long[])params[0], false, NXCSession.OBJECT_SYNC_WAIT);
				return currentParent.getChildsAsArray();
			}
			catch (Exception e)
			{
				Log.e(TAG, "Exception while executing service.getSession().syncMissingObjects in SyncMissingObjectsTask", e);
			}
			return null;
		}

		@Override
		protected void onPostExecute(AbstractObject[] result)
		{
			if (dialog != null)
				dialog.cancel();
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

		@Override
		protected void onPreExecute()
		{
			if (dialog != null)
			{
				dialog.setMessage(getString(R.string.progress_gathering_data));
				dialog.setIndeterminate(true);
				dialog.setCancelable(false);
				dialog.show();
			}
		}

		protected void getChildsList(long[] list)
		{
			for (int i = 0; i < list.length; i++)
			{
				childIdList.add((int)list[i]);
				AbstractObject obj = service.findObjectById(list[i]);
				if (obj != null && (obj.getObjectClass() == AbstractObject.OBJECT_CONTAINER || obj.getObjectClass() == AbstractObject.OBJECT_CLUSTER))
				{
					try
					{
						service.getSession().syncMissingObjects(obj.getChildIdList(), false, NXCSession.OBJECT_SYNC_WAIT);
						getChildsList(obj.getChildIdList());
					}
					catch (Exception e)
					{
						Log.e(TAG, "Exception while executing service.getSession().syncMissingObjects in SyncMissingChildsTask", e);
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
			if (dialog != null)
				dialog.cancel();
			viewAlarms(childIdList);
		}
	}
}
