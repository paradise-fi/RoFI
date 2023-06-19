import { getLocalRofi, Joint} from "rofi";
import {Angle} from "./angle.js"

let rofi = getLocalRofi();
class ServoMover {
	private servoId: number;
	private joint: Joint;
	private angle: Angle = Angle.deg(-90);

	constructor(servoId: number) {
		this.servoId = servoId;
		this.joint = rofi.getJoint(servoId);
	}

	tick() {
		this.joint.setPosition(this.angle.rad(), this.joint.maxSpeed(), () => {});
		this.angle.add(Angle.deg(10));
		if(this.angle.deg() > 90) {
			this.angle = Angle.deg(-90);
		}
	}

	showInfo() {
		console.log("Servo " + this.servoId + " info:");
		console.log("Max speed: " + this.roundAngleSpeed(this.joint.maxSpeed()));
		console.log("Min speed: " + this.roundAngleSpeed(this.joint.minSpeed()));
		console.log("Max torque: " + this.roundTorque(this.joint.maxTorque()));
		console.log("Max position: " + this.roundAngleRadians(this.joint.maxPosition()) + " | " + this.roundAngleDegrees(this.joint.maxPosition()));
		console.log("Min position: " + this.roundAngleRadians(this.joint.minPosition()) + " | " + this.roundAngleDegrees(this.joint.minPosition()));
		console.log("Current position: " + this.roundAngleRadians(this.joint.getPosition()) + " | " + this.roundAngleDegrees(this.joint.getPosition()));
		console.log("Current velocity: " + this.joint.getVelocity() + " rad / s");
		console.log("Current torque: " + this.roundTorque(this.joint.getTorque()));
		console.log("");
	}

	roundAngleSpeed(angle: number): string {
		return angle.toPrecision(2) + " rad / s"
	}

	roundAngleRadians(angleRad: number): string {
		return angleRad.toPrecision(2) + " rad"
	}

	roundAngleDegrees(angleRad: number): string {
		return Angle.rad(angleRad).deg().toPrecision(2) + " deg"
	}

	roundTorque(angle: number): string {
		return angle.toPrecision(2) + " N * m"
	}
}

let servo0 = new ServoMover(0);
let servo1 = new ServoMover(1);
let servo2 = new ServoMover(2);

setInterval(() => {
	servo0.tick();
	// console.log("Moving servo 0");
}, 1000);

setInterval(() => {
	servo1.tick();
	// console.log("Moving servo 1");
}, 300);


setInterval(() => {
	servo2.tick();
	// console.log("Moving servo 2");
}, 300);

setInterval(() => {
	servo0.showInfo();
}, 5000);