/*** Global variables ***/
var selectedAlarms = '';

/*** Handler for alarm selection change ***/
function onAlarmSelect(ctrlId, alarmId, isSelected)
{
	if (isSelected)
	{
		if (selectedAlarms.indexOf('(' + alarmId + ')') == -1)
		{
			selectedAlarms += '(' + alarmId + ')';
		}
	}
	else
	{
		var str = '(' + alarmId + ')';
		var idx = selectedAlarms.indexOf(str);
		if (idx != -1)
		{
			var s1 = (idx > 0) ? selectedAlarms.substring(0, idx) : '';
			var s2 = selectedAlarms.substring(idx + str.length);
			selectedAlarms = s1 + s2;
		}
	}
}

/*** Handler for alarm list buttons ***/
function alarmButtonHandler(sid, button)
{
	loadDivContent('alarm_list', sid, '&cmd=alarmCtrl&action=' + button + '&alarmList=' + selectedAlarms);
	selectedAlarms = '';
}

/*** Handler for single alarm acknowledge/terminate ***/
function alarmCtrl(sid, alarmId, action)
{
	selectedAlarms = '';
	loadDivContent('alarm_list', sid, '&cmd=alarmCtrl&action=' + action + '&alarmList=(' + alarmId + ')');
}
