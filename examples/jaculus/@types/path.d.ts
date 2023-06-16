declare module "path" {
    /**
     * Normalize the given path.
     * @param path The path to normalize.
     * @returns The normalized path.
     */
    function normalize(path: string): string;

    /**
     * Get the parent directory of the given path.
     * @param path The path to get the parent directory of.
     * @returns The parent directory.
     */
    function dirname(path: string): string;

    /**
     * Get the last part of the given path.
     * @param path The path to get the last part of.
     * @returns The last part of the path.
     */
    function basename(path: string): string;

    /**
     * Join the given paths.
     * @param paths The paths to join.
     * @returns The joined path.
     */
    function join(...paths: string[]): string;

    /**
     * Check if the given path is absolute.
     * @param path The path to check.
     * @returns True if the path is absolute, false otherwise.
     */
    function isAbsolute(path: string): boolean;
}
