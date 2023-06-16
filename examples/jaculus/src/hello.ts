import { getLocalRofi} from "rofi";
let rofi = getLocalRofi();

setInterval(() => {
	console.log("Random number" + rofi.getRandomNumber());
}, 1000);