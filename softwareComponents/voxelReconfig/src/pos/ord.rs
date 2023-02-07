use super::Pos;
use std::ops::Deref;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct OrdPos<IndexType: num::Num>(pub Pos<IndexType>);

impl<TIndex: num::Num> Deref for OrdPos<TIndex> {
    type Target = Pos<TIndex>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl<TIndex: num::Num + PartialOrd> PartialOrd for OrdPos<TIndex> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        for (lhs, rhs) in [
            (&self.x, &other.x),
            (&self.y, &other.y),
            (&self.z, &other.z),
        ] {
            match lhs.partial_cmp(rhs) {
                Some(std::cmp::Ordering::Equal) => {}
                ord => return ord,
            }
        }
        Some(std::cmp::Ordering::Equal)
    }
}

impl<TIndex: num::Num + Ord> Ord for OrdPos<TIndex> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        for (lhs, rhs) in [
            (&self.x, &other.x),
            (&self.y, &other.y),
            (&self.z, &other.z),
        ] {
            match lhs.cmp(rhs) {
                std::cmp::Ordering::Equal => {}
                ord => return ord,
            }
        }
        std::cmp::Ordering::Equal
    }
}
