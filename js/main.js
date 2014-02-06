window.addEventListener("load", function () {
	var host = "ws://localhost:7681/";
	try {
		var socket = new WebSocket(host);

		socket.onopen = function (openEvent) {
			console.log("Connected to server");
			console.log(openEvent);
		};

		socket.onmessage = function (messageEvent) {
			var object = JSON.parse(messageEvent.data);
			console.log(object);
			graph.series[0].data = object;
			console.log("updated");
			window.requestAnimationFrame(function () {graph.render();});
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

	window.graph = new Rickshaw.Graph( {
		element: document.querySelector("#chart"), 
		width: 1920, 
		height: 1080, 
		series: [{
			color: 'steelblue',
			data: [ 
			{ x: 0, y: 40 }, 
			{ x: 1, y: 49 }, 
			{ x: 2, y: 38 }, 
			{ x: 3, y: 30 }, 
			{ x: 4, y: 32 } ]
		}]
	});

	graph.render();
});

