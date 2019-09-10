package org.netxms.agent.android.helpers;

import android.content.SharedPreferences;

/**
 * Helper for storing values in shared preferences
 *
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class SharedPrefs
{
    /**
     * Helper to write boolean into shared preferences
     *
     * @param prefs shared preference where to write the value
     * @param key name of the key
     * @param value boolean value to set
     */
    static public void writeBoolean(SharedPreferences prefs, String key, boolean value)
    {
        if (prefs != null)
        {
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean(key, value);
            editor.commit();
        }
    }

    /**
     * Helper to write string into shared preferences
     *
     * @param prefs shared preference where to write the value
     * @param key name of the key
     * @param value string value to set
     */
    static public void writeString(SharedPreferences prefs, String key, String value)
    {
        if (prefs != null)
        {
            SharedPreferences.Editor editor = prefs.edit();
            editor.putString(key, value);
            editor.commit();
        }
    }

    /**
     * Helper to write long into shared preferences
     *
     * @param prefs shared preference where to write the value
     * @param key name of the key
     * @param value long value to set
     */
    static public void writeLong(SharedPreferences prefs, String key, long value)
    {
        if (prefs != null)
        {
            SharedPreferences.Editor editor = prefs.edit();
            editor.putLong(key, value);
            editor.commit();
        }
    }
}
