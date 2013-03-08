package org.netxms.ui.android.main.fragments;

import android.os.Bundle;
import android.os.Handler;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.FrameLayout;
import android.widget.ListView;

/**
 * ExpandableList for fragment usage
 * 
 * This class has originally been taken from
 * http://stackoverflow.com/questions/6051050/expandablelistfragment-with-loadermanager-for-compatibility-package
 * and then modified and adapted to be used into the NetXMS Android console.
 *
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class ExpandableListFragment extends AbstractListFragment
		implements OnCreateContextMenuListener, ExpandableListView.OnChildClickListener,
		ExpandableListView.OnGroupCollapseListener, ExpandableListView.OnGroupExpandListener
{
	static final int INTERNAL_LIST_CONTAINER_ID = 0x00ff0003;

	final private Handler mHandler = new Handler();

	final private Runnable mRequestFocus = new Runnable()
	{
		@Override
		public void run()
		{
			mList.focusableViewAvailable(mList);
		}
	};

	final private AdapterView.OnItemClickListener mOnClickListener = new AdapterView.OnItemClickListener()
	{
		@Override
		public void onItemClick(AdapterView<?> parent, View v, int position, long id)
		{
			onListItemClick((ListView)parent, v, position, id);
		}
	};

	ExpandableListAdapter mAdapter;
	ExpandableListView mList;
	boolean mFinishedStart = false;

	public ExpandableListFragment()
	{
	}

	/**
	 * Provide default implementation to return a simple list view.  Subclasses
	 * can override to replace with their own layout.  If doing so, the
	 * returned view hierarchy <em>must</em> have a ListView whose id
	 * is {@link android.R.id#list android.R.id.list} and can optionally
	 * have a sibling view id {@link android.R.id#empty android.R.id.empty}
	 * that is to be shown when the list is empty.
	 * <p/>
	 * <p>If you are overriding this method with your own custom content,
	 * consider including the standard layout {@link android.R.layout#list_content}
	 * in your layout file, so that you continue to retain all of the standard
	 * behavior of ListFragment.  In particular, this is currently the only
	 * way to have the built-in indeterminant progress state be shown.
	 */
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
	{
		FrameLayout root = new FrameLayout(getActivity());

		FrameLayout lframe = new FrameLayout(getActivity());
		lframe.setId(INTERNAL_LIST_CONTAINER_ID);

		ExpandableListView lv = new ExpandableListView(getActivity());
		lv.setId(android.R.id.list);
		lv.setDrawSelectorOnTop(false);
		lv.setOnChildClickListener(this);
		lv.setOnGroupExpandListener(this);
		lv.setOnGroupCollapseListener(this);
		lframe.addView(lv,
				new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

		root.addView(lframe, new FrameLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

		ListView.LayoutParams lp =
				new ListView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
		root.setLayoutParams(lp);
		return root;
	}

	/**
	 * Attach to list view once the view hierarchy has been created.
	 */
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState)
	{
		super.onViewCreated(view, savedInstanceState);
		ensureList();
	}

	/** Detach from list view. */
	@Override
	public void onDestroyView()
	{
		mHandler.removeCallbacks(mRequestFocus);
		mList = null;
		super.onDestroyView();
	}

	/**
	 * This method will be called when an item in the list is selected.
	 * Subclasses should override. Subclasses can call
	 * getListView().getItemAtPosition(position) if they need to access the
	 * data associated with the selected item.
	 * @param l        The ListView where the click happened
	 * @param v        The view that was clicked within the ListView
	 * @param position The position of the view in the list
	 * @param id       The row id of the item that was clicked
	 */
	@Override
	public void onListItemClick(ListView l, View v, int position, long id)
	{
	}

	/** Provide the cursor for the list view. */
	public void setListAdapter(ExpandableListAdapter adapter)
	{
		boolean hadAdapter = mAdapter != null;
		mAdapter = adapter;
		if (mList != null)
		{
			mList.setAdapter(adapter);
			if (!listShown && !hadAdapter)
			{
				// The list was hidden, and previously didn't have an
				// adapter.  It is now time to show it.
				setListShown(true, getView().getWindowToken() != null);
			}
		}
	}

	/**
	 * Set the currently selected list item to the specified
	 * position with the adapter's data
	 */
	@Override
	public void setSelection(int position)
	{
		ensureList();
		mList.setSelection(position);
	}

	public long getSelectedPosition()
	{
		ensureList();
		return mList.getSelectedPosition();
	}

	public long getSelectedId()
	{
		ensureList();
		return mList.getSelectedId();
	}

	public ExpandableListView getExpandableListView()
	{
		ensureList();
		return mList;
	}

	/** Get the ListAdapter associated with this activity's ListView. */
	public ExpandableListAdapter getExpandableListAdapter()
	{
		return mAdapter;
	}

	private void ensureList()
	{
		if (mList != null)
		{
			return;
		}
		View root = getView();
		if (root == null)
		{
			throw new IllegalStateException("Content view not yet created");
		}
		if (root instanceof ExpandableListView)
		{
			mList = (ExpandableListView)root;
		}
		else
		{
			View rawListView = root.findViewById(android.R.id.list);
			if (!(rawListView instanceof ExpandableListView))
			{
				if (rawListView == null)
				{
					throw new RuntimeException("Your content must have a ExpandableListView whose id attribute is 'android.R.id.list'");
				}
				throw new RuntimeException("Content has view with id attribute 'android.R.id.list' that is not a ExpandableListView class");
			}
			mList = (ExpandableListView)rawListView;
		}
		mList.setOnItemClickListener(mOnClickListener);
		if (mAdapter != null)
		{
			setListAdapter(mAdapter);
		}
		else
		{
			// We are starting without an adapter, so assume we won't
			// have our data right away and start with the progress indicator.
			setListShown(false, false);
		}
		mHandler.post(mRequestFocus);
	}

	@Override
	public void onGroupExpand(int arg0)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public void onGroupCollapse(int arg0)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public boolean onChildClick(ExpandableListView arg0, View arg1, int arg2, int arg3, long arg4)
	{
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
	}

	public void onContentChanged()
	{
		View emptyView = getView().findViewById(android.R.id.empty);
		mList = (ExpandableListView)getView().findViewById(android.R.id.list);
		if (mList == null)
		{
			throw new RuntimeException(
					"Your content must have a ExpandableListView whose id attribute is 'android.R.id.list'");
		}
		if (emptyView != null)
		{
			mList.setEmptyView(emptyView);
		}
		mList.setOnChildClickListener(this);
		mList.setOnGroupExpandListener(this);
		mList.setOnGroupCollapseListener(this);

		if (mFinishedStart)
		{
			setListAdapter(mAdapter);
		}
		mFinishedStart = true;
	}

	@Override
	public void refresh()
	{
	}
}
