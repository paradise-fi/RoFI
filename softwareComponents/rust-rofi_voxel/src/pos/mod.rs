pub type IndexType = u8;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct VoxelPos(pub [IndexType; 3]);

pub fn compute_minimal_pos_hull<TIndex, IPos>(
    positions: IPos,
) -> Option<[std::ops::Range<TIndex>; 3]>
where
    TIndex: Ord + std::ops::Add<TIndex, Output = TIndex> + num::One + Copy,
    IPos: IntoIterator<Item = [TIndex; 3]>,
{
    positions
        .into_iter()
        .map(|pos| pos.map(|pos| pos..pos + TIndex::one()))
        .reduce(|acc, pos| {
            acc.zip(pos).map(|(acc, pos)| {
                std::cmp::min(acc.start, pos.start)..std::cmp::max(acc.end, pos.end)
            })
        })
}
