declare module "stdio" {
    let stdout: Writable;
    let stderr: Writable;
    let stdin: Readable;
}

declare const console: {
    debug(arg: any): void;
    log(arg: any): void;
    warn(arg: any): void;
    info(arg: any): void;
    error(arg: any): void;
}
