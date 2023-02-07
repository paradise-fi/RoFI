use std::ops::{Neg, Range};

pub struct NegableRange<T>(pub Range<T>);

impl<T> From<NegableRange<T>> for Range<T> {
    fn from(range: NegableRange<T>) -> Self {
        let NegableRange(range) = range;
        range
    }
}

impl<T: num::Signed + num::Integer> Neg for NegableRange<T> {
    type Output = Self;
    fn neg(self) -> Self::Output {
        let Self(Range { start, end }) = self;
        Self(Range {
            start: -(end - num::one()),
            end: -start + num::one(),
        })
    }
}
