/**
 * 
 */
package org.netxms.ui.android.main;

import java.util.Iterator;
import java.util.List;
import java.util.Stack;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.NodeListAdapter;
import org.netxms.ui.android.service.ClientConnectorService;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
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
public class NodeBrowser extends Activity implements ServiceConnection
{
	private ClientConnectorService service;
	private ListView listView;
	private NodeListAdapter adapter;
	private long initialParent = 2;
	private GenericObject currentParent = null;
	private Stack<GenericObject> containerPath = new Stack<GenericObject>();

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.node_view);

		bindService(new Intent(this, ClientConnectorService.class), this, 0);

		// keeps current list of nodes as datasource for listview
		adapter = new NodeListAdapter(this);

		listView = (ListView)findViewById(R.id.NodeList);
		listView.setAdapter(adapter);
		listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
		{
			public void onItemClick(AdapterView parent, View v, int position, long id)
			{
				GenericObject obj = (GenericObject)adapter.getItem(position);
				if (obj.getObjectClass() == GenericObject.OBJECT_CONTAINER)
				{
					containerPath.push(currentParent);
					currentParent = obj;
					refreshList();
				}
				if (obj.getObjectClass() == GenericObject.OBJECT_NODE)
				{
					showLastValues(obj.getObjectId());
				}
			}
		});

		registerForContextMenu(listView);
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
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		// make sure adapter can read required additional data
		adapter.setService(service);
		// remember this activity in service, so that service can send updates
		service.registerNodeBrowser(this);
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
			
		// don't forget about 'cancel' item :)
		menu.add(R.string.cancel);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		// get selected item
		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		GenericObject obj = (GenericObject)adapter.getItem(info.position);

		// process menu selection
		switch(item.getItemId())
		{
			case R.id.unmanage:
				adapter.unmanageObject(obj.getObjectId());
				refreshList();
				return true;
			case R.id.manage:
				adapter.manageObject(obj.getObjectId());
				refreshList();
				return true;
			default:
		}

		// if we didn't match static menu, check if it was some of tools
		List<ObjectTool> tools = service.getTools();
		if (tools != null)
		{
			Iterator<ObjectTool> tl = tools.iterator();
			ObjectTool tool;
			while(tl.hasNext())
			{
				tool = tl.next();
				if ((int)tool.getId() == item.getItemId())
				{
					service.executeAction(obj.getObjectId(), tool.getData());
					return true;
				}
			}
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

		TextView curPath = (TextView)findViewById(R.id.NodePath);
		curPath.setText(currentParent.getObjectName());
		adapter.setNodes(service.findChilds(this.currentParent));
		adapter.notifyDataSetChanged();
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
}
