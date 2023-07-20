declare module "rofi" {
    function getLocalRofi(): Rofi;

    interface Descriptor {
        jointCount: number;
        connectorCount: number;
    }

    class Rofi {
        /**
         * Get RoFI Id.
         * @return RoFI Id
         */
        getId(): number;

        /**
         * Get random number to test communication.
         * @return Random number
         */
        getRandomNumber(): number;

        /**
         * Get proxy for controlling Joint.
         * @param index index of the Joint
         * @return Proxy for controlling Joint
         */
        getJoint(index: number): Joint;

        /**
         * Get proxy for controlling Connector.
         * @param index index of the Connector
         * @return Proxy for controlling Connector
         */
        getConnector(index: number): Connector;

        /**
         * Get RoFI descriptor.
         * @return RoFI descriptor
         */
        getDescriptor(): Descriptor;

        /**
         * Reboot device
         */
        reboot(): void;
    }

    type JointError = "Communication" | "Hardware";

    class Joint {

        /**
         * Get maximal joint position.
         * @return maximal joint position in rad
         */
        public maxPosition(): number;

        /**
         * Get minimal joint position.
         * @return minimal joint position in rad
         */
        public minPosition(): number;

        /**
         * Get maximal joint speed.
         * @return maximal joint speed in rad / s
         */
        public maxSpeed(): number;

        /**
         * Get minimal joint speed.
         * @return minimal joint speed in rad / s
         */
        public minSpeed(): number;

        /**
         * Get maximal joint torque.
         * @return maximal joint torque in N * m
         */
        public maxTorque(): number;


        /**
         * Get current joint velocity setpoint.
         * @return current joint velocity in rad / s
         */
        public getVelocity(): number;

        /**
         * Change the joint's control mode to velocity and move at velocity.
         * @param velocity velocity setpoint in rad / s
         */
        public setVelocity(velocity: number): void;


        /**
         * Get current joint position.
         * @return current position in rad
         */
        public getPosition(): number;

        /**
         * Change the joint's control mode to position and move to position.\
         *
         * Calling any joint control method  (`Joint::setPosition`,
         * `Joint::setVelocity` or `Joint::setTorque`) before the position is
         * reached will result in the \p callback never beeing called.
         * @param position position setpoint in rad
         * @param velocity velocity limit in rad / s, required to be positive and non-zero
         * @param callback callback to be called once the position is reached
         */
        public setPosition(position: number, velocity: number, callback: () => void): void;


        /**
         * Get current joint torque.
         * @return current torque in N * m
         */
        public getTorque(): number;

        /**
         * Change the joint's control mode to torque and provide torque.
         * @param torque torque setpoint in N * m
         */
        public setTorque(torque: number): void;


        /**
         * Set error handling callback.
         *
         * Calling this method again (on the same `Joint`) will overwrite
         * the previous `callback`.
         * You can call this method with empty function to remove the `callback`.
         * @param callback callback to be called on error
         */
        public onError(callback: (error: JointError, message: string) => void): void;


        /**
         * Test servo move
         * @return void
        */
        public testMove(): void;
    }

    type ConnectorPosition = "Retracted" | "Extended";
    type ConnectorOrientation = "North" | "South" | "East" | "West";
    type ConnectorLine = "Internal" | "External";
    type LidarDistanceMode = "Autonomous" | "Short" | "Long";
    type LidarStatus = "Error" | "NotMesured" | "OutsideRange" | "Valid";
    type ConnectorEvent = "Connect" | "Disconnected" | "PowerChanged";

    interface ConnectorState {
        /**
         * Position of the Connector.
         */
        position: ConnectorPosition,
        /**
         * Is internal power bus connected to the connector?
         */
        internal: boolean

        /**
         * Is internal power bus connected to the connector
         */
        external: boolean

        /**
         * The current distance mode of the RoFICoM's Lidar.
         */
        distanceMode: LidarDistanceMode

        /**
         * Is there a mating side connected?
         */
        connected: boolean

        /**
         * Orientation of the connection. Applicable only when connected.
         */
        orientation: ConnectorOrientation

        /**
         * Internal voltage of the Connector.
         */
        internalVoltage: number

        /**
         * Internal current of the Connector.
         */
        internalCurrent: number

        /**
         * External voltage of the Connector.
         */
        externalVoltage: number

        /**
         * External current of the Connector.
         */
        externalCurrent: number

        /**
         * Status of lidar measurement.
         */
        lidarStatus: LidarStatus

        /**
         * Lidar measured distance in mm. Validity depends on `lidarStatus`.
         */
        distance: number
    }

    class Connector {
        /**
         * Get connector state.
         * @return connector state
         */
        public getState(): ConnectorState;

        /**
         * Extend the connector to be ready to accept connection.
         *
         * This action does not establish connection.
         * To react to connections, register a callback via Connector::onConnectorEvent().
         */
        public connect(): void;

        /**
         * Retract the connector.
         *
         * Does not release a connection immediately.
         * After the connection is broken physically, calls callback registered via
         * Connector::onConnectorEvent()
         */
        public disconnect(): void;

        /**
         * Register callback for connector events.
         *
         * Calling this method again (on the same `Connector`) will overwrite the previous `callback`.
         * You can call this method with empty function to remove the `callback`.
         * @param callback callback to be called on connector events
         */
        public onConnectorEvent(callback: (event: ConnectorEvent) => void): void;

        /**
         * Connect power of mating side to a power line.
         * @param line power line to connect to
         */
        public connectPower(line: ConnectorLine): void;

        /**
         * Disconnect power of mating side from a power line.
         * @param line power line to disconnect from
         */
        public disconnectPower(line: ConnectorLine): void;

        /**
         * Sets distance mode for the RoFICoM's Lidar.
         * @param mode distance mode
         */
        public setDistanceMode(mode: LidarDistanceMode): void;
    }
}
