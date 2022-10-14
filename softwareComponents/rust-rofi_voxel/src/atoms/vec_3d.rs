type Sizes = [usize; 3];

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Vec3D<T> {
    data: Vec<T>,
    sizes: Sizes,
}

impl<T> Vec3D<T> {
    pub fn new_with_value(value: T, sizes: Sizes) -> Self
    where
        T: Clone,
    {
        assert!(sizes.iter().all(|&size| size > 0));
        let data = vec![value; sizes.iter().product()];
        Self { data, sizes }
    }
    pub fn new(sizes: Sizes) -> Self
    where
        T: Default,
    {
        assert!(sizes.iter().all(|&size| size > 0));
        let mut data = Vec::new();
        data.resize_with(sizes.iter().product(), Default::default);
        Self { data, sizes }
    }

    fn inner_index(&self, indices: Sizes) -> usize {
        self.sizes
            .iter()
            .zip(indices.iter())
            .fold(0, |value, (&size, &idx)| value * size + idx)
    }

    pub fn sizes(&self) -> Sizes {
        self.sizes
    }

    pub fn get_data(&self) -> impl Iterator<Item = impl Iterator<Item = &[T]>> {
        let [_x_size, y_size, z_size] = self.sizes;
        self.data
            .chunks_exact(y_size * z_size)
            .map(move |chunk| chunk.chunks_exact(z_size))
    }
    pub fn get_data_mut(&mut self) -> impl Iterator<Item = impl Iterator<Item = &mut [T]>> {
        let [_x_size, y_size, z_size] = self.sizes;
        self.data
            .chunks_exact_mut(y_size * z_size)
            .map(move |chunk| chunk.chunks_exact_mut(z_size))
    }

    pub fn get(&self, indices: Sizes) -> Option<&T> {
        self.data.get(self.inner_index(indices))
    }
    pub fn get_mut(&mut self, indices: Sizes) -> Option<&mut T> {
        let index = self.inner_index(indices);
        self.data.get_mut(index)
    }
}
