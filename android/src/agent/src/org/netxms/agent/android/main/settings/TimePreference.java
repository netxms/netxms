package org.netxms.agent.android.main.settings;

import org.netxms.agent.android.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.preference.DialogPreference;
import android.text.format.DateFormat;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TimePicker;

/**
 * Custom handler for time picker
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class TimePreference extends DialogPreference
{
	private int lastHour = 0;
	private int lastMinute = 0;
	private TimePicker picker = null;

	public static int getHour(String time)
	{
		String[] pieces = time.split(":");

		return (Integer.parseInt(pieces[0]));
	}

	public static int getMinute(String time)
	{
		String[] pieces = time.split(":");

		return (Integer.parseInt(pieces[1]));
	}

	public TimePreference(Context ctxt)
	{
		this(ctxt, null);
	}

	public TimePreference(Context ctxt, AttributeSet attrs)
	{
		this(ctxt, attrs, 0);
	}

	public TimePreference(Context ctxt, AttributeSet attrs, int defStyle)
	{
		super(ctxt, attrs, defStyle);

		setPositiveButtonText(ctxt.getResources().getString(R.string.set));
		setNegativeButtonText(ctxt.getResources().getString(R.string.cancel));
	}

	@Override
	protected View onCreateDialogView()
	{
		picker = new TimePicker(getContext());
		picker.setIs24HourView(DateFormat.is24HourFormat(this.getContext()));
		picker.setAddStatesFromChildren(true);
		return (picker);
	}

	@Override
	protected void onBindDialogView(View v)
	{
		super.onBindDialogView(v);

		picker.setCurrentHour(lastHour);
		picker.setCurrentMinute(lastMinute);
	}

	@Override
	protected void onDialogClosed(boolean positiveResult)
	{
		picker.clearFocus(); // Force reading of values input by keyboard
		super.onDialogClosed(positiveResult);

		if (positiveResult)
		{
			lastHour = picker.getCurrentHour();
			lastMinute = picker.getCurrentMinute();

			String time = String.valueOf(lastHour) + ":" + String.valueOf(lastMinute);

			if (callChangeListener(time))
				persistString(time);
		}
	}

	@Override
	protected Object onGetDefaultValue(TypedArray a, int index)
	{
		return (a.getString(index));
	}

	@Override
	protected void onSetInitialValue(boolean restoreValue, Object defaultValue)
	{
		String time = null;

		if (restoreValue)
		{
			if (defaultValue == null)
				time = getPersistedString("00:00");
			else
				time = getPersistedString(defaultValue.toString());
		}
		else
		{
			time = defaultValue.toString();
		}
		lastHour = getHour(time);
		lastMinute = getMinute(time);
	}
}
