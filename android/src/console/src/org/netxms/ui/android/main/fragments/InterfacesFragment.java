/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.android.R;
import org.netxms.ui.android.loaders.GenericObjectChildrenLoader;
import org.netxms.ui.android.main.activities.ConnectionPointBrowser;
import org.netxms.ui.android.main.adapters.InterfacesAdapter;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ExpandableListView;
import android.widget.ListView;

/**
 * Fragment for last values info
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class InterfacesFragment extends ExpandableListFragment implements LoaderManager.LoaderCallbacks<Set<AbstractObject>>
{
	private ListView lv = null;
	private InterfacesAdapter adapter = null;
	private final GenericObjectChildrenLoader loader = null;
	private AbstractObject selectedObject = null;
	private static final int IF_EXPECTED_STATE_UP = 0;
	private static final int IF_EXPECTED_STATE_DOWN = 1;
	private static final int IF_EXPECTED_STATE_IGNORE = 2;
	private static final String TAG = "nxclient/InterfacesFragment";

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
	{
		View v = inflater.inflate(R.layout.interfaces_fragment, container, false);
		createProgress(v);
		return v;
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState)
	{
		super.onActivityCreated(savedInstanceState);
		adapter = new InterfacesAdapter(getActivity(), null, null);
		setListAdapter(adapter);
		setListShown(false, true);
		GenericObjectChildrenLoader loader = (GenericObjectChildrenLoader)getActivity().getSupportLoaderManager().initLoader(R.layout.interfaces_fragment, null, this);
		if (loader != null)
		{
			loader.setObjId(nodeId);
			loader.setClassFilter(AbstractObject.OBJECT_INTERFACE);
			loader.setService(service);
		}
		lv = getListView();
		registerForContextMenu(lv);
		lv.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener()
		{
			@Override
			public boolean onItemLongClick(AdapterView<?> arg0, View v, int position, long id)
			{
				if (ExpandableListView.getPackedPositionType(id) == ExpandableListView.PACKED_POSITION_TYPE_GROUP)
				{
					selectedObject = (AbstractObject)adapter.getChild(ExpandableListView.getPackedPositionGroup(id), 0);
				}
				return false;
			}
		});
	}
	@Override
	public void refresh()
	{
		if (loader != null)
		{
			loader.setObjId(nodeId);
			loader.setService(service);
			loader.forceLoad();
		}
	}

	/* (non-Javadoc)
	 * @see android.support.v4.app.LoaderManager.LoaderCallbacks#onCreateLoader(int, android.os.Bundle)
	 */
	@Override
	public Loader<Set<AbstractObject>> onCreateLoader(int arg0, Bundle arg1)
	{
		return new GenericObjectChildrenLoader(getActivity());
	}

	/* (non-Javadoc)
	 * @see android.support.v4.app.LoaderManager.LoaderCallbacks#onLoadFinished(android.support.v4.content.Loader, java.lang.Object)
	 */
	@Override
	public void onLoadFinished(Loader<Set<AbstractObject>> arg0, Set<AbstractObject> arg1)
	{
		setListShown(true, true);
		if (adapter != null)
		{
			List<Interface> interfaces = null;
			if (arg1 != null)
			{
				interfaces = new ArrayList<Interface>();
				for (AbstractObject go : arg1)
					interfaces.add((Interface)go);
			}
			adapter.setValues(interfaces);
			adapter.notifyDataSetChanged();
		}
	}
	/* (non-Javadoc)
	 * @see android.support.v4.app.LoaderManager.LoaderCallbacks#onLoaderReset(android.support.v4.content.Loader)
	 */
	@Override
	public void onLoaderReset(Loader<Set<AbstractObject>> arg0)
	{
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getActivity().getMenuInflater();
		inflater.inflate(R.menu.interface_actions, menu);
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
	{
		inflater.inflate(R.menu.interface_actions, menu);
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public void onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (!getUserVisibleHint())
			return false;
		if (handleItemSelection(item))
			return true;
		return super.onContextItemSelected(item);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if (handleItemSelection(item))
			return true;
		return super.onOptionsItemSelected(item);
	}

	/**
	 * Handles menu item selection for both Option and Context menus
	 * @param item	Menu item to handle
	 * @return true if menu has been properly handled
	 */
	private boolean handleItemSelection(MenuItem item)
	{
		switch (item.getItemId())
		{
			case R.id.manage:
				if (selectedObject != null)
					service.setObjectMgmtState(selectedObject.getObjectId(), true);
				return true;
			case R.id.unmanage:
				if (selectedObject != null)
					service.setObjectMgmtState(selectedObject.getObjectId(), false);
				return true;
			case R.id.if_expected_state_up:
				return modifyExpectedState(IF_EXPECTED_STATE_UP);
			case R.id.if_expected_state_down:
				return modifyExpectedState(IF_EXPECTED_STATE_DOWN);
			case R.id.if_expected_state_ignore:
				return modifyExpectedState(IF_EXPECTED_STATE_IGNORE);
			case R.id.find_switch_port:
				Intent fspIntent = new Intent(getActivity(), ConnectionPointBrowser.class);
				if (fspIntent != null && selectedObject != null)
				{
					fspIntent.putExtra("nodeId", (int)selectedObject.getObjectId());
					startActivity(fspIntent);
					return true;
				}
		}
		return false;
	}

	/**
	 * Modifies expected state of the selected interface
	 * @param newState	The new infterface state
	 * @return true if the command can be issued properly
	 */
	private boolean modifyExpectedState(int newState)
	{
		if (selectedObject != null)
		{
			NXCSession session = service.getSession();
			if (session != null)
			{
				NXCObjectModificationData md = new NXCObjectModificationData(selectedObject.getObjectId());
				md.setExpectedState(newState);
				try
				{
					session.modifyObject(md);
				}
				catch (NXCException e)
				{
					Log.e(TAG, "NXCException in modifyState...", e);
					e.printStackTrace();
				}
				catch (IOException e)
				{
					Log.e(TAG, "IOException in modifyState...", e);
					e.printStackTrace();
				}
				return true;
			}
		}
		return false;
	}
}
