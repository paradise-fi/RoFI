use super::{Connections, VoxelWorldWithConnections};
use crate::reconfig::metric::{cost::Cost, Metric};
use crate::voxel_world::NormVoxelWorld;

#[derive(Debug)]
pub struct WorldMetricWrapper<TMetric>(TMetric);

impl<TWorld, TConnections, TMetric> Metric<VoxelWorldWithConnections<TWorld, TConnections>>
    for WorldMetricWrapper<TMetric>
where
    TWorld: NormVoxelWorld,
    TConnections: Connections<IndexType = TWorld::IndexType>,
    TMetric: Metric<TWorld>,
{
    type Potential = TMetric::Potential;
    type EstimatedCost = TMetric::EstimatedCost;

    fn new(goal: &VoxelWorldWithConnections<TWorld, TConnections>) -> Self
    where
        Self: Sized,
    {
        Self(TMetric::new(goal.world()))
    }

    fn get_potential(
        &mut self,
        state: &VoxelWorldWithConnections<TWorld, TConnections>,
    ) -> Self::Potential {
        self.0.get_potential(state.world())
    }

    fn estimated_cost(cost: Cost<Self::Potential>) -> Self::EstimatedCost {
        TMetric::estimated_cost(cost)
    }
}
