/**
 * 
 */
package org.netxms.agent.android.main.views;

import android.content.Context;
import android.widget.CheckBox;
import android.widget.Checkable;
import android.widget.LinearLayout;

/**
 * Linear layout implementing checkable interface
 */
public class CheckableLinearLayout extends LinearLayout implements Checkable
{
	private final CheckBox checkBox;

	/**
	 * @param context
	 */
	public CheckableLinearLayout(Context context)
	{
		super(context);
		checkBox = new CheckBox(context)
		{

		};
		checkBox.setFocusable(false);
		checkBox.setClickable(false);
		addView(checkBox);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Checkable#isChecked()
	 */
	@Override
	public boolean isChecked()
	{
		return checkBox.isChecked();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Checkable#setChecked(boolean)
	 */
	@Override
	public void setChecked(boolean checked)
	{
		checkBox.setChecked(checked);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.Checkable#toggle()
	 */
	@Override
	public void toggle()
	{
		checkBox.toggle();
	}
}
