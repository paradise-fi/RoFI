use iter_fixed::IntoIteratorFixed;

type Sizes = [usize; 3];

#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Vec3D<T> {
    data: Vec<T>,
    sizes: Sizes,
}

impl<T: std::fmt::Debug> std::fmt::Debug for Vec3D<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.get_data()
            .map(Iterator::collect)
            .collect::<Vec<Vec<_>>>()
            .fmt(f)
    }
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
        assert!(
            sizes.iter().all(|&size| size > 0),
            "size of new Vec3D is zero (sizes={sizes:?})"
        );
        let mut data = Vec::new();
        data.resize_with(sizes.iter().product(), Default::default);
        Self { data, sizes }
    }

    fn inner_index(&self, indices: Sizes) -> Option<usize> {
        if indices
            .into_iter_fixed()
            .zip(self.sizes)
            .into_iter()
            .any(|(idx, size)| idx >= size)
        {
            return None;
        }
        let inner_index = indices
            .into_iter_fixed()
            .zip(self.sizes)
            .into_iter()
            .fold(0, |value, (idx, size)| value * size + idx);
        Some(inner_index)
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
        self.data.get(self.inner_index(indices)?)
    }
    pub fn get_mut(&mut self, indices: Sizes) -> Option<&mut T> {
        let index = self.inner_index(indices)?;
        self.data.get_mut(index)
    }
}
