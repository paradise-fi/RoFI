declare module "fs" {
    interface File {
        path: string;

        /**
         * Check if the file is open.
         */
        isOpen(): boolean;

        /**
         * Close the file.
         */
        close(): void;

        /**
         * Read characters from the file.
         * @param len The number of characters to read.
         */
        read(len: number): string;

        /**
         * Write text to the file.
         * @param text The text to write.
         */
        write(text: string): void;
    }

    /**
     * Open the given file in the given mode.
     * @param path The path to the file.
     * @param mode The mode to open the file in ("r", "w", "a" and combinations).
     */
    function open(path: string, mode: string): File;

    /**
     * Check if the given path exists.
     * @param path The path to check.
     * @returns True if the path exists, false otherwise.
     */
    function exists(path: string): boolean;

    /**
     * Check if the given path is a file.
     * @param path The path to check.
     * @returns True if the path is a file, false otherwise.
     */
    function isFile(path: string): boolean;

    /**
     * Check if the given path is a directory.
     * @param path The path to check.
     * @returns True if the path is a directory, false otherwise.
     */
    function isDirectory(path: string): boolean;

    /**
     * Create a directory at the given path.
     * @param path The path to create the directory at.
     */
    function mkdir(path: string): void;

    /**
     * Remove the file at the given path.
     * @param path The path to the file to remove.
     */
    function rm(path: string): void;

    /**
     * Remove the directory at the given path.
     * @param path The path to the directory to remove.
     */
    function rmdir(path: string): void;

    /**
     * List the files in the given directory.
     * @param path The path to the directory to list.
     * @returns An array of file names in the directory.
     */
    function readdir(path: string): string[];
}
