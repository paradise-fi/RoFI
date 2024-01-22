import {getLocalRofi, Connector} from 'rofi'

let rofi = getLocalRofi();

console.log("Starting descriptor");

setInterval(() => {
	console.log("Descriptor: " + JSON.stringify(rofi.getDescriptor(), null, 2))
}, 5000);