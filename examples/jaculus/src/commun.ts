import { getLocalRofi} from "rofi";
let rofi = getLocalRofi();

setInterval(() => {
	console.log("Random number from ESP32: " + rofi.getRandomNumber());
}, 1000);