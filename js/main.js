window.addEventListener("load", function () {
	var chart;
	var selected = d3.select("#chart svg")
	nv.addGraph(function (){ 

		chart = nv.models.lineChart();
		
		chart.xAxis
		.axisLabel('Frequency (Hz)')
		.tickFormat(d3.format(',r'));

		chart.yAxis
		.axisLabel('Magnitude')
		.tickFormat(d3.format('.02f'));

		chart.forceY([0, 150]);

		nv.utils.windowResize(function() { selected.call(chart); });
	})


	var host = "ws://localhost:7681/";
	try {
		var socket = new WebSocket(host);

		socket.onopen = function (openEvent) {
			console.log("Connected to server");
			console.log(openEvent);
		};

		socket.onmessage = function (messageEvent) {
			var object = JSON.parse(messageEvent.data);
			//console.log(object);
			selected.datum([{ 
				values : object,
				color: '#0000FF',
				key: 'Wave'
			}])
			//.transition()
			//.duration(10)
			.call(chart);
		};

		 socket.onerror = function (errorEvent) {
		 	console.log('WebSocket Status:: Error was reported');
		 	console.log(errorEvent);
		 };

		 socket.onclose = function (closeEvent) {
		 	console.log( 'WebSocket Status:: Socket Closed');
		 	console.log(closeEvent);
		 };
	} catch (exception) { console.log(exception); }
});

