function changeDivHeight()
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

	var clArea = document.getElementById('object_tree');
	if (clArea != null)
	{
		var t = clArea.offsetTop;
		//var b = document.body.scrollHeight - document.getElementById('footer').offsetTop;
		var h = winHeight - clArea.offsetTop;
		clArea.style.height = h + "px";
	}
}

if (window.addEventListener)
{
	window.addEventListener("load", changeDivHeight, false);
	window.addEventListener("resize", changeDivHeight, false);
}
else if (window.attachEvent)
{
	window.attachEvent("onload", changeDivHeight);
	window.attachEvent("onresize", changeDivHeight);
}
