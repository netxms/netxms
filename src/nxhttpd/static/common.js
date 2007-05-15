/*** Set button's state ***/
function setButtonState(name, isPressed)
{
	img = document.getElementById('img_' + name);
	if (img != null)
	{
		img.src = '/images/buttons/' + (isPressed ? 'pressed' : 'normal') + '/' + name + '.png';
	}
	return true;
}

/*** Toggle checkbox ***/
function toggleCheckbox(handler, name)
{
	img = document.getElementById('img_' + name);
	if (img != null)
	{
		var newState = (img.src.indexOf('checkbox_on') != -1) ? 0 : 1;
		img.src = '/images/checkbox_' + ((newState == 0) ? 'off' : 'on') + '.png';
		if (handler != null)
		{
			handler(name, newState);
		}
	}
	return true;
}

/*** Reload content of given div ***/
function loadDivContent(divId, sid, cmd)
{
	var xmlHttp = XmlHttp.create();
	xmlHttp.open('GET', '/main.app?sid=' + sid + cmd, false);
	xmlHttp.send(null);
	document.getElementById(divId).innerHTML = xmlHttp.responseText;
	if (self.correctPNG)
	{
		correctPNG();
	}
}

/*** Change height of single div ***/
function changeDivHeight(winHeight, elementId)
{
	var element = document.getElementById(elementId);
	if (element != null)
	{
		var t = element.offsetTop;
		p = element.offsetParent;
		while(p != null)
		{
			if (typeof(p.offsetTop) == 'number')
			{
				t += p.offsetTop;
			}
			p = p.offsetParent;
		}
		element.style.height = (winHeight - t) + "px";
	}
}

/*** Resize all needed elements on page ***/
function resizeElements()
{
	var winHeight = 0;
	if (typeof(window.innerWidth) == 'number')
	{
		// Non-IE
		winHeight = window.innerHeight;
	} 
	else if (document.documentElement && (document.documentElement.clientWidth || document.documentElement.clientHeight))
	{
		//IE 6+ in 'standards compliant mode'
		winHeight = document.documentElement.clientHeight;
	}
	else if (document.body && (document.body.clientWidth || document.body.clientHeight))
	{
		//IE 4 compatible
		winHeight = document.body.clientHeight;
	}

	changeDivHeight(winHeight, 'object_tree');
	changeDivHeight(winHeight, 'object_data');
	changeDivHeight(winHeight, 'alarm_view');
}

/*** Event handlers ***/
if (window.addEventListener)
{
	window.addEventListener("load", resizeElements, false);
	window.addEventListener("resize", resizeElements, false);
}
else if (window.attachEvent)
{
	window.attachEvent("onload", resizeElements);
	window.attachEvent("onresize", resizeElements);
}
