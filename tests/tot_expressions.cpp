#include "tot_array_fixture.h"

//------------------------------------------------------------------------------
//                            Permutations
//------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_SUITE(tot_permutations, ToTArrayFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(no_perm, TestParam, test_params){
  for(auto tr_t : run_all<TestParam>()){
    auto& in_rank = std::get<1>(tr_t);
    auto& t       = std::get<2>(tr_t);

    std::string out_idx = t.range().rank() == 1 ? "i" : "i, j";
    std::string in_idx  = in_rank == 1 ? "k" : "k, l";
    std::string idx     = out_idx + ";" + in_idx;

    tensor_type<TestParam> result;
    result(idx) = t(idx);
    BOOST_TEST(are_equal(result, t));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(permute_outer, TestParam, test_params){
  for(auto tr_t : run_all<TestParam>()){
    auto& in_rank = std::get<1>(tr_t);
    auto& t       = std::get<2>(tr_t);

    if(t.range().rank() == 1) continue;

    std::string rhs_out_idx = "i, j";
    std::string lhs_out_idx = "j, i";
    std::string in_idx      = in_rank == 1 ? "k" : "k, l";
    std::string rhs_idx     = rhs_out_idx + ";" + in_idx;
    std::string lhs_idx     = lhs_out_idx + ";" + in_idx;
    tensor_type<TestParam> result;
    result(lhs_idx) = t(rhs_idx);

    for(auto tile_idx : t.range()){
      auto rtile = t.find(tile_idx).get();
      auto ltile = result.find({tile_idx[1], tile_idx[0]}).get();
      for(auto outer_idx : ltile.range()){
        auto inner_range = ltile(outer_idx).range();
        auto outer_idx_t = {outer_idx[1], outer_idx[0]};
        bool same_inner_range = inner_range == rtile(outer_idx_t).range();
        BOOST_CHECK(same_inner_range);
        for(auto inner_idx : inner_range){
          const auto lelem = ltile(outer_idx)(inner_idx);
          const auto relem = rtile(outer_idx_t)(inner_idx);
          BOOST_CHECK_EQUAL(lelem, relem);
        }
      }
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(permute_inner, TestParam, test_params){
  for(auto tr_t : run_all<TestParam>()){
    auto& in_rank = std::get<1>(tr_t);
    auto& t       = std::get<2>(tr_t);

    if(in_rank == 1) continue;

    std::string rhs_in_idx = "i, j";
    std::string lhs_in_idx = "j, i";
    std::string out_idx    = t.range().rank() == 1 ? "k" : "k, l";
    std::string rhs_idx    = out_idx + ";" + rhs_in_idx;
    std::string lhs_idx    = out_idx + ";" + lhs_in_idx;
    tensor_type<TestParam> result;
    result(lhs_idx) = t(rhs_idx);

    for(auto tile_idx : t.range()){
      auto rtile = t.find(tile_idx).get();
      auto ltile = result.find(tile_idx).get();
      bool same_outer_range = ltile.range() == rtile.range();
      BOOST_CHECK(same_outer_range);
      for(auto outer_idx : ltile.range()){
        auto inner_range = ltile(outer_idx).range();
        for(auto inner_idx : inner_range){
          const auto lelem = ltile(outer_idx)(inner_idx);
          const auto inner_idx_t ={inner_idx[1], inner_idx[0]};
          const auto relem = rtile(outer_idx)(inner_idx_t);
          BOOST_CHECK_EQUAL(lelem, relem);
        }
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
//                           Addition
//------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_SUITE(tot_addition, ToTArrayFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(vov, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r2{2}, r3{3};
  inner_type lhs_0{r2}; lhs_0[0] = 0; lhs_0[1] = 1;
  inner_type lhs_1{r3}; lhs_1[0] = 1; lhs_1[1] = 2; lhs_1[2] = 3;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r2}; rhs_0[0] = 1; rhs_0[1] = 2;
  inner_type rhs_1{r3}; rhs_1[0] = 2; rhs_1[1] = 3; rhs_1[2] = 4;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r2}; c_0[0] = 1; c_0[1] = 3;
  inner_type c_1{r3}; c_1[0] = 3; c_1[1] = 5; c_1[2] = 7;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j") = lhs("i;j") + rhs("i;j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23}; lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
                         lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33}; lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
                         lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
                         lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23}; rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
                         rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33}; rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
                         rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
                         rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23}; c_0(0, 0) = 1; c_0(0, 1) = 3; c_0(0, 2) = 5;
                       c_0(1, 0) = 3; c_0(1, 1) = 5; c_0(1, 2) = 7;
  inner_type c_1{r33}; c_1(0, 0) = 3; c_1(0, 1) = 5; c_1(0, 2) = 7;
                       c_1(1, 0) = 5; c_1(1, 1) = 7; c_1(1, 2) = 9;
                       c_1(2, 0) = 7; c_1(2, 1) = 9; c_1(2, 2) = 11;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j,k") = lhs("i;j,k") + rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
    lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
    lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
    lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
    lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
    lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
    rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
    rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
    rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
    rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
    rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{range_type{3, 2}}; c_0(0, 0) = 1; c_0(0, 1) = 3;
                                    c_0(1, 0) = 3; c_0(1, 1) = 5;
                                    c_0(2, 0) = 5; c_0(2, 1) = 7;
  inner_type c_1{r33}; c_1(0, 0) = 3; c_1(0, 1) = 5; c_1(0, 2) = 7;
                       c_1(1, 0) = 5; c_1(1, 1) = 7; c_1(1, 2) = 9;
                       c_1(2, 0) = 7; c_1(2, 1) = 9; c_1(2, 2) = 11;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;k,j") = lhs("i;j,k") + rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0(range_type{3, 2});
    lhs_0(0, 0) = 0; lhs_0(0, 1) = 1;
    lhs_0(1, 0) = 1; lhs_0(1, 1) = 2;
    lhs_0(2, 0) = 2; lhs_0(2, 1) = 3;
  inner_type lhs_1{r33};
    lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
    lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
    lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
    rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
    rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
    rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
    rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
    rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
    c_0(0, 0) = 1; c_0(0, 1) = 3; c_0(0, 2) = 5;
    c_0(1, 0) = 3; c_0(1, 1) = 5; c_0(1, 2) = 7;
  inner_type c_1{r33}; c_1(0, 0) = 3; c_1(0, 1) = 5; c_1(0, 2) = 7;
  c_1(1, 0) = 5; c_1(1, 1) = 7; c_1(1, 2) = 9;
  c_1(2, 0) = 7; c_1(2, 1) = 9; c_1(2, 2) = 11;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;k,j") + rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_rhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{range_type{3, 2}};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3;
  rhs_0(2, 0) = 3; rhs_0(2, 1) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = 1; c_0(0, 1) = 3; c_0(0, 2) = 5;
  c_0(1, 0) = 3; c_0(1, 1) = 5; c_0(1, 2) = 7;
  inner_type c_1{r33}; c_1(0, 0) = 3; c_1(0, 1) = 5; c_1(0, 2) = 7;
  c_1(1, 0) = 5; c_1(1, 1) = 7; c_1(1, 2) = 9;
  c_1(2, 0) = 7; c_1(2, 1) = 9; c_1(2, 2) = 11;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;j,k") + rhs("i;k,j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs_and_rhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0(range_type{3, 2});
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2;
  lhs_0(2, 0) = 2; lhs_0(2, 1) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{range_type{3, 2}};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3;
  rhs_0(2, 0) = 3; rhs_0(2, 1) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = 1; c_0(0, 1) = 3; c_0(0, 2) = 5;
  c_0(1, 0) = 3; c_0(1, 1) = 5; c_0(1, 2) = 7;
  inner_type c_1{r33}; c_1(0, 0) = 3; c_1(0, 1) = 5; c_1(0, 2) = 7;
  c_1(1, 0) = 5; c_1(1, 1) = 7; c_1(1, 2) = 9;
  c_1(2, 0) = 7; c_1(2, 1) = 9; c_1(2, 2) = 11;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;k,j") + rhs("i;k,j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = 1; c_00[1] = 3;
  inner_type c_01(range_type{2}); c_01[0] = 3; c_01[1] = 5;
  inner_type c_02(range_type{2}); c_02[0] = 5; c_02[1] = 7;
  inner_type c_10(range_type{3}); c_10[0] = 3; c_10[1] = 5; c_10[2] = 7;
  inner_type c_11(range_type{3}); c_11[0] = 5; c_11[1] = 7; c_11[2] = 9;
  inner_type c_12(range_type{3}); c_12[0] = 7; c_12[1] = 9; c_12[2] = 11;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k") = lhs("i,j;k") + rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = 1; c_00[1] = 3;
  inner_type c_01(range_type{2}); c_01[0] = 3; c_01[1] = 5;
  inner_type c_02(range_type{2}); c_02[0] = 5; c_02[1] = 7;
  inner_type c_10(range_type{3}); c_10[0] = 3; c_10[1] = 5; c_10[2] = 7;
  inner_type c_11(range_type{3}); c_11[0] = 5; c_11[1] = 7; c_11[2] = 9;
  inner_type c_12(range_type{3}); c_12[0] = 7; c_12[1] = 9; c_12[2] = 11;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_10}, {c_01, c_11}, {c_02, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("j,i;k") = lhs("i,j;k") + rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = 1; c_00(0, 1) = 3;
    c_00(1, 0) = 3; c_00(1, 1) = 5;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = 3; c_01(0, 1) = 5; c_01(0, 2) = 7;
    c_01(1, 0) = 5; c_01(1, 1) = 7; c_01(1, 2) = 9;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = 5; c_02(0, 1) = 7; c_02(0, 2) = 9; c_02(0, 3) = 11;
    c_02(1, 0) = 7; c_02(1, 1) = 9; c_02(1, 2) = 11; c_02(1, 3) = 13;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = 3; c_10(0, 1) = 5;
    c_10(1, 0) = 5; c_10(1, 1) = 7;
    c_10(2, 0) = 7; c_10(2, 1) = 9;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = 5; c_11(0, 1) = 7; c_11(0, 2) = 9;
    c_11(1, 0) = 7; c_11(1, 1) = 9; c_11(1, 2) = 11;
    c_11(2, 0) = 9; c_11(2, 1) = 11; c_11(2, 2) = 13;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = 7; c_12(0, 1) = 9; c_12(0, 2) = 11; c_12(0, 3) = 13;
    c_12(1, 0) = 9; c_12(1, 1) = 11; c_12(1, 2) = 13; c_12(1, 3) = 15;
    c_12(2, 0) = 11; c_12(2, 1) = 13; c_12(2, 2) = 15; c_12(2, 3)= 17;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;k,l") + rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = 1; c_00(0, 1) = 3;
    c_00(1, 0) = 3; c_00(1, 1) = 5;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = 3; c_01(0, 1) = 5; c_01(0, 2) = 7;
    c_01(1, 0) = 5; c_01(1, 1) = 7; c_01(1, 2) = 9;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = 5; c_02(0, 1) = 7; c_02(0, 2) = 9; c_02(0, 3) = 11;
    c_02(1, 0) = 7; c_02(1, 1) = 9; c_02(1, 2) = 11; c_02(1, 3) = 13;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = 3; c_10(0, 1) = 5;
    c_10(1, 0) = 5; c_10(1, 1) = 7;
    c_10(2, 0) = 7; c_10(2, 1) = 9;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = 5; c_11(0, 1) = 7; c_11(0, 2) = 9;
    c_11(1, 0) = 7; c_11(1, 1) = 9; c_11(1, 2) = 11;
    c_11(2, 0) = 9; c_11(2, 1) = 11; c_11(2, 2) = 13;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = 7; c_12(0, 1) = 9; c_12(0, 2) = 11; c_12(0, 3) = 13;
    c_12(1, 0) = 9; c_12(1, 1) = 11; c_12(1, 2) = 13; c_12(1, 3) = 15;
    c_12(2, 0) = 11; c_12(2, 1) = 13; c_12(2, 2) = 15; c_12(2, 3)= 17;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_10}, {c_01, c_11}, {c_02, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("j,i;k,l") = lhs("i,j;k,l") + rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_lhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
  lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
  lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{3, 2});
  lhs_01(0, 0) = 1; lhs_01(0, 1) = 2;
  lhs_01(1, 0) = 2; lhs_01(1, 1) = 3;
  lhs_01(2, 0) = 3; lhs_01(2, 1) = 4;
  inner_type lhs_02(range_type{4, 2});
  lhs_02(0, 0) = 2; lhs_02(0, 1) = 3;
  lhs_02(1, 0) = 3; lhs_02(1, 1) = 4;
  lhs_02(2, 0) = 4; lhs_02(2, 1) = 5;
  lhs_02(3, 0) = 5; lhs_02(3, 1) = 6;
  inner_type lhs_10(range_type{2, 3});
  lhs_10(0, 0) = 1; lhs_10(0, 1) = 2; lhs_10(0, 2) = 3;
  lhs_10(1, 0) = 2; lhs_10(1, 1) = 3; lhs_10(1, 2) = 4;
  inner_type lhs_11(range_type{3, 3});
  lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
  lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
  lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{4, 3});
  lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5;
  lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6;
  lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7;
  lhs_12(3, 0) = 6; lhs_12(3, 1) = 7; lhs_12(3, 2) = 8;

  inner_type rhs_00(range_type{2, 2});
  rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
  rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
  rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
  rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
  rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
  rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
  rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
  rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
  rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
  rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
  rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
  rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
  rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
  rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
  rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
  c_00(0, 0) = 1; c_00(0, 1) = 3;
  c_00(1, 0) = 3; c_00(1, 1) = 5;
  inner_type c_01(range_type{2, 3});
  c_01(0, 0) = 3; c_01(0, 1) = 5; c_01(0, 2) = 7;
  c_01(1, 0) = 5; c_01(1, 1) = 7; c_01(1, 2) = 9;
  inner_type c_02(range_type{2, 4});
  c_02(0, 0) = 5; c_02(0, 1) = 7; c_02(0, 2) = 9; c_02(0, 3) = 11;
  c_02(1, 0) = 7; c_02(1, 1) = 9; c_02(1, 2) = 11; c_02(1, 3) = 13;
  inner_type c_10(range_type{3, 2});
  c_10(0, 0) = 3; c_10(0, 1) = 5;
  c_10(1, 0) = 5; c_10(1, 1) = 7;
  c_10(2, 0) = 7; c_10(2, 1) = 9;
  inner_type c_11(range_type{3, 3});
  c_11(0, 0) = 5; c_11(0, 1) = 7; c_11(0, 2) = 9;
  c_11(1, 0) = 7; c_11(1, 1) = 9; c_11(1, 2) = 11;
  c_11(2, 0) = 9; c_11(2, 1) = 11; c_11(2, 2) = 13;
  inner_type c_12(range_type{3, 4});
  c_12(0, 0) = 7; c_12(0, 1) = 9; c_12(0, 2) = 11; c_12(0, 3) = 13;
  c_12(1, 0) = 9; c_12(1, 1) = 11; c_12(1, 2) = 13; c_12(1, 3) = 15;
  c_12(2, 0) = 11; c_12(2, 1) = 13; c_12(2, 2) = 15; c_12(2, 3)= 17;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;l,k") + rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
//                            Subtraction
//------------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(tot_subtaction, ToTArrayFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(vov, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r2{2}, r3{3};
  inner_type lhs_0{r2}; lhs_0[0] = 0; lhs_0[1] = 1;
  inner_type lhs_1{r3}; lhs_1[0] = 1; lhs_1[1] = 2; lhs_1[2] = 3;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r2}; rhs_0[0] = 1; rhs_0[1] = 2;
  inner_type rhs_1{r3}; rhs_1[0] = 2; rhs_1[1] = 3; rhs_1[2] = 4;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r2}; c_0[0] = -1; c_0[1] = -1;
  inner_type c_1{r3}; c_1[0] = -1; c_1[1] = -1; c_1[2] = -1;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j") = lhs("i;j") - rhs("i;j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23}; lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
                         lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33}; lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
                         lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
                         lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23}; rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
                         rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33}; rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
                         rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
                         rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23}; c_0(0, 0) = -1; c_0(0, 1) = -1; c_0(0, 2) = -1;
                       c_0(1, 0) = -1; c_0(1, 1) = -1; c_0(1, 2) = -1;
  inner_type c_1{r33}; c_1(0, 0) = -1; c_1(0, 1) = -1; c_1(0, 2) = -1;
                       c_1(1, 0) = -1; c_1(1, 1) = -1; c_1(1, 2) = -1;
                       c_1(2, 0) = -1; c_1(2, 1) = -1; c_1(2, 2) = -1;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j,k") = lhs("i;j,k") - rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23}; lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
                         lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33}; lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
                         lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
                         lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23}; rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
                         rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33}; rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
                         rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
                         rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{range_type{3, 2}}; c_0(0, 0) = -1; c_0(0, 1) = -1;
                                    c_0(1, 0) = -1; c_0(1, 1) = -1;
                                    c_0(2, 0) = -1; c_0(2, 1) = -1;
  inner_type c_1{r33}; c_1(0, 0) = -1; c_1(0, 1) = -1; c_1(0, 2) = -1;
                       c_1(1, 0) = -1; c_1(1, 1) = -1; c_1(1, 2) = -1;
                       c_1(2, 0) = -1; c_1(2, 1) = -1; c_1(2, 2) = -1;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;k,j") = lhs("i;j,k") - rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0(range_type{3, 2});
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2;
  lhs_0(2, 0) = 2; lhs_0(2, 1) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = -1; c_0(0, 1) = -1; c_0(0, 2) = -1;
  c_0(1, 0) = -1; c_0(1, 1) = -1; c_0(1, 2) = -1;
  inner_type c_1{r33}; c_1(0, 0) = -1; c_1(0, 1) = -1; c_1(0, 2) = -1;
  c_1(1, 0) = -1; c_1(1, 1) = -1; c_1(1, 2) = -1;
  c_1(2, 0) = -1; c_1(2, 1) = -1; c_1(2, 2) = -1;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;k,j") - rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_rhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{range_type{3, 2}};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3;
  rhs_0(2, 0) = 3; rhs_0(2, 1) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = -1; c_0(0, 1) = -1; c_0(0, 2) = -1;
  c_0(1, 0) = -1; c_0(1, 1) = -1; c_0(1, 2) = -1;
  inner_type c_1{r33}; c_1(0, 0) = -1; c_1(0, 1) = -1; c_1(0, 2) = -1;
  c_1(1, 0) = -1; c_1(1, 1) = -1; c_1(1, 2) = -1;
  c_1(2, 0) = -1; c_1(2, 1) = -1; c_1(2, 2) = -1;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;j,k") - rhs("i;k,j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs_and_rhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0(range_type{3, 2});
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2;
  lhs_0(2, 0) = 2; lhs_0(2, 1) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{range_type{3, 2}};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3;
  rhs_0(2, 0) = 3; rhs_0(2, 1) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = -1; c_0(0, 1) = -1; c_0(0, 2) = -1;
  c_0(1, 0) = -1; c_0(1, 1) = -1; c_0(1, 2) = -1;
  inner_type c_1{r33}; c_1(0, 0) = -1; c_1(0, 1) = -1; c_1(0, 2) = -1;
  c_1(1, 0) = -1; c_1(1, 1) = -1; c_1(1, 2) = -1;
  c_1(2, 0) = -1; c_1(2, 1) = -1; c_1(2, 2) = -1;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;k,j") - rhs("i;k,j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;

  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = -1; c_00[1] = -1;
  inner_type c_01(range_type{2}); c_01[0] = -1; c_01[1] = -1;
  inner_type c_02(range_type{2}); c_02[0] = -1; c_02[1] = -1;
  inner_type c_10(range_type{3}); c_10[0] = -1; c_10[1] = -1; c_10[2] = -1;
  inner_type c_11(range_type{3}); c_11[0] = -1; c_11[1] = -1; c_11[2] = -1;
  inner_type c_12(range_type{3}); c_12[0] = -1; c_12[1] = -1; c_12[2] = -1;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k") = lhs("i,j;k") - rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = -1; c_00[1] = -1;
  inner_type c_01(range_type{2}); c_01[0] = -1; c_01[1] = -1;
  inner_type c_02(range_type{2}); c_02[0] = -1; c_02[1] = -1;
  inner_type c_10(range_type{3}); c_10[0] = -1; c_10[1] = -1; c_10[2] = -1;
  inner_type c_11(range_type{3}); c_11[0] = -1; c_11[1] = -1; c_11[2] = -1;
  inner_type c_12(range_type{3}); c_12[0] = -1; c_12[1] = -1; c_12[2] = -1;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_10}, {c_01, c_11}, {c_02, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("j,i;k") = lhs("i,j;k") - rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = -1; c_00(0, 1) = -1;
    c_00(1, 0) = -1; c_00(1, 1) = -1;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = -1; c_01(0, 1) = -1; c_01(0, 2) = -1;
    c_01(1, 0) = -1; c_01(1, 1) = -1; c_01(1, 2) = -1;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = -1; c_02(0, 1) = -1; c_02(0, 2) = -1; c_02(0, 3) = -1;
    c_02(1, 0) = -1; c_02(1, 1) = -1; c_02(1, 2) = -1; c_02(1, 3) = -1;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = -1; c_10(0, 1) = -1;
    c_10(1, 0) = -1; c_10(1, 1) = -1;
    c_10(2, 0) = -1; c_10(2, 1) = -1;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = -1; c_11(0, 1) = -1; c_11(0, 2) = -1;
    c_11(1, 0) = -1; c_11(1, 1) = -1; c_11(1, 2) = -1;
    c_11(2, 0) = -1; c_11(2, 1) = -1; c_11(2, 2) = -1;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = -1; c_12(0, 1) = -1; c_12(0, 2) = -1; c_12(0, 3) = -1;
    c_12(1, 0) = -1; c_12(1, 1) = -1; c_12(1, 2) = -1; c_12(1, 3) = -1;
    c_12(2, 0) = -1; c_12(2, 1) = -1; c_12(2, 2) = -1; c_12(2, 3) = -1;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;k,l") - rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = -1; c_00(0, 1) = -1;
    c_00(1, 0) = -1; c_00(1, 1) = -1;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = -1; c_01(0, 1) = -1; c_01(0, 2) = -1;
    c_01(1, 0) = -1; c_01(1, 1) = -1; c_01(1, 2) = -1;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = -1; c_02(0, 1) = -1; c_02(0, 2) = -1; c_02(0, 3) = -1;
    c_02(1, 0) = -1; c_02(1, 1) = -1; c_02(1, 2) = -1; c_02(1, 3) = -1;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = -1; c_10(0, 1) = -1;
    c_10(1, 0) = -1; c_10(1, 1) = -1;
    c_10(2, 0) = -1; c_10(2, 1) = -1;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = -1; c_11(0, 1) = -1; c_11(0, 2) = -1;
    c_11(1, 0) = -1; c_11(1, 1) = -1; c_11(1, 2) = -1;
    c_11(2, 0) = -1; c_11(2, 1) = -1; c_11(2, 2) = -1;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = -1; c_12(0, 1) = -1; c_12(0, 2) = -1; c_12(0, 3) = -1;
    c_12(1, 0) = -1; c_12(1, 1) = -1; c_12(1, 2) = -1; c_12(1, 3) = -1;
    c_12(2, 0) = -1; c_12(2, 1) = -1; c_12(2, 2) = -1; c_12(2, 3) = -1;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_10}, {c_01, c_11}, {c_02, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("j,i;k,l") = lhs("i,j;k,l") - rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_lhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
  lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
  lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{3, 2});
  lhs_01(0, 0) = 1; lhs_01(0, 1) = 2;
  lhs_01(1, 0) = 2; lhs_01(1, 1) = 3;
  lhs_01(2, 0) = 3; lhs_01(2, 1) = 4;
  inner_type lhs_02(range_type{4, 2});
  lhs_02(0, 0) = 2; lhs_02(0, 1) = 3;
  lhs_02(1, 0) = 3; lhs_02(1, 1) = 4;
  lhs_02(2, 0) = 4; lhs_02(2, 1) = 5;
  lhs_02(3, 0) = 5; lhs_02(3, 1) = 6;
  inner_type lhs_10(range_type{2, 3});
  lhs_10(0, 0) = 1; lhs_10(0, 1) = 2; lhs_10(0, 2) = 3;
  lhs_10(1, 0) = 2; lhs_10(1, 1) = 3; lhs_10(1, 2) = 4;
  inner_type lhs_11(range_type{3, 3});
  lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
  lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
  lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{4, 3});
  lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5;
  lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6;
  lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7;
  lhs_12(3, 0) = 6; lhs_12(3, 1) = 7; lhs_12(3, 2) = 8;

  inner_type rhs_00(range_type{2, 2});
  rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
  rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
  rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
  rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
  rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
  rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
  rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
  rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
  rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
  rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
  rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
  rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
  rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
  rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
  rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
  c_00(0, 0) = -1; c_00(0, 1) = -1;
  c_00(1, 0) = -1; c_00(1, 1) = -1;
  inner_type c_01(range_type{2, 3});
  c_01(0, 0) = -1; c_01(0, 1) = -1; c_01(0, 2) = -1;
  c_01(1, 0) = -1; c_01(1, 1) = -1; c_01(1, 2) = -1;
  inner_type c_02(range_type{2, 4});
  c_02(0, 0) = -1; c_02(0, 1) = -1; c_02(0, 2) = -1; c_02(0, 3) = -1;
  c_02(1, 0) = -1; c_02(1, 1) = -1; c_02(1, 2) = -1; c_02(1, 3) = -1;
  inner_type c_10(range_type{3, 2});
  c_10(0, 0) = -1; c_10(0, 1) = -1;
  c_10(1, 0) = -1; c_10(1, 1) = -1;
  c_10(2, 0) = -1; c_10(2, 1) = -1;
  inner_type c_11(range_type{3, 3});
  c_11(0, 0) = -1; c_11(0, 1) = -1; c_11(0, 2) = -1;
  c_11(1, 0) = -1; c_11(1, 1) = -1; c_11(1, 2) = -1;
  c_11(2, 0) = -1; c_11(2, 1) = -1; c_11(2, 2) = -1;
  inner_type c_12(range_type{3, 4});
  c_12(0, 0) = -1; c_12(0, 1) = -1; c_12(0, 2) = -1; c_12(0, 3) = -1;
  c_12(1, 0) = -1; c_12(1, 1) = -1; c_12(1, 2) = -1; c_12(1, 3) = -1;
  c_12(2, 0) = -1; c_12(2, 1) = -1; c_12(2, 2) = -1; c_12(2, 3) = -1;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;l,k") - rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
//                      Binary Op with Scaling
//------------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(tot_scale, ToTArrayFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(vov_lhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r2{2}, r3{3};
  inner_type lhs_0{r2}; lhs_0[0] = 0; lhs_0[1] = 1;
  inner_type lhs_1{r3}; lhs_1[0] = 1; lhs_1[1] = 2; lhs_1[2] = 3;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r2}; rhs_0[0] = 1; rhs_0[1] = 2;
  inner_type rhs_1{r3}; rhs_1[0] = 2; rhs_1[1] = 3; rhs_1[2] = 4;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r2}; c_0[0] = 1; c_0[1] = 4;
  inner_type c_1{r3}; c_1[0] = 4; c_1[1] = 7; c_1[2] = 10;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j") = 2 * lhs("i;j") + rhs("i;j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vov_rhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r2{2}, r3{3};
  inner_type lhs_0{r2}; lhs_0[0] = 0; lhs_0[1] = 1;
  inner_type lhs_1{r3}; lhs_1[0] = 1; lhs_1[1] = 2; lhs_1[2] = 3;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r2}; rhs_0[0] = 1; rhs_0[1] = 2;
  inner_type rhs_1{r3}; rhs_1[0] = 2; rhs_1[1] = 3; rhs_1[2] = 4;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r2}; c_0[0] = 2; c_0[1] = 5;
  inner_type c_1{r3}; c_1[0] = 5; c_1[1] = 8; c_1[2] = 11;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j") = lhs("i;j") + 2 * rhs("i;j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
    lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
    lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
    lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
    lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
    lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
    rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
    rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
    rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
    rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
    rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
    c_0(0, 0) = 1; c_0(0, 1) = 4; c_0(0, 2) = 7;
    c_0(1, 0) = 4; c_0(1, 1) = 7; c_0(1, 2) = 10;
  inner_type c_1{r33};
    c_1(0, 0) = 4; c_1(0, 1) = 7; c_1(0, 2) = 10;
    c_1(1, 0) = 7; c_1(1, 1) = 10; c_1(1, 2) = 13;
    c_1(2, 0) = 10; c_1(2, 1) = 13; c_1(2, 2) = 16;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j,k") = 2 * lhs("i;j,k") + rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_rhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
    lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
    lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
    lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
    lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
    lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
    rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
    rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
    rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
    rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
    rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
    c_0(0, 0) = 2; c_0(0, 1) = 5; c_0(0, 2) = 8;
    c_0(1, 0) = 5; c_0(1, 1) = 8; c_0(1, 2) = 11;
  inner_type c_1{r33};
    c_1(0, 0) = 5; c_1(0, 1) = 8; c_1(0, 2) = 11;
    c_1(1, 0) = 8; c_1(1, 1) = 11; c_1(1, 2) = 14;
    c_1(2, 0) = 11; c_1(2, 1) = 14; c_1(2, 2) = 17;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j,k") = lhs("i;j,k") + 2 * rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov_lhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;

  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = 1; c_00[1] = 4;
  inner_type c_01(range_type{2}); c_01[0] = 4; c_01[1] = 7;
  inner_type c_02(range_type{2}); c_02[0] = 7; c_02[1] = 10;
  inner_type c_10(range_type{3}); c_10[0] = 4; c_10[1] = 7; c_10[2] = 10;
  inner_type c_11(range_type{3}); c_11[0] = 7; c_11[1] = 10; c_11[2] = 13;
  inner_type c_12(range_type{3}); c_12[0] = 10; c_12[1] = 13; c_12[2] = 16;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k") = 2 * lhs("i,j;k") + rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov_rhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;

  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = 2; c_00[1] = 5;
  inner_type c_01(range_type{2}); c_01[0] = 5; c_01[1] = 8;
  inner_type c_02(range_type{2}); c_02[0] = 8; c_02[1] = 11;
  inner_type c_10(range_type{3}); c_10[0] = 5; c_10[1] = 8; c_10[2] = 11;
  inner_type c_11(range_type{3}); c_11[0] = 8; c_11[1] = 11; c_11[2] = 14;
  inner_type c_12(range_type{3}); c_12[0] = 11; c_12[1] = 14; c_12[2] = 17;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k") = lhs("i,j;k") + 2 * rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_lhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = 1; c_00(0, 1) = 4;
    c_00(1, 0) = 4; c_00(1, 1) = 7;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = 4; c_01(0, 1) = 7; c_01(0, 2) = 10;
    c_01(1, 0) = 7; c_01(1, 1) = 10; c_01(1, 2) = 13;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = 7; c_02(0, 1) = 10; c_02(0, 2) = 13; c_02(0, 3) = 16;
    c_02(1, 0) = 10; c_02(1, 1) = 13; c_02(1, 2) = 16; c_02(1, 3) = 19;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = 4; c_10(0, 1) = 7;
    c_10(1, 0) = 7; c_10(1, 1) = 10;
    c_10(2, 0) = 10; c_10(2, 1) = 13;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = 7; c_11(0, 1) = 10; c_11(0, 2) = 13;
    c_11(1, 0) = 10; c_11(1, 1) = 13; c_11(1, 2) = 16;
    c_11(2, 0) = 13; c_11(2, 1) = 16; c_11(2, 2) = 19;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = 10; c_12(0, 1) = 13; c_12(0, 2) = 16; c_12(0, 3) = 19;
    c_12(1, 0) = 13; c_12(1, 1) = 16; c_12(1, 2) = 19; c_12(1, 3) = 22;
    c_12(2, 0) = 16; c_12(2, 1) = 19; c_12(2, 2) = 22; c_12(2, 3) = 25;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = 2 * lhs("i,j;k,l") + rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_rhs, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = 2; c_00(0, 1) = 5;
    c_00(1, 0) = 5; c_00(1, 1) = 8;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = 5; c_01(0, 1) = 8; c_01(0, 2) = 11;
    c_01(1, 0) = 8; c_01(1, 1) = 11; c_01(1, 2) = 14;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = 8; c_02(0, 1) = 11; c_02(0, 2) = 14; c_02(0, 3) = 17;
    c_02(1, 0) = 11; c_02(1, 1) = 14; c_02(1, 2) = 17; c_02(1, 3) = 20;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = 5; c_10(0, 1) = 8;
    c_10(1, 0) = 8; c_10(1, 1) = 11;
    c_10(2, 0) = 11; c_10(2, 1) = 14;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = 8; c_11(0, 1) = 11; c_11(0, 2) = 14;
    c_11(1, 0) = 11; c_11(1, 1) = 14; c_11(1, 2) = 17;
    c_11(2, 0) = 14; c_11(2, 1) = 17; c_11(2, 2) = 20;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = 11; c_12(0, 1) = 14; c_12(0, 2) = 17; c_12(0, 3) = 20;
    c_12(1, 0) = 14; c_12(1, 1) = 17; c_12(1, 2) = 20; c_12(1, 3) = 23;
    c_12(2, 0) = 17; c_12(2, 1) = 20; c_12(2, 2) = 23; c_12(2, 3) = 26;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;k,l") + 2 * rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
//                       Element-wise Product
//------------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(tot_hadamard, ToTArrayFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(vov, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r2{2}, r3{3};
  inner_type lhs_0{r2}; lhs_0[0] = 0; lhs_0[1] = 1;
  inner_type lhs_1{r3}; lhs_1[0] = 1; lhs_1[1] = 2; lhs_1[2] = 3;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r2}; rhs_0[0] = 1; rhs_0[1] = 2;
  inner_type rhs_1{r3}; rhs_1[0] = 2; rhs_1[1] = 3; rhs_1[2] = 4;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r2}; c_0[0] = 0; c_0[1] = 2;
  inner_type c_1{r3}; c_1[0] = 2; c_1[1] = 6; c_1[2] = 12;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j") = lhs("i;j") * rhs("i;j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
    lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
    lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
    lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
    lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
    lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
    rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
    rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
    rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
    rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
    rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
    c_0(0, 0) = 0; c_0(0, 1) = 2; c_0(0, 2) = 6;
    c_0(1, 0) = 2; c_0(1, 1) = 6; c_0(1, 2) = 12;
  inner_type c_1{r33};
    c_1(0, 0) = 2; c_1(0, 1) = 6; c_1(0, 2) = 12;
    c_1(1, 0) = 6; c_1(1, 1) = 12; c_1(1, 2) = 20;
    c_1(2, 0) = 12; c_1(2, 1) = 20; c_1(2, 2) = 30;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;j,k") = lhs("i;j,k") * rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
    lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
    lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
    lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
    lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
    lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
    rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
    rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
    rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
    rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
    rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{range_type{3, 2}};
    c_0(0, 0) = 0; c_0(0, 1) = 2;
    c_0(1, 0) = 2; c_0(1, 1) = 6;
    c_0(2, 0) = 6; c_0(2, 1) = 12;
  inner_type c_1{r33};
    c_1(0, 0) = 2; c_1(0, 1) = 6; c_1(0, 2) = 12;
    c_1(1, 0) = 6; c_1(1, 1) = 12; c_1(1, 2) = 20;
    c_1(2, 0) = 12; c_1(2, 1) = 20; c_1(2, 2) = 30;
  il_type corr_il{c_0, c_1};

  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i;k,j") = lhs("i;j,k") * rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0(range_type{3, 2});
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2;
  lhs_0(2, 0) = 2; lhs_0(2, 1) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r23};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2; rhs_0(0, 2) = 3;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3; rhs_0(1, 2) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = 0; c_0(0, 1) = 2; c_0(0, 2) = 6;
  c_0(1, 0) = 2; c_0(1, 1) = 6; c_0(1, 2) = 12;
  inner_type c_1{r33};
  c_1(0, 0) = 2; c_1(0, 1) = 6; c_1(0, 2) = 12;
  c_1(1, 0) = 6; c_1(1, 1) = 12; c_1(1, 2) = 20;
  c_1(2, 0) = 12; c_1(2, 1) = 20; c_1(2, 2) = 30;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;k,j") * rhs("i;j,k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_rhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0{r23};
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1; lhs_0(0, 2) = 2;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2; lhs_0(1, 2) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{range_type{3, 2}};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3;
  rhs_0(2, 0) = 3; rhs_0(2, 1) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = 0; c_0(0, 1) = 2; c_0(0, 2) = 6;
  c_0(1, 0) = 2; c_0(1, 1) = 6; c_0(1, 2) = 12;
  inner_type c_1{r33};
  c_1(0, 0) = 2; c_1(0, 1) = 6; c_1(0, 2) = 12;
  c_1(1, 0) = 6; c_1(1, 1) = 12; c_1(1, 2) = 20;
  c_1(2, 0) = 12; c_1(2, 1) = 20; c_1(2, 2) = 30;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;j,k") * rhs("i;k,j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(vom_lhs_and_rhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  range_type r23{2, 3}, r33{3, 3};
  inner_type lhs_0(range_type{3, 2});
  lhs_0(0, 0) = 0; lhs_0(0, 1) = 1;
  lhs_0(1, 0) = 1; lhs_0(1, 1) = 2;
  lhs_0(2, 0) = 2; lhs_0(2, 1) = 3;
  inner_type lhs_1{r33};
  lhs_1(0, 0) = 1; lhs_1(0, 1) = 2; lhs_1(0, 2) = 3;
  lhs_1(1, 0) = 2; lhs_1(1, 1) = 3; lhs_1(1, 2) = 4;
  lhs_1(2, 0) = 3; lhs_1(2, 1) = 4; lhs_1(2, 2) = 5;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{range_type{3, 2}};
  rhs_0(0, 0) = 1; rhs_0(0, 1) = 2;
  rhs_0(1, 0) = 2; rhs_0(1, 1) = 3;
  rhs_0(2, 0) = 3; rhs_0(2, 1) = 4;
  inner_type rhs_1{r33};
  rhs_1(0, 0) = 2; rhs_1(0, 1) = 3; rhs_1(0, 2) = 4;
  rhs_1(1, 0) = 3; rhs_1(1, 1) = 4; rhs_1(1, 2) = 5;
  rhs_1(2, 0) = 4; rhs_1(2, 1) = 5; rhs_1(2, 2) = 6;
  il_type rhs_il{rhs_0, rhs_1};

  inner_type c_0{r23};
  c_0(0, 0) = 0; c_0(0, 1) = 2; c_0(0, 2) = 6;
  c_0(1, 0) = 2; c_0(1, 1) = 6; c_0(1, 2) = 12;
  inner_type c_1{r33};
  c_1(0, 0) = 2; c_1(0, 1) = 6; c_1(0, 2) = 12;
  c_1(1, 0) = 6; c_1(1, 1) = 12; c_1(1, 2) = 20;
  c_1(2, 0) = 12; c_1(2, 1) = 20; c_1(2, 2) = 30;
  il_type corr_il{c_0, c_1};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result, temp;
  result("i;j,k") = lhs("i;k,j") * rhs("i;k,j");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = 0; c_00[1] = 2;
  inner_type c_01(range_type{2}); c_01[0] = 2; c_01[1] = 6;
  inner_type c_02(range_type{2}); c_02[0] = 6; c_02[1] = 12;
  inner_type c_10(range_type{3}); c_10[0] = 2; c_10[1] = 6; c_10[2] = 12;
  inner_type c_11(range_type{3}); c_11[0] = 6; c_11[1] = 12; c_11[2] = 20;
  inner_type c_12(range_type{3}); c_12[0] = 12; c_12[1] = 20; c_12[2] = 30;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k") = lhs("i,j;k") * rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mov_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2}); lhs_00[0] = 0; lhs_00[1] = 1;
  inner_type lhs_01(range_type{2}); lhs_01[0] = 1; lhs_01[1] = 2;
  inner_type lhs_02(range_type{2}); lhs_02[0] = 2; lhs_02[1] = 3;
  inner_type lhs_10(range_type{3}); lhs_10[0] = 1; lhs_10[1] = 2; lhs_10[2] = 3;
  inner_type lhs_11(range_type{3}); lhs_11[0] = 2; lhs_11[1] = 3; lhs_11[2] = 4;
  inner_type lhs_12(range_type{3}); lhs_12[0] = 3; lhs_12[1] = 4; lhs_12[2] = 5;

  inner_type rhs_02(range_type{2}); rhs_02[0] = 3; rhs_02[1] = 4;
  inner_type rhs_12(range_type{3}); rhs_12[0] = 4; rhs_12[1] = 5; rhs_12[2] = 6;

  inner_type c_00(range_type{2}); c_00[0] = 0; c_00[1] = 2;
  inner_type c_01(range_type{2}); c_01[0] = 2; c_01[1] = 6;
  inner_type c_02(range_type{2}); c_02[0] = 6; c_02[1] = 12;
  inner_type c_10(range_type{3}); c_10[0] = 2; c_10[1] = 6; c_10[2] = 12;
  inner_type c_11(range_type{3}); c_11[0] = 6; c_11[1] = 12; c_11[2] = 20;
  inner_type c_12(range_type{3}); c_12[0] = 12; c_12[1] = 20; c_12[2] = 30;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{lhs_01, lhs_02, rhs_02}, {lhs_11, lhs_12, rhs_12}};
  il_type corr_il{{c_00, c_10}, {c_01, c_11}, {c_02, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("j,i;k") = lhs("i,j;k") * rhs("i,j;k");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = 0; c_00(0, 1) = 2;
    c_00(1, 0) = 2; c_00(1, 1) = 6;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = 2; c_01(0, 1) = 6; c_01(0, 2) = 12;
    c_01(1, 0) = 6; c_01(1, 1) = 12; c_01(1, 2) = 20;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = 6; c_02(0, 1) = 12; c_02(0, 2) = 20; c_02(0, 3) = 30;
    c_02(1, 0) = 12; c_02(1, 1) = 20; c_02(1, 2) = 30; c_02(1, 3) = 42;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = 2; c_10(0, 1) = 6;
    c_10(1, 0) = 6; c_10(1, 1) = 12;
    c_10(2, 0) = 12; c_10(2, 1) = 20;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = 6; c_11(0, 1) = 12; c_11(0, 2) = 20;
    c_11(1, 0) = 12; c_11(1, 1) = 20; c_11(1, 2) = 30;
    c_11(2, 0) = 20; c_11(2, 1) = 30; c_11(2, 2) = 42;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = 12; c_12(0, 1) = 20; c_12(0, 2) = 30; c_12(0, 3) = 42;
    c_12(1, 0) = 20; c_12(1, 1) = 30; c_12(1, 2) = 42; c_12(1, 3) = 56;
    c_12(2, 0) = 30; c_12(2, 1) = 42; c_12(2, 2) = 56; c_12(2, 3) = 72;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;k,l") * rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_result_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
    lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
    lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{2, 3});
    lhs_01(0, 0) = 1; lhs_01(0, 1) = 2; lhs_01(0, 2) = 3;
    lhs_01(1, 0) = 2; lhs_01(1, 1) = 3; lhs_01(1, 2) = 4;
  inner_type lhs_02(range_type{2, 4});
    lhs_02(0, 0) = 2; lhs_02(0, 1) = 3; lhs_02(0, 2) = 4; lhs_02(0, 3) = 5;
    lhs_02(1, 0) = 3; lhs_02(1, 1) = 4; lhs_02(1, 2) = 5; lhs_02(1, 3) = 6;
  inner_type lhs_10(range_type{3, 2});
    lhs_10(0, 0) = 1; lhs_10(0, 1) = 2;
    lhs_10(1, 0) = 2; lhs_10(1, 1) = 3;
    lhs_10(2, 0) = 3; lhs_10(2, 1) = 4;
  inner_type lhs_11(range_type{3, 3});
    lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
    lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
    lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{3, 4});
    lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5; lhs_12(0, 3) = 6;
    lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6; lhs_12(1, 3) = 7;
    lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7; lhs_12(2, 3) = 8;

  inner_type rhs_00(range_type{2, 2});
    rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
    rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
    rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
    rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
    rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
    rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
    rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
    rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
    rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
    rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
    rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
    rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
    rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
    rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
    rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
    c_00(0, 0) = 0; c_00(0, 1) = 2;
    c_00(1, 0) = 2; c_00(1, 1) = 6;
  inner_type c_01(range_type{2, 3});
    c_01(0, 0) = 2; c_01(0, 1) = 6; c_01(0, 2) = 12;
    c_01(1, 0) = 6; c_01(1, 1) = 12; c_01(1, 2) = 20;
  inner_type c_02(range_type{2, 4});
    c_02(0, 0) = 6; c_02(0, 1) = 12; c_02(0, 2) = 20; c_02(0, 3) = 30;
    c_02(1, 0) = 12; c_02(1, 1) = 20; c_02(1, 2) = 30; c_02(1, 3) = 42;
  inner_type c_10(range_type{3, 2});
    c_10(0, 0) = 2; c_10(0, 1) = 6;
    c_10(1, 0) = 6; c_10(1, 1) = 12;
    c_10(2, 0) = 12; c_10(2, 1) = 20;
  inner_type c_11(range_type{3, 3});
    c_11(0, 0) = 6; c_11(0, 1) = 12; c_11(0, 2) = 20;
    c_11(1, 0) = 12; c_11(1, 1) = 20; c_11(1, 2) = 30;
    c_11(2, 0) = 20; c_11(2, 1) = 30; c_11(2, 2) = 42;
  inner_type c_12(range_type{3, 4});
    c_12(0, 0) = 12; c_12(0, 1) = 20; c_12(0, 2) = 30; c_12(0, 3) = 42;
    c_12(1, 0) = 20; c_12(1, 1) = 30; c_12(1, 2) = 42; c_12(1, 3) = 56;
    c_12(2, 0) = 30; c_12(2, 1) = 42; c_12(2, 2) = 56; c_12(2, 3) = 72;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_10}, {c_01, c_11}, {c_02, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("j,i;k,l") = lhs("i,j;k,l") * rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mom_lhs_transpose, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::matrix_il<inner_type>;
  inner_type lhs_00(range_type{2, 2});
  lhs_00(0, 0) = 0; lhs_00(0, 1) = 1;
  lhs_00(1, 0) = 1; lhs_00(1, 1) = 2;
  inner_type lhs_01(range_type{3, 2});
  lhs_01(0, 0) = 1; lhs_01(0, 1) = 2;
  lhs_01(1, 0) = 2; lhs_01(1, 1) = 3;
  lhs_01(2, 0) = 3; lhs_01(2, 1) = 4;
  inner_type lhs_02(range_type{4, 2});
  lhs_02(0, 0) = 2; lhs_02(0, 1) = 3;
  lhs_02(1, 0) = 3; lhs_02(1, 1) = 4;
  lhs_02(2, 0) = 4; lhs_02(2, 1) = 5;
  lhs_02(3, 0) = 5; lhs_02(3, 1) = 6;
  inner_type lhs_10(range_type{2, 3});
  lhs_10(0, 0) = 1; lhs_10(0, 1) = 2; lhs_10(0, 2) = 3;
  lhs_10(1, 0) = 2; lhs_10(1, 1) = 3; lhs_10(1, 2) = 4;
  inner_type lhs_11(range_type{3, 3});
  lhs_11(0, 0) = 2; lhs_11(0, 1) = 3; lhs_11(0, 2) = 4;
  lhs_11(1, 0) = 3; lhs_11(1, 1) = 4; lhs_11(1, 2) = 5;
  lhs_11(2, 0) = 4; lhs_11(2, 1) = 5; lhs_11(2, 2) = 6;
  inner_type lhs_12(range_type{4, 3});
  lhs_12(0, 0) = 3; lhs_12(0, 1) = 4; lhs_12(0, 2) = 5;
  lhs_12(1, 0) = 4; lhs_12(1, 1) = 5; lhs_12(1, 2) = 6;
  lhs_12(2, 0) = 5; lhs_12(2, 1) = 6; lhs_12(2, 2) = 7;
  lhs_12(3, 0) = 6; lhs_12(3, 1) = 7; lhs_12(3, 2) = 8;

  inner_type rhs_00(range_type{2, 2});
  rhs_00(0, 0) = 1; rhs_00(0, 1) = 2;
  rhs_00(1, 0) = 2; rhs_00(1, 1) = 3;
  inner_type rhs_01(range_type{2, 3});
  rhs_01(0, 0) = 2; rhs_01(0, 1) = 3; rhs_01(0, 2) = 4;
  rhs_01(1, 0) = 3; rhs_01(1, 1) = 4; rhs_01(1, 2) = 5;
  inner_type rhs_02(range_type{2, 4});
  rhs_02(0, 0) = 3; rhs_02(0, 1) = 4; rhs_02(0, 2) = 5; rhs_02(0, 3) = 6;
  rhs_02(1, 0) = 4; rhs_02(1, 1) = 5; rhs_02(1, 2) = 6; rhs_02(1, 3) = 7;
  inner_type rhs_10(range_type{3, 2});
  rhs_10(0, 0) = 2; rhs_10(0, 1) = 3;
  rhs_10(1, 0) = 3; rhs_10(1, 1) = 4;
  rhs_10(2, 0) = 4; rhs_10(2, 1) = 5;
  inner_type rhs_11(range_type{3, 3});
  rhs_11(0, 0) = 3; rhs_11(0, 1) = 4; rhs_11(0, 2) = 5;
  rhs_11(1, 0) = 4; rhs_11(1, 1) = 5; rhs_11(1, 2) = 6;
  rhs_11(2, 0) = 5; rhs_11(2, 1) = 6; rhs_11(2, 2) = 7;
  inner_type rhs_12(range_type{3, 4});
  rhs_12(0, 0) = 4; rhs_12(0, 1) = 5; rhs_12(0, 2) = 6; rhs_12(0, 3) = 7;
  rhs_12(1, 0) = 5; rhs_12(1, 1) = 6; rhs_12(1, 2) = 7; rhs_12(1, 3) = 8;
  rhs_12(2, 0) = 6; rhs_12(2, 1) = 7; rhs_12(2, 2) = 8; rhs_12(2, 3) = 9;

  inner_type c_00(range_type{2, 2});
  c_00(0, 0) = 0; c_00(0, 1) = 2;
  c_00(1, 0) = 2; c_00(1, 1) = 6;
  inner_type c_01(range_type{2, 3});
  c_01(0, 0) = 2; c_01(0, 1) = 6; c_01(0, 2) = 12;
  c_01(1, 0) = 6; c_01(1, 1) = 12; c_01(1, 2) = 20;
  inner_type c_02(range_type{2, 4});
  c_02(0, 0) = 6; c_02(0, 1) = 12; c_02(0, 2) = 20; c_02(0, 3) = 30;
  c_02(1, 0) = 12; c_02(1, 1) = 20; c_02(1, 2) = 30; c_02(1, 3) = 42;
  inner_type c_10(range_type{3, 2});
  c_10(0, 0) = 2; c_10(0, 1) = 6;
  c_10(1, 0) = 6; c_10(1, 1) = 12;
  c_10(2, 0) = 12; c_10(2, 1) = 20;
  inner_type c_11(range_type{3, 3});
  c_11(0, 0) = 6; c_11(0, 1) = 12; c_11(0, 2) = 20;
  c_11(1, 0) = 12; c_11(1, 1) = 20; c_11(1, 2) = 30;
  c_11(2, 0) = 20; c_11(2, 1) = 30; c_11(2, 2) = 42;
  inner_type c_12(range_type{3, 4});
  c_12(0, 0) = 12; c_12(0, 1) = 20; c_12(0, 2) = 30; c_12(0, 3) = 42;
  c_12(1, 0) = 20; c_12(1, 1) = 30; c_12(1, 2) = 42; c_12(1, 3) = 56;
  c_12(2, 0) = 30; c_12(2, 1) = 42; c_12(2, 2) = 56; c_12(2, 3) = 72;

  il_type lhs_il{{lhs_00, lhs_01, lhs_02}, {lhs_10, lhs_11, lhs_12}};
  il_type rhs_il{{rhs_00, rhs_01, rhs_02}, {rhs_10, rhs_11, rhs_12}};
  il_type corr_il{{c_00, c_01, c_02}, {c_10, c_11, c_12}};
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> corr(m_world, corr_il);
  tensor_type<TestParam> result;
  result ("i,j;k,l") = lhs("i,j;l,k") * rhs("i,j;k,l");
  BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_SUITE_END()

//------------------------------------------------------------------------------
//                           Contraction
//------------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(tot_contraction, ToTArrayFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(vov_inner_contraction, TestParam, test_params){
  using inner_type = inner_type<TestParam>;
  using range_type = Range;
  using il_type    = detail::vector_il<inner_type>;
  using policy_type = policy_type<TestParam>;
  range_type r2{2}, r3{3};
  inner_type lhs_0{r2}; lhs_0[0] = 0; lhs_0[1] = 1;
  inner_type lhs_1{r3}; lhs_1[0] = 1; lhs_1[1] = 2; lhs_1[2] = 3;
  il_type lhs_il{lhs_0, lhs_1};

  inner_type rhs_0{r2}; rhs_0[0] = 1; rhs_0[1] = 2;
  inner_type rhs_1{r3}; rhs_1[0] = 2; rhs_1[1] = 3; rhs_1[2] = 4;
  il_type rhs_il{rhs_0, rhs_1};

  using result_type = DistArray<Tensor<scalar_type<TestParam>>, policy_type>;
  // N.B. explicitly declare il type here due to https://bugs.llvm.org//show_bug.cgi?id=23689
  result_type corr(m_world, TiledArray::detail::vector_il<double>{2.0, 20.0});
  tensor_type<TestParam> lhs(m_world, lhs_il);
  tensor_type<TestParam> rhs(m_world, rhs_il);
  tensor_type<TestParam> result;
  //einsum(result("i"), lhs("i;j"), rhs("i;j"));
  //BOOST_CHECK(are_equal(result, corr));
}

BOOST_AUTO_TEST_SUITE_END()
