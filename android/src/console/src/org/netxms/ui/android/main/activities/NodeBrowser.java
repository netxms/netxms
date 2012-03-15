/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Stack;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.NodeListAdapter;

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
 * 
 */
public class NodeBrowser extends AbstractClientActivity
{
	private ListView listView;
	private NodeListAdapter adapter;
	private long initialParent = 2;
	private GenericObject currentParent = null;
	private Stack<GenericObject> containerPath = new Stack<GenericObject>();
	private long[] savedPath = null;

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
		adapter = new NodeListAdapter(this);

		listView = (ListView)findViewById(R.id.NodeList);
		listView.setAdapter(adapter);
		listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
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
					showLastValues(obj.getObjectId());
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
	public void showLastValues(long objectId)
	{
		Intent newIntent = new Intent(this, LastValues.class);
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
		
		adapter.setService(service);
		service.registerNodeBrowser(this);

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

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * android.content.ServiceConnection#onServiceDisconnected(android.content
	 * .ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
		adapter.setService(null);
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
		GenericObject object = (GenericObject)adapter.getItem(info.position);

		if (object instanceof Node)
		{
			// add available tools to context menu
			List<ObjectTool> tools = service.getTools();
			if (tools != null)
			{
				Iterator<ObjectTool> tl = tools.iterator();
				ObjectTool tool;
				while(tl.hasNext())
				{
					tool = tl.next();
					if ((tool.getType() == ObjectTool.TYPE_ACTION || tool.getType() == ObjectTool.TYPE_SERVER_COMMAND) &&
					    tool.isApplicableForNode((Node)object))
					{
						menu.add(Menu.NONE, (int)tool.getId(), 0, tool.getDisplayName());
					}
				}
			}
		}
		else
			menu.getItem(0).setVisible(false); 

	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		// get selected item
		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		final GenericObject object = (GenericObject)adapter.getItem(info.position);

		// process menu selection
		switch(item.getItemId())
		{
			case R.id.find_switch_port:
				Intent newIntent = new Intent(this, ConnectionPointBrowser.class);
				newIntent.putExtra("nodeId", (int)object.getObjectId());
				startActivity(newIntent);
				return true;
			case R.id.view_alarms:
				new SyncMissingChildsTask().execute(new Integer[] { (int)object.getObjectId() });
				return true;
			case R.id.unmanage:
				adapter.unmanageObject(object.getObjectId());
				refreshList();
				return true;
			case R.id.manage:
				adapter.manageObject(object.getObjectId());
				refreshList();
				return true;
			default:
		}

		// if we didn't match static menu, check if it was some of tools
		List<ObjectTool> tools = service.getTools();
		if (tools != null)
		{
			for(final ObjectTool tool : tools)
			{
				if ((int)tool.getId() == item.getItemId())
				{
					if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
					{	
						String message = tool.getConfirmationText()
								.replaceAll("%OBJECT_NAME%", object.getObjectName())
								.replaceAll("%OBJECT_IP_ADDR%", object.getPrimaryIP().getHostAddress());
						new AlertDialog.Builder(this)
							.setIcon(android.R.drawable.ic_dialog_alert)
							.setTitle(R.string.confirm_tool_execution)
							.setMessage(message)
							.setCancelable(true)
							.setPositiveButton(R.string.yes, new OnClickListener() {
								@Override
								public void onClick(DialogInterface dialog, int which)
								{
									service.executeAction(object.getObjectId(), tool.getData());
								}
							})
							.setNegativeButton(R.string.no, null)
							.show();
						return true;
					}
					service.executeAction(object.getObjectId(), tool.getData());
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

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerNodeBrowser(null);
		unbindService(this);
		super.onDestroy();
	}
	
	
	private void viewAlarms(ArrayList<Integer> nodeIdList)
	{
		Intent newIntent = new Intent(this, AlarmBrowser.class);
		newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
		startActivity(newIntent);
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
			if ((result == null) && (NodeBrowser.this.currentParent.getObjectId() == currentRoot))
			{
				adapter.setNodes(currentParent.getChildsAsArray());				
				adapter.notifyDataSetChanged();
			}
		}
	}

	
	/**
	 * Internal task for synching missing objects
	 */
	private class SyncMissingChildsTask extends AsyncTask<Integer, Void, Integer>
	{
		private ArrayList<Integer> childIdList;
		
		protected SyncMissingChildsTask()
		{
			childIdList = new ArrayList<Integer>();
		}
		
		protected void getChildsList(long[] list)
		{
			for (int i = 0; i< list.length; i++)
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
					catch(Exception e)
					{
						Log.d("nxclient/SyncMissingChildsTask", "Exception while executing service.getSession().syncMissingObjects", e);
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
