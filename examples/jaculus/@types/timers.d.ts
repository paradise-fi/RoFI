/**
 * Returns a promise that resolves after the specified time.
 * @param ms The number of milliseconds to wait before resolving the promise.
 */
declare function sleep(ms: number): Promise<void>;

/**
 * Calls a function after the specified time.
 * @param callback The function to call.
 * @param ms The number of milliseconds to wait before calling the function.
 */
declare function setTimeout(callback: () => void, ms: number): number;

/**
 * Calls a function repeatedly, with a fixed time delay between each call.
 * @param callback The function to call.
 * @param ms The number of milliseconds to wait before calling the function.
 */
declare function setInterval(callback: () => void, ms: number): number;

/**
 * Cancels a timeout previously established by calling setTimeout().
 * @param id The identifier of the timeout to cancel.
 */
declare function clearTimeout(id: number): void;

/**
 * Cancels a timeout previously established by calling setInterval().
 * @param id The identifier of the interval to cancel.
 */
declare function clearInterval(id: number): void;
