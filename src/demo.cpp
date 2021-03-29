#include <kmn/K_means.hpp>

int main()
{
  using kmn::DataPoint, kmn::print_kmn_result, kmn::k_means;
  // INPUT ranges
  auto const int_arr_df =
  std::array{ DataPoint(1, 2, 3),    DataPoint(4, 5, 6),
              DataPoint(7, 8, 9),    DataPoint(28, 29, 30),
              DataPoint(31, 32, 33), DataPoint(34, 35, 36),
              DataPoint(19, 20, 21), DataPoint(22, 23, 24),
              DataPoint(25, 26, 27), DataPoint(10, 11, 12),
              DataPoint(13, 14, 15), DataPoint(16, 17, 18),
              DataPoint(37, 38, 39), DataPoint(40, 41, 42) };

  //auto const double_arr_df = std::array{
  //  DataPoint(1., 2., 3.),    DataPoint(4., 5., 6.),
  //  DataPoint(7., 8., 9.),    DataPoint(10., 11., 12.),
  //  DataPoint(13., 14., 15.), DataPoint(16., 17., 18.),
  //  DataPoint(19., 20., 21.), DataPoint(22., 23., 24.),
  //  DataPoint(25., 26., 27.), DataPoint(28., 29., 30.),
  //  DataPoint(31., 32., 33.), DataPoint(34., 35., 36.),
  //  DataPoint(37., 38., 39.), DataPoint(40., 41., 42.) };

  //auto const float_arr_df = std::array{
  //  DataPoint(1.f, 2.f, 3.f),    DataPoint(4.f, 5.f, 6.f),
  //  DataPoint(7.f, 8.f, 9.f),    DataPoint(10.f, 11.f, 12.f),
  //  DataPoint(13.f, 14.f, 15.f), DataPoint(16.f, 17.f, 18.f),
  //  DataPoint(19.f, 20.f, 21.f), DataPoint(22.f, 23.f, 24.f),
  //  DataPoint(25.f, 26.f, 27.f), DataPoint(28.f, 29.f, 30.f),
  //  DataPoint(31.f, 32.f, 33.f), DataPoint(34.f, 35.f, 36.f),
  //  DataPoint(37.f, 38.f, 39.f), DataPoint(40.f, 41.f, 42.f) };
  //
  //auto const int_vec_df = std::vector{
  //  DataPoint(1, 2, 3),    DataPoint(4, 5, 6),
  //  DataPoint(7, 8, 9),    DataPoint(10, 11, 12),
  //  DataPoint(13, 14, 15), DataPoint(16, 17, 18),
  //  DataPoint(19, 20, 21), DataPoint(22, 23, 24),
  //  DataPoint(25, 26, 27), DataPoint(28, 29, 30),
  //  DataPoint(31, 32, 33), DataPoint(34, 35, 36),
  //  DataPoint(37, 38, 39), DataPoint(40, 41, 42) };

  // OUTPUT RANGE
  std::vector<std::size_t> out_indices(int_arr_df.size());

  // FUNCTION ARGUMENTS; k: Number of clusters, n: partitionning resolution
  std::size_t const k{ 4 };
  std::size_t const n{ 10 };

  // CALL & DISPLAY RESULT
  if(auto&& kmn_result = k_means(int_arr_df, out_indices, k, n)) {
    print_kmn_result(std::move(kmn_result));
  } else {
    fmt::print("k_means returned std::nullopt\n");
  }

  return 0;
}
