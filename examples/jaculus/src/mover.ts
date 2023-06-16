import { getLocalRofi} from "rofi";
let rofi = getLocalRofi();

setInterval(() => {
	console.log("Random number" + rofi.getRandomNumber());
}, 1000);



class ServoMover {
	private servoId: number;
	private position: number = -90;

	constructor(servoId: number) {
		this.servoId = servoId;
	}

	tick() {
		rofi.getJoint(this.servoId).setPosition(this.position);
		this.position += 20;
		if(this.position > 90) {
			this.position = -90;
		}
	}
}

let servo0 = new ServoMover(0);
let servo1 = new ServoMover(1);

setInterval(() => {
	servo0.tick();
}, 300);

setInterval(() => {
	servo1.tick();
}, 300);
