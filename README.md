![alt text](https://i.imgur.com/RBXzdQ8.png)
#### Infographic made by [spiyer99](https://github.com/spiyer99)
## Requirements
- **-std=c++2a**
- **gcc 10**
    - `msvc` hasn't implemented `std::ranges` yet and `clang`'s `std::ranges`/`concepts` seem to be yet incomplete.
- The only external dependency is `fmtlib` for output display but it isn't strictly necessary.

## Context
At the time of writing this, I'm an undergrad in Systems Engineering, and this is intended as a practice project that ideally evolves into something useful.

## Demo
See https://godbolt.org/z/4z9bs1 (code may not be up to date with the latest version).

```cpp
using fmt::print, std::array, kmn::DataPoint;
using kmn::print_clusters, kmn::k_means;

//INPUT range
auto const int_arr_df =
array{DataPoint(1, 2, 3),    DataPoint(4, 5, 6),    DataPoint(7, 8, 9),
      DataPoint(10, 11, 12), DataPoint(13, 14, 15), DataPoint(16, 17, 18),
      DataPoint(19, 20, 21), DataPoint(22, 23, 24), DataPoint(25, 26, 27),
      DataPoint(28, 29, 30), DataPoint(31, 32, 33), DataPoint(34, 35, 36),
      DataPoint(37, 38, 39), DataPoint(40, 41, 42), DataPoint(43, 44, 45)};

//OUTPUT range
std::array<kmn::Cluster<int, 3>, 6> clusters;

//CALL
std::size_t const k{ 4 }, n{ 10 };
k_means(int_arr_df, clusters, k, n);

//DISPLAY
print("OUTPUT clusters:\n\n");
print_clusters(clusters, k);
```
```
OUTPUT clusters:

centroid: {26.5, 27.5, 28.5}
satellites: {{22, 23, 24}, {25, 26, 27}, {28, 29, 30}, {31, 32, 33}}

centroid: {5.5, 6.5, 7.5}
satellites: {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}

centroid: {38.5, 39.5, 40.5}
satellites: {{34, 35, 36}, {37, 38, 39}, {40, 41, 42}, {43, 44, 45}}

centroid: {16, 17, 18}
satellites: {{13, 14, 15}, {19, 20, 21}}
```
A call to `k_means(data_points_range, out_clusters_range, k, n);` writes `K` clusters in `out_clusters_range` formed through `n` update iterations (the higher the `n` the better the partitioning).

A data point is to be wrapped with the `kmn::DataPoint<T, D>` type, with `T` an arithmetic type and `D` the point's dimensionality. `T` and `D` can be implicit through CTAD as shown in the above example. All data points must naturally have the same dimensionality.

## TODO
- Require from clusters' output range iterator to model `std::forward_iterator` since it's being written to in multiple passes.
- Write unit tests.
- Decide on return type of `kmn::k_means`.
- Decide whether it's intrusive/avoidable to impose `kmn::Cluster` on the user.
- *marcorubini@#includecpp*: Wrap k_means as a function object to allow for lazy semantics and passing it around.
- Lookup opportunities for `std::move`, in-place construction...etc.
- Provide an interface for file input.
- *dicroce@Reddit*: Write `auto_k_means`; start with K=1, iteratively employ k-means with greater K's until adding a new centroid implies most of the satellites assigned to it came from an existing cluster.
- Parallelizing.

## Thanks
My thanks go to a few competent minds from the #includecpp Discord who helped me in understanding the C++ ins and outs to write this code: sarah, Léo, marcorubini, oktal and Lesley Lai.
