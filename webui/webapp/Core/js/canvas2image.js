function canvas2image_findElementByRWTId(id, tagName)
{
	console.log("test");
	var elements = document.getElementsByTagName(tagName);

	for(i = 0; i < elements.length; i++)
	{
		if (elements[i].rwtObject._control._rwtId != null && elements[i].rwtObject._control._rwtId == id)
		{
			console.log("match!");
			return elements[i];
		}
	}
}

function canvas2image_drawImageFromChart(chartRoot, background)
{
	if (chartRoot.rwtObject._control._children != null)
	{
		var children = chartRoot.rwtObject._control._children;
	}

	console.log(children);
	var canvas = document.createElement("canvas");
	canvas.height = chartRoot.height;
	canvas.width = chartRoot.width;

	var context = canvas.getContext("2d");
	context.fillStyle = background;
	context.fillRect(0, 0, canvas.width, canvas.height);

	context.drawImage(chartRoot, 0,0);

	for(i = children.length-1; i >= 0; i--)
	{
		console.log(children[i]._element.firstChild);
		if ((children[i]._computedWidthValue > 0) && (children[i]._computedHeightValue > 0))
		{
			context.drawImage(children[i]._element.firstChild, children[i]._computedLeftValue, children[i]._computedTopValue);
		}	
	}
	download(canvas.toDataURL(), "graph.png", "image/png");
}

function canvas2image_convert(id, tagName, background)
{
	canvas2image_drawImageFromChart(canvas2image_findElementByRWTId(id, tagName), background);
}
