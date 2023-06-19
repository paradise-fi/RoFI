export class Angle {
    public static readonly PI = 3.141592653589793238462643383279502884;

    private _rads: number;

    private constructor(rads: number) {
        this._rads = rads;
    }

    public static rad(rads: number): Angle {
        return new Angle(rads);
    }

    public static deg(degs: number): Angle {
        return new Angle(this.PI * degs / 180);
    }

    public add(other: Angle): Angle {
        this._rads += other._rads;
        return this;
    }

    public subtract(other: Angle): Angle {
        this._rads -= other._rads;
        return this;
    }

    public multiply(value: number): Angle {
        this._rads *= value;
        return this;
    }

    public divide(value: number): Angle {
        this._rads /= value;
        return this;
    }

    public equals(other: Angle): boolean {
        return this._rads === other._rads;
    }

    public deg(): number {
        return 180 * this._rads / Angle.PI;
    }

    public rad(): number {
        return this._rads;
    }
}

// Helper functions
export function negative(angle: Angle): Angle {
    return Angle.rad(-angle.rad());
}

export function add(lhs: Angle, rhs: Angle): Angle {
    return lhs.add(rhs);
}

export function subtract(lhs: Angle, rhs: Angle): Angle {
    return lhs.subtract(rhs);
}

export function multiply(angle: Angle, value: number): Angle {
    return angle.multiply(value);
}

export function divide(angle: Angle, value: number): Angle {
    return angle.divide(value);
}

export function clamp(value: Angle, min: Angle, max: Angle): Angle {
    const rads = Math.min(Math.max(value.rad(), min.rad()), max.rad());
    return Angle.rad(rads);
}
