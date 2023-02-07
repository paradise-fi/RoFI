pub type IndexType = u8;

#[derive(Debug, Clone)]
pub struct Subset(u64);

impl Subset {
    pub const MAX_SIZE: IndexType = 64;

    pub fn all(size: usize) -> Subset {
        assert!(size <= Self::MAX_SIZE.into());
        if size == Self::MAX_SIZE.into() {
            Self(u64::MAX)
        } else {
            Self((1u64 << size) - 1)
        }
    }

    pub fn iter_all(size: usize) -> SubsetIter {
        assert!(size <= Self::MAX_SIZE.into());
        SubsetIter::new(Self::all(size))
    }

    pub fn has_elem(&self, elem: IndexType) -> bool {
        assert!(elem < Self::MAX_SIZE);
        self.0 & (1u64 << elem) != 0
    }
}

#[derive(Debug, Clone)]
pub struct SubsetIter {
    next: Option<Subset>,
}

impl SubsetIter {
    fn new(subset: Subset) -> Self {
        Self { next: Some(subset) }
    }
}

impl Iterator for SubsetIter {
    type Item = Subset;

    fn next(&mut self) -> Option<Self::Item> {
        let new_next = self.next.as_ref()?.0.checked_sub(1).map(Subset);
        std::mem::replace(&mut self.next, new_next)
    }
}
