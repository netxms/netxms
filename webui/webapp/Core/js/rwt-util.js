function RWTUtil_findElementById(id, tagName)
{
   var elements = document.getElementsByTagName((tagName != null) ? tagName : 'div');
   for(i = 0; i < elements.length; i++)
   {
      if (elements[i].rwtWidget != null && elements[i].rwtWidget._rwtId == id)
      {
         return elements[i];
      }
   }
   return null;
}

function RWTUtil_widgetToImage(id, tagName, fileName)
{
   var domNode = RWTUtil_findElementById(id, tagName);
   domtoimage.toPng(domNode).then(function (dataUrl) {
      download(dataUrl, fileName, "image/png");
   });
}

function RWTUtil_widgetToClipboard(id, tagName)
{
   if (navigator.clipboard)
   {
      var domNode = RWTUtil_findElementById(id, tagName);
      domtoimage.toBlob(domNode).then(function (blob) {
         navigator.clipboard.write([new ClipboardItem({ 'image/png': blob })]);
      });
   }
}

