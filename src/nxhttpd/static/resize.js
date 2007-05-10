function changeDivHeight(winHeight, elementId)
{
	var element = document.getElementById(elementId);
	if (element != null)
	{
		element.style.height = (winHeight - element.offsetTop) + "px";
	}
}

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
}

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
