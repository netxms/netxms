/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.netxms.ui.android.main.fragments;

import java.util.ArrayList;
import java.util.List;

import org.netxms.ui.android.R;

import android.content.ComponentName;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.ImageSpan;

/**
 * Node info fragment
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class NodeInfoFragment extends AbstractFragmentActivity
{
	public static final int TAB_OVERVIEW_ID = 0;
	public static final int TAB_ALARMS_ID = 1;
	public static final int TAB_LAST_VALUES_ID = 2;
	public static final int TAB_INTERFACES_ID = 3;

	private NodeInfoAdapter adapter;
	private long nodeId = 0;
	private int tabId = TAB_OVERVIEW_ID;

	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.nodeinfo_pager_fragment);

		nodeId = getIntent().getLongExtra("objectId", 0);
		tabId = getIntent().getIntExtra("tabId", TAB_OVERVIEW_ID);

		adapter = new NodeInfoAdapter(getSupportFragmentManager(), getResources());

		OverviewFragment overview = new OverviewFragment();
		overview.setNodeId(nodeId);
		adapter.setItem(TAB_OVERVIEW_ID, overview, getString(R.string.ni_overview), R.drawable.ni_overview_tab);
		AlarmsFragment alarms = new AlarmsFragment();
		alarms.setNodeId(nodeId);
		alarms.enableLastValuesMenu(false);
		adapter.setItem(TAB_ALARMS_ID, alarms, getString(R.string.ni_alarms), R.drawable.ni_alarms_tab);
		LastValuesFragment lastValues = new LastValuesFragment();
		lastValues.setNodeId(nodeId);
		adapter.setItem(TAB_LAST_VALUES_ID, lastValues, getString(R.string.ni_last_values), R.drawable.ni_last_values_tab);
		InterfacesFragment interfaces = new InterfacesFragment();
		interfaces.setNodeId(nodeId);
		adapter.setItem(TAB_INTERFACES_ID, interfaces, getString(R.string.ni_interfaces), R.drawable.ni_interfaces_tab);

		ViewPager pager = (ViewPager)findViewById(R.id.pager);
		pager.setAdapter(adapter);
		pager.setCurrentItem(tabId, true);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		if (service != null)
		{
			for (int i = 0; i < adapter.getCount(); i++)
			{
				AbstractListFragment frag = ((AbstractListFragment)adapter.getItem(i));
				if (frag != null)
				{
					frag.setNodeId(nodeId);
					frag.setService(service);
					frag.refresh();
				}
			}
		}
	}

	public static class NodeInfoAdapter extends FragmentPagerAdapter
	{
		private final List<Fragment> fragments = new ArrayList<Fragment>(0);
		private final List<String> labels = new ArrayList<String>(0);
		private final List<Integer> icons = new ArrayList<Integer>(0);
		private final Resources r;

		public NodeInfoAdapter(FragmentManager fm, Resources r)
		{
			super(fm);
			this.r = r;
		}

		@Override
		public int getCount()
		{
			return fragments.size();
		}

		@Override
		public Fragment getItem(int position)
		{
			if (position >= 0 && position < fragments.size())
				return fragments.get(position);
			return null;
		}

		@SuppressWarnings("deprecation")
		@Override
		public CharSequence getPageTitle(int position)
		{
			if (position >= 0 && position < labels.size())
			{
				SpannableStringBuilder sb = new SpannableStringBuilder("  " + labels.get(position));// space added before text for convenience
				Drawable icon = r.getDrawable(icons.get(position));
				icon.setBounds(0, 0, icon.getIntrinsicWidth(), icon.getIntrinsicHeight());
				ImageSpan span = new ImageSpan(icon, ImageSpan.ALIGN_BASELINE);
				sb.setSpan(span, 0, 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
				return sb;
			}
			return "";
		}

		public void setItem(int position, Fragment fragment, String label, int icon)
		{
			fragments.add(position, fragment);
			labels.add(position, label);
			icons.add(position, icon);
		}
	}
}
