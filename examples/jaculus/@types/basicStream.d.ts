declare interface Writable {
    /**
     * Write the given data to the stream.
     * @param data The data to write.
     */
    write(data: string): void;
}

declare interface Readable {
    /**
     * Read a single character from the stream.
     * @returns Promise that resolves to the character read.
     */
    get(): Promise<string>;

    /**
     * Read a chunk of data from the stream. The size of the chunk is
     * given by the implementation and available data.
     * @returns Promise that resolves to the data read.
     */
    read(): Promise<string>;
}
