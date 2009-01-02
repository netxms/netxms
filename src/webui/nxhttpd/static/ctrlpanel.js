/**
 ** Server variables
**/

function editServerVar(sid, varName, currValue)
{
	loadDivContent('ctrlpanel_view', sid, '&cmd=ctrlPanelView&view=0&edit=' + escape(varName) + '&value=' + escape(currValue));
	resetCheckboxStates();
	resizeElements();
}

function newServerVar(sid)
{
	loadDivContent('ctrlpanel_view', sid, '&cmd=ctrlPanelView&view=0&edit=!new!');
	resetCheckboxStates();
	resizeElements();
}

function deleteServerVar(sid)
{
	var varList = '';
	for (item in checkBoxState)
	{
		if (checkBoxState[item])
			varList += '*' + item.length + '.' + escape(item);
	}
	loadDivContent('ctrlpanel_view', sid, '&cmd=ctrlPanelView&view=0&delete=' + varList);
	resetCheckboxStates();
	resizeElements();
}

function editVarButtonHandler(sid, button)
{
	var s = '';
	if (button == 'ok')
	{
		s = '&update=' + escape(document.forms.formCfgVar.varName.value) + '&value=' + escape(document.forms.formCfgVar.varValue.value);
	}
	loadDivContent('ctrlpanel_view', sid, '&cmd=ctrlPanelView&view=0' + s);
	resizeElements();
}
