qx.Class.define("org.netxms.ui.eclipse.charts.widgets.LineChart", {
    extend: qx.ui.layout.CanvasLayout,

	construct: function(id) {
		this.base(arguments);
		this.setHtmlAttribute("id", id);
		this._id = id;
		this._plot = null;
	},
	
	properties : {
		gridVisible : {
			"check" : "Boolean",
			"apply" : "_applyGridVisible"
		},
		legendVisible : {
			"check" : "Boolean",
			"apply" : "_applyLegendVisible"
		},
		legend : {
			"check" : "Array",
			"apply" : "_applyLegend"
		},
	},

    members : {
        _applyGridVisible : function() {
			qx.ui.core.Widget.flushGlobalQueues();

			if (this._plot != null) {
				this._plot.getOptions().grid.show = this.getGridVisible();
    	    	this._plot.draw();
			}
        },

        _applyLegendVisible : function() {
			qx.ui.core.Widget.flushGlobalQueues();

			if (this._plot != null) {
				this._plot.getOptions().legend.show = this.getLegendVisible();
    	    	this._plot.draw();
			}
        },

        _applyLegend : function() {
        	alert(this.getLegend());
        },
        
		update : function() {
			qx.ui.core.Widget.flushGlobalQueues();
			
			var plotData = [];
			for (var i = 1; i < arguments.length; i++) {
				elements = arguments[i].split("|");
				var dciPoints = [];
				for (var j = 0; j < elements.length; j += 2) {
					var point = [elements[j], elements[j + 1]];
					dciPoints.push(point);
				}
				plotData.push({ label : arguments[0][i - 1], data : dciPoints });
			}
			//console.log(arguments);
			//console.log(plotData);
			
	    	$.plot($("#" + this._id), plotData, { xaxis: { mode: "time" } });

			//$.plot($("#" + this._id), plotData);
			//plot = this._getPlot();
			//plot.setData(plotData);
			//plot.setupGrid();
			//plot.draw();
			
			//var labels = arguments[0];
			//var data = arguments[1];
			
			//alert(data[0][0]);
			//alert(data[0][1]);
			//alert(data[1][0]);
			//alert(data[1][1]);
			
			//plot = this._getPlot()
			//plot.setData([[0, 3], [4, 8], [8, 5], [9, 13]]);
			//plot.setupGrid();
			//plot.draw();
        },
    },

});
