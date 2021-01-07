![alt text](https://i.redd.it/g16w49p2hw661.png)
## Requirements
- **-std=c++2a**
- **gcc 10**
    - `msvc` hasn't implemented `std::ranges` yet and `clang`'s `std::ranges`/`concepts` seem to be yet incomplete.
- The only external dependency is `fmtlib` for output display but it isn't strictly necessary.

## Context
At the time of writing this, I'm an undergrad in Systems Engineering, and this is intended as a practice project that ideally evolves into something useful.

## Demo
See https://godbolt.org/z/hxbrTz (code may not be up to date with the latest version).

```cpp
using fmt::print, std::array, kmn::DataPoint;
using kmn::print_clusters, kmn::k_means;
auto const df = array{DataPoint(1, 2, 3), DataPoint(4, 5, 6),
                      DataPoint(7, 8, 9), DataPoint(10, 11, 12),
                      DataPoint(13, 14, 15), DataPoint(16, 17, 18),
                      DataPoint(19, 20, 21), DataPoint(22, 23, 24),
                      DataPoint(25, 26, 27), DataPoint(28, 29, 30),
                      DataPoint(31, 32, 33), DataPoint(34, 35, 36),
                      DataPoint(37, 38, 39), DataPoint(40, 41, 42)};
                     
print("OUTPUT clusters:\n\n");
print_clusters(k_means<4>(df, 100));
```
```
OUTPUT clusters:

centroid: {7, 8, 9}
satellites: {{1, 2, 3}, {4, 5, 6}, {10, 11, 12}}

centroid: {16, 17, 18}
satellites: {{13, 14, 15}, {19, 20, 21}, {22, 23, 24}}

centroid: {28, 29, 30}
satellites: {{25, 26, 27}, {31, 32, 33}}

centroid: {37, 38, 39}
satellites: {{34, 35, 36}, {40, 41, 42}}
```
A call to `kmn::k_means<K>(data_frame, n)` returns a range of `K` clusters formed through `n` update iterations (the higher the `n` the better the partitioning).

A data point is to be wrapped with the `kmn::DataPoint<T, D>` type, with `T` an arithmetic type and `D` the point's dimensionality. `T` and `D` can be implicit through CTAD as shown in the above example. All data points must naturally have the same dimensionality.

A `std::array<kmn::DataPoint<T, D>, SZ>` is currently the type expected of the input data range.

## TODO
- *andrewsutton @Reddit*: Refactor to a range type of data points parameter.
- Refactor to an output range parameter instead of a returned function-local array.
- Fix bug with an input range of `DataPoint<float, D>`.
- Decide whether it's intrusive/avoidable to impose `kmn::Cluster` on the user.
- *sarah @#includecpp*: Use `std::ranges::*` function objects e.g. `rn::begin(\*)` instead of` \*.begin()`.
- *sarah @#includecpp*: `begin` and `end` instead of `cbegin` and `cend` in the context of ranges; views have shallow const semantics.
- *marcorubini @#includecpp*: Wrap k_means as a fuction object to allow for lazy semantics and passing it around.
- *Lesley Lai @#includecpp*: Explicitize generic lambdas; `auto` isn't type inference but a template parameter `T` in this context.
- Lookup opportunities for `std::move`, in-place construction...etc.
- *dicroce @Reddit*: Write `auto_k_means`; start with K=1, iteratively employ k-means with greater K's until adding a new centroid implies most of the satellites assigned to it came from an existing cluster.
- Parallelizing.

## Thanks
My thanks go to a few competent minds from the #includecpp Discord who helped me in understanding the C++ ins and outs to write this code: sarah, Léo, marcorubini, oktal and Lesley Lai.
