import {getLocalRofi, Connector} from 'rofi'

let rofi = getLocalRofi();
let connector = rofi.getConnector(0);

console.log("Starting connector");


setInterval(() => {
	console.log("Connector 0 state: " + JSON.stringify(connector.getState(), null, 2))
}, 5000);

let connected: boolean = false;
setInterval(() => {
	console.log("Toggling connector 0 to: " + connected);

	if(connected) {
		connector.disconnect();
	} else {
		connector.connect();
	}
	connected = !connected;
}, 5000);
