declare module "rofi" {
    function getLocalRofi(): Rofi;

    class Rofi {
        getId(): number;
        getJoint(index: number): Joint;
        getConnector(index: number): Connector;
        reboot(): void;
    }

    class Joint {
        public getVelocity(): number;
        public setVelocity(velocity: number): void;

        public getPosition(): number;
        public setPosition(position: number): void;

        public maxPosition(): number;
        public setMaxPosition(maxPosition: number): void;

        public getTorque(): number;
        public setTorque(torque: number): void;

        public onError(callback: () => void): void;
    }

    class Connector {
        public connect(): void;
        public disconnect(): void;
    }
}
