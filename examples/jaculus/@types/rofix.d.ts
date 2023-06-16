declare module "rofix" {
    /**
     * Setup velocity
     * @param velocity The velocity to set the motor to.
     */
    function setVelocity(velocity: number) : void;

    /**
     * Get the velocity
     * @returns The velocity of the motor.
     */
    function getVelocity(): number;
}
