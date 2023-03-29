pub enum IterEnum<A, B> {
    A(A),
    B(B),
}

impl<A, B> Iterator for IterEnum<A, B>
where
    A: Iterator,
    B: Iterator<Item = A::Item>,
{
    type Item = A::Item;
    fn next(&mut self) -> Option<Self::Item> {
        match self {
            IterEnum::A(this) => this.next(),
            IterEnum::B(this) => this.next(),
        }
    }
}
