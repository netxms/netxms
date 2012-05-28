qx.Class.define("org.netxms.ui.eclipse.charts.widgets.LineChart", {
    extend: qx.ui.layout.CanvasLayout,

	construct: function(id) {
		this.base(arguments);
		this.setHtmlAttribute("id", id);
		this._id = id;
		this._plot = null;
		
		this._defaultColors = [
                "rgb(64,105,156)",
                "rgb(158,65,62)",
                "rgb(127,154,72)",
                "rgb(105,81,133)",
                "rgb(60,141,163)",
                "rgb(204,123,56)",
                "rgb(79,129,189)",
                "rgb(192,80,77)",
                "rgb(155,187,89)",
                "rgb(128,100,162)",
                "rgb(75,172,198)",
                "rgb(247,150,70)",
                "rgb(170,186,215)",
                "rgb(217,170,169)",
                "rgb(198,214,172)",
                "rgb(186,176,201)"
		];
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
				this._plot.getOptions().grid.borderWidth = 1;
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
        	//alert(this.getLegend());
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
				plotData.push({ label : arguments[0][i - 1], data : dciPoints, color: this._defaultColors[i - 1] });
			}
			//console.log(arguments);
			//console.log(plotData);
			
			try {
	    		$.plot($("#" + this._id), plotData, { xaxis: { mode: "time" } });
	    	}
	    	catch(e) {
	    	}

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
