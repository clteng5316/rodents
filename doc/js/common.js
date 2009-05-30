// common module

var onLoadListeners = Array();

function addLoadListener(listener)
{
	if (window.addEventListener)
	{
		window.addEventListener('load', listener, false);
	}
	else
	{
		onLoadListeners.push(listener);
		if (!window.onload)
		{
			window.onload = function()
			{
				for (var i = 0; i < onLoadListeners.length; i++)
					onLoadListeners[i]()
			}
		}
	}
}

// table module

function compareString(a, b)
{
	var n = a[0].toLowerCase();
	var m = b[0].toLowerCase();
	return n == m ? 0 : (n < m ? -1 : 1);	// default is asc
}

function tablePaint(row, index)
{
	row.className = (index % 2 == 0 ? 'even' : 'odd');
}

function tableClick(table, column, compare)
{
	var head = table.tHead.rows[0].cells;
	var body = table.tBodies[0];
	var rows = [];
	for (var r = 0; r < body.rows.length; r++)
	{
		var cell = body.rows[r].cells[column];
		rows.push([cell.innerHTML, body.rows[r]]);
	}
	rows.sort(compare);

	for (var c = 0; c < head.length; c++)
	{
		var cell = head[c];
		if (c != column)
			cell.className = '';
		else if (cell.className != 'asc')
			cell.className = 'asc';
		else
		{
			cell.className = 'desc';
			rows.reverse();
		}
	}

	for (r = 0; r < rows.length; r++)
	{
		tablePaint(rows[r][1], r);
		body.appendChild(rows[r][1]);
	}
};

function tableInit()
{
	function connect(cell, table, column, compare)
	{
		cell.onclick = function() { tableClick(table, column, compare); };
	}
	var tables = document.getElementsByTagName('table');
	for (var i = 0; i < tables.length; i++)
	{
		var table = tables[i];
		if (table.tBodies.length <= 0)
			continue;
		if (!table.tHead)
			continue;
		for (var r = 0; r < table.tBodies[0].rows.length; r++)
			tablePaint(table.tBodies[0].rows[r], r);
		var head = table.tHead.rows[0].cells;
		for (var c = 0; c < head.length; c++)
			connect(head[c], table, c, compareString);
	}
};

addLoadListener(tableInit);
