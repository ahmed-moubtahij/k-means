## K_means clustering
> k-means clustering is a method of vector quantization, originally from
> signal processing, that aims to partition n observations into k
> clusters in which each observation belongs to the cluster with the
> nearest mean (cluster centers or cluster centroid), serving as a
> prototype of the cluster.
> \- Wikipedia

## Requirements
- **C++2a**
- **gcc 10**
    - `msvc` hasn't implemented `std::ranges` yet and `clang`'s `std::ranges`/`concepts` seem to be yet incomplete.
- The only external dependency is `fmtlib` for output display but it isn't strictly necessary.

## Demo
See https://godbolt.org/z/e1eMos

Example use:

    using fmt::print, std::array;
    using kmn::DataPoint, kmn::print_clusters, kmn::k_means;
        
    auto const df = array{DataPoint(1, 2, 3),
                          DataPoint(4, 5, 6),
                          DataPoint(7, 8, 9),
                          DataPoint(10, 11, 12),
                          DataPoint(13, 14, 15),
                          DataPoint(16, 17, 18),
                          DataPoint(19, 20, 21),
                          DataPoint(22, 23, 24)};
        
    print("INPUT data points:\n\n{}\n\n", df);
        
    print("OUTPUT clusters:\n\n");
    print_clusters(k_means<3>(df, 50));
A call to `kmn::k_means<K>(data_frame, n)` returns a range of `K` clusters formed through `n` update iterations (the higher the `n` the better the partitioning).

A data point is to be wrapped with the `kmn::DataPoint<T, D>` type, with `T` an arithmetic type and `D` the point's dimensionality. `T` and `D` can be implicit through CTAD as shown in the above example. All data points must naturally have the same dimensionality.

A `std::array<kmn::DataPoint<T, D>, SZ>` is currently the type expected of the input data range.
