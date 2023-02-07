pub mod ord;

use std::ops::Range;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Pos<IndexType: num::Num> {
    pub x: IndexType,
    pub y: IndexType,
    pub z: IndexType,
}

/// Deliberately choosing not to use lexicographical comparison
/// to prevent bugs of type `max(point1, point2)`
impl<TIndex: num::Num + PartialOrd> PartialOrd for Pos<TIndex> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        let mut last_ord = std::cmp::Ordering::Equal;
        for (lhs, rhs) in [
            (&self.x, &other.x),
            (&self.y, &other.y),
            (&self.z, &other.z),
        ] {
            match lhs.partial_cmp(rhs) {
                None => return None,
                Some(std::cmp::Ordering::Equal) => {}
                Some(ord) if last_ord == std::cmp::Ordering::Equal => last_ord = ord,
                Some(ord) if last_ord == ord => {}
                Some(_) => return None,
            }
        }
        Some(last_ord)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Hash)]
pub struct Sizes<IndexType: num::Num + Ord>(Pos<IndexType>);

impl<TIndex: num::Num + Ord> Sizes<TIndex> {
    pub fn new(sizes: Pos<TIndex>) -> Self {
        let sizes = sizes.as_array();
        if sizes.iter().all(|pos| pos > &num::zero()) || sizes.iter().all(TIndex::is_zero) {
            Self(sizes.into())
        } else if sizes.iter().any(|pos| pos < &num::zero()) {
            panic!("Size cannot be negative")
        } else {
            panic!("Size cannot be mixed positive and zero")
        }
    }

    pub fn get(self) -> Pos<TIndex> {
        self.0
    }

    pub fn empty() -> Self {
        Self([num::zero(), num::zero(), num::zero()].into())
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct SizeRanges<IndexType: num::Num + Ord> {
    start: Pos<IndexType>,
    end: Pos<IndexType>,
}

impl<TIndex: num::Num> From<[TIndex; 3]> for Pos<TIndex> {
    fn from(value: [TIndex; 3]) -> Self {
        let [x, y, z] = value;
        Self { x, y, z }
    }
}
impl<TIndex: num::Num> From<Pos<TIndex>> for [TIndex; 3] {
    fn from(value: Pos<TIndex>) -> Self {
        let Pos { x, y, z } = value;
        [x, y, z]
    }
}
impl<'a, TIndex: num::Num> From<&'a Pos<TIndex>> for [&'a TIndex; 3] {
    fn from(value: &'a Pos<TIndex>) -> Self {
        let Pos { x, y, z } = value;
        [x, y, z]
    }
}

impl<TIndex: num::Num> std::ops::Add for Pos<TIndex> {
    type Output = Self;
    fn add(self, rhs: Self) -> Self::Output {
        let lhs = self.as_array();
        let rhs = rhs.as_array();
        lhs.zip(rhs).map(|(lhs, rhs)| lhs + rhs).into()
    }
}
impl<TIndex: num::Signed> std::ops::Sub for Pos<TIndex> {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self::Output {
        let lhs = self.as_array();
        let rhs = rhs.as_array();
        lhs.zip(rhs).map(|(lhs, rhs)| lhs - rhs).into()
    }
}
impl<TIndex: num::Signed> std::ops::Neg for Pos<TIndex> {
    type Output = Self;
    fn neg(self) -> Self::Output {
        self.as_array().map(|pos| -pos).into()
    }
}

impl<TIndex: num::Num> Pos<TIndex> {
    pub fn as_array(self) -> [TIndex; 3] {
        self.into()
    }
    pub fn as_array_ref(&self) -> [&TIndex; 3] {
        self.into()
    }
    pub fn is_all_zeroes(&self) -> bool {
        let Self { x, y, z } = self;
        x.is_zero() && y.is_zero() && z.is_zero()
    }
}

impl<TIndex: num::Num + Ord> SizeRanges<TIndex> {
    pub fn new(start: Pos<TIndex>, end: Pos<TIndex>) -> Self {
        let start = start.as_array();
        let end = end.as_array();
        if start.iter().zip(&end).all(|(start, end)| start < end) || (start == end) {
            Self {
                start: start.into(),
                end: end.into(),
            }
        } else if start.iter().zip(&end).any(|(start, end)| start > end) {
            panic!("start cannot be bigger than end")
        } else {
            panic!("`end-start` have to be all positive or all zero")
        }
    }

    pub fn from_sizes(sizes: Sizes<TIndex>) -> Self {
        Self {
            start: Sizes::empty().get(),
            end: sizes.get(),
        }
    }
    pub fn empty() -> Self {
        Self::from_sizes(Sizes::empty())
    }

    pub fn start_end(self) -> (Pos<TIndex>, Pos<TIndex>) {
        (self.start, self.end)
    }
    pub fn start(self) -> Pos<TIndex> {
        self.start
    }
    pub fn end(self) -> Pos<TIndex> {
        self.end
    }

    pub fn x_range(&self) -> Range<TIndex>
    where
        TIndex: Copy,
    {
        self.start.x..self.end.x
    }
    pub fn y_range(&self) -> Range<TIndex>
    where
        TIndex: Copy,
    {
        self.start.y..self.end.y
    }
    pub fn z_range(&self) -> Range<TIndex>
    where
        TIndex: Copy,
    {
        self.start.z..self.end.z
    }

    pub fn as_ranges_array(self) -> [Range<TIndex>; 3] {
        self.start
            .as_array()
            .zip(self.end.as_array())
            .map(|(start, end)| start..end)
    }
    pub fn from_ranges_array(ranges_array: [Range<TIndex>; 3]) -> Self
    where
        TIndex: Copy,
    {
        let ranges_array = ranges_array.map(|range| (range.start, range.end));
        Self {
            start: ranges_array.map(|(start, _end)| start).into(),
            end: ranges_array.map(|(_start, end)| end).into(),
        }
    }
}
pub fn minimal_pos_hull<TIndex, IPos>(positions: IPos) -> SizeRanges<TIndex>
where
    TIndex: num::Num + Ord + Copy,
    IPos: IntoIterator<Item = Pos<TIndex>>,
{
    positions
        .into_iter()
        .map(|pos| {
            let pos = pos.as_array();
            (pos, pos.map(|pos| pos + TIndex::one()))
        })
        .reduce(|acc, pos| {
            (
                acc.0.zip(pos.0).map(|(acc, pos)| std::cmp::min(acc, pos)),
                acc.1.zip(pos.1).map(|(acc, pos)| std::cmp::max(acc, pos)),
            )
        })
        .map(|(start, end)| SizeRanges {
            start: start.into(),
            end: end.into(),
        })
        .unwrap_or_else(SizeRanges::empty)
}

#[cfg(test)]
mod tests {
    use super::Pos;

    #[test]
    fn test_pos_cmp() {
        assert_eq!(
            Pos::from([0, 0, 0]).partial_cmp(&Pos::from([1, 1, 1])),
            Some(std::cmp::Ordering::Less)
        );
        assert_eq!(
            Pos::from([0, -1, 1]).partial_cmp(&Pos::from([1, -1, 1])),
            Some(std::cmp::Ordering::Less)
        );
        assert_eq!(
            Pos::from([0, 1, 0]).partial_cmp(&Pos::from([1, 0, 1])),
            None
        );
        assert_eq!(
            Pos::from([-1, 1, 0]).partial_cmp(&Pos::from([-1, 0, 1])),
            None
        );
        assert_eq!(
            Pos::from([3, 2, 1]).partial_cmp(&Pos::from([1, 2, 3])),
            None
        );
        assert_eq!(
            Pos::from([1, 1, 1]).partial_cmp(&Pos::from([0, 0, 0])),
            Some(std::cmp::Ordering::Greater)
        );
        assert_eq!(
            Pos::from([1, 3, -5]).partial_cmp(&Pos::from([0, 3, -6])),
            Some(std::cmp::Ordering::Greater)
        );
        assert_eq!(
            Pos::from([1, -1, 1]).partial_cmp(&Pos::from([0, -1, 1])),
            Some(std::cmp::Ordering::Greater)
        );
    }
}
