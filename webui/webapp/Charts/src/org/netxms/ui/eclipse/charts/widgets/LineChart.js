qx.Class.define("org.netxms.ui.eclipse.charts.widgets.LineChart", {
    //extend: qx.ui.layout.CanvasLayout,
    extend: org.eclipse.swt.widgets.Composite,

	construct: function(id) {
		this.base(arguments);
		this.setHtmlAttribute("id", id);
		this._id = id;
		this._plot = null;
		this._title = "Chart";
		this._bgColor = "#ffffff";
		
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
		titleVisible : {
			"check" : "Boolean",
			"apply" : "_applyTitleVisible"
		},
		title : {
			"apply" : "_applyTitle"
		},
		bgColor : {
			"apply" : "_applyBgColor"
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
        
        _applyTitleVisible : function() {
			qx.ui.core.Widget.flushGlobalQueues();
			this._titleVisible = this.getTitleVisible();
        },

        _applyTitle : function() {
			qx.ui.core.Widget.flushGlobalQueues();
			this._title = this.getTitle();
        },

        _applyBgColor : function() {
			qx.ui.core.Widget.flushGlobalQueues();
			console.debug('111');
			this._bgColor = this.getBgColor();
			console.debug('222');
        },
        
		update : function() {
			qx.ui.core.Widget.flushGlobalQueues();
			var plotData = [];
			
			for (var i = 2; i < arguments.length; i++) {
				elements = arguments[i].split("|");
				var dciPoints = [];
				for (var j = 0; j < elements.length; j += 2) {
					var point = [elements[j], elements[j + 1]];
					dciPoints.push(point);
				}
				plotData.push({ label : arguments[0][i - 2], data : dciPoints, color: arguments[1][i - 2] });
			}
			//console.log(arguments);
			//console.log(plotData);
			
			try {
				$("#" + this._id).empty();
				$("#" + this._id).append('<div style="font-size: -1; text-align: center; background-color: ' + this._bgColor + '; padding: 0px;" id="_title_' + this._id + '" /><div style="width: 100%; height: 100%; padding: 0px;background-color: ' + this._bgColor + '" id="_chart_' + this._id + '" />');
				if (this._titleVisible) {
					$("#_title_" + this._id).show();
				}
				else {
					$("#_title_" + this._id).hide();
				}

				console.debug($("#" + this._id));
			
				$("#_title_" + this._id).text(this._title);
	    		$.plot($("#_chart_" + this._id), plotData, { xaxis: { mode: "time" } });
				console.debug('All Done');
	    	}
	    	catch(e) {
	    		console.error(e);
	    	}

        },
    },

});
