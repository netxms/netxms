/**
 * 
 */
package org.netxms.ui.android.main.views;

import android.content.Context;
import android.view.Gravity;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * View representing single element in activity list
 *
 */
public class ActivityListElement extends LinearLayout
{
	/**
	 * @param context
	 */
	public ActivityListElement(Context context, int imageId, int textId)
	{
		super(context);
		setOrientation(VERTICAL);
		setGravity(Gravity.CENTER_HORIZONTAL);
		
		ImageView imageView = new ImageView(context);
		imageView.setImageResource(imageId);
		//imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
		imageView.setPadding(8, 8, 8, 8);
		addView(imageView, new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		
		TextView text = new TextView(context);
		text.setText(textId);
		//text.setTextColor(0);
		text.setGravity(Gravity.CENTER_HORIZONTAL);
		addView(text, new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
	}
}
