![alt text](https://i.imgur.com/RBXzdQ8.png)
#### Infographic made by [spiyer99](https://github.com/spiyer99)
## Requirements
- **-std=c++20**
- **> gcc 10.2**
    - `msvc` hasn't implemented `std::ranges` yet and `clang`'s `std::ranges`/`concepts` seem to be yet incomplete.
- External dependencies: `range-v3` and `fmtlib` (optional for output display).

## Context
At the time of writing this, I'm an undergrad in Systems Engineering, and this is intended as a practice project that ideally evolves into something useful.

## Demo
```cpp
using fmt::print, std::array, kmn::DataPoint;
using kmn::print_clusters, kmn::k_means;

//INPUT range
auto const int_arr_df =
array{DataPoint(1, 2, 3),    DataPoint(4, 5, 6),    DataPoint(7, 8, 9),
      DataPoint(28, 29, 30), DataPoint(31, 32, 33), DataPoint(34, 35, 36),
      DataPoint(19, 20, 21), DataPoint(22, 23, 24), DataPoint(25, 26, 27),
      DataPoint(10, 11, 12), DataPoint(13, 14, 15), DataPoint(16, 17, 18),
      DataPoint(37, 38, 39), DataPoint(40, 41, 42), DataPoint(43, 44, 45)};

//OUTPUT range
vector<std::size_t> out_indices(int_arr_df.size());
//CALL & DISPLAY RESULT
std::size_t const k{ 4 }, n{ 10 };
// k_means(int_arr_df, out_indices, k, n);
auto kmn_result = k_means(int_arr_df, out_indices, k, n);
fmt::print("Cluster indices for each point:\n {}\n\n", out_indices);
fmt::print("Points partitionned into clusters:\n\n");
print_kmn_result(kmn_result);
```
```
Cluster indices for each point:
 {1, 1, 1, 2, 2, 2, 3, 3, 2, 1, 3, 3, 4, 4, 4}

Points partitionned into clusters:

Centroid: {5.5, 6.5, 7.5}
Satellites: {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}

Centroid: {29.5, 30.5, 31.5}
Satellites: {{28, 29, 30}, {31, 32, 33}, {34, 35, 36}, {25, 26, 27}}

Centroid: {17.5, 18.5, 19.5}
Satellites: {{19, 20, 21}, {22, 23, 24}, {13, 14, 15}, {16, 17, 18}}

Centroid: {40, 41, 42}
Satellites: {{37, 38, 39}, {40, 41, 42}, {43, 44, 45}}
```
A call to `k_means(data_points_range, out_indices_range, k, n);` populates `out_indices_range` with a cluster index (from 1 to `k`) for each data point. The partitionning is done through `n` updates (the higher the `n` the better the partitioning).

A call to `k_means` returns a `std::optional` object with potentially useful information for the user: `{ vector of centroids, vector of cluster sizes, reference to input range, reference to output range }`. This object is iterable over cluster objects i.e. each iterated element returns a `{ centroid, satellites }` object.

A data point is to be wrapped with the `kmn::DataPoint<T, D>` type, with `T` an arithmetic type and `D` the point's dimensionality. `T` and `D` can be implicit through CTAD as shown in the above example. All data points must naturally have the same dimensionality.

## TODO
- Write unit tests.
    - A given input (with fixed `n` and `k`) will have N reference outputs which the output of a given revision of the implementation has to compare against (i.e. references' mean or any one of them) within a tolerance.
        - The comparison would be done by euclidean distance between output ranges of indices.
    - Do this for differently typed (range-wise, value type-wise, cv-qualification-wise) X inputs.
- Convergence criteria instead of `n` iterations.
- Lookup opportunities for `std::move`, in-place construction...etc.
- Provide an interface for file input.
- *dicroce@Reddit*: Write `auto_k_means`; start with K=1, iteratively employ k-means with greater K's until adding a new centroid implies most of the satellites assigned to it came from an existing cluster.
- Parallelizing.

## Thanks
My thanks go to a few competent minds from the #includecpp Discord who helped me in understanding the C++ ins and outs to write this code: sarah, Léo, marcorubini, oktal, Lesley Lai and ninjawedding.
