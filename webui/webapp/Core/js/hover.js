/**
 * hover.js
 * 
 * License: MIT License - http://www.opensource.org/licenses/mit-license.php
 * Copyright (c) 2018 Raden Solutions
 */

var handleEvent = function(e) 
{ 
	var t = e.widget.getData("hoverTimer");
	if (t != null) 
	{ 
		clearTimeout(t);
	}
	if (e.type == SWT.MouseEnter || e.type == SWT.MouseMove)
	{ 
		var widget = e.widget;
		var x = e.x;
		var y = e.y;
		t = setTimeout(function() 
			{ 
				var proxyId = widget.getData("msgProxyWidget");
				proxy = rap.getObject(proxyId);
				if (proxy)
				{
					rap.getRemoteObject(proxy).notify("mouseHover", { "x":x, "y":y });
					widget.setData("notifyOnExit", proxyId);
				}
			}, 400); 
		widget.setData("hoverTimer", t);
		widget.setData("notifyOnExit", null);
	}
	else 
	{
		e.widget.setData("hoverTimer", null);
		var proxyId = e.widget.getData("notifyOnExit");
		if (proxyId)
		{
			proxy = rap.getObject(proxyId);
			if (proxy)
			{
				rap.getRemoteObject(proxy).notify("mouseExit", { "x":e.x, "y":e.y });
			}
			e.widget.setData("notifyOnExit", null);
		}
	}
}
