<img src="https://i.imgur.com/RBXzdQ8.png" alt="drawing" width="650"/>
#### Infographic made by [spiyer99](https://github.com/spiyer99)

## Demo
### See: https://godbolt.org/z/zTvbcK (may not be up to date with the most recent code).
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

## Context
At the time of writing this, I'm an undergrad in Systems Engineering, and this is intended as a practice project that ideally evolves into something useful.

## How do I use this?

**Requirements**
- **-std=c++20**
- **gcc 10.1**
    - `msvc` hasn't implemented `std::ranges` yet and `clang`'s `std::ranges`/`concepts` seem to be yet incomplete.
- External dependencies: `range-v3` and `fmtlib` (optional for output display).

**Build Instructions**
```
    cd path/to/your/folder
    git clone https://github.com/Ayenem/k-means.git --recursive
    cd k-means
    cmake -B build
    cmake --build build
```
You may then execute the provided example to ensure everything works e.g. `./build/src/demo`.

## TODO
- Clang-tidy
- Cppcheck
- Include-what-you-use
- Write unit tests (`catch2`).
    - Check https://youtu.be/YbgH7yat-Jo?t=1455 and https://www.youtube.com/watch?v=Ob5_XZrFQH0&t=589s
    - A given input (with fixed `n` and `k`) will have N reference outputs which the output of a given revision of the implementation has to compare against (i.e. references' mean or any one of them) within a tolerance.
        - The comparison would be done by euclidean distance between output ranges of indices.
    - Do this for differently typed (range-wise, value type-wise, cv-qualification-wise) X inputs.    
- Set up CI.
    - _ninjawedding@includecpp_: if you've got CI configured you should have at least one test configuration that runs your tests with sanitizers enabled.
    - Need to benchmark compile and run times.
- Measure compile-time & run-time.
- Subject further refactorings to TDD.
- See if k_means_impl's steps can be made into custom views (see https://youtu.be/d_E-VLyUnzc?list=PLco7M25q_3hCWAYODpIDsq9_IH9oXf04W&t=1035); it's currently operating with eager intermediate operations.
    - "Just as with borrowed ranges, a type can opt in to being a view using the `enable_view` trait, or by inheriting from `ranges::views_base`.
- Adapt implementation to rvalue inputs (e.g. spans, adapted views, borrowed ranges) either with rvalue ref parameter overload, or with a specialization returning a `kmn::dangling_reference` empty object when appropriate for handling the returned `data_points` and `out_indices` references.
- Prettify `print_kmn_result` with a tabular format display.
- Look into how to detect moves/copies of types (including library types).
- Make sure these are used as they should be: move semantics, RVO, in-place construction, `explicit` ctors, `noexcept`...etc.
- Look into SBO for the returned vectors.
- Write a blog?
- Provide an interface for file input.
- Convergence criteria instead of `n` iterations.
- *dicroce@Reddit*: Write `auto_k_means`; start with K=1, iteratively employ k-means with greater K's until adding a new centroid implies most of the satellites assigned to it came from an existing cluster.
- Concurrency.
- Look into `#include <immintrin.h>` compiler intrinsics.

## Thanks
My thanks go to a few competent minds from the #includecpp Discord who helped me in understanding the C++ ins and outs to write this code: _sarah_, _Léo_, _marcorubini_, _oktal_, _Lesley Lai_ and _ninjawedding_. _Lorely_ and _melak-47_ for the CMake stuff. _tre_, _Nicole Mazzuca_ and _Robert Schumacher_ for the vcpkg stuff.
