#include <gtest/gtest.h>

#include "../../../src/core/maps/grid_rasterization.h"

class Directions {
private:
  constexpr static int Left_Id  = 1 << 0;
  constexpr static int Right_Id = 1 << 1;
  constexpr static int Up_Id    = 1 << 2;
  constexpr static int Down_Id  = 1 << 3;
public:
  Directions& set_left()  { _data |= Left_Id; return *this; }
  Directions& set_right() { _data |= Right_Id; return *this; }
  Directions& set_up()    { _data |= Up_Id; return *this; }
  Directions& set_down()  { _data |= Down_Id; return *this; }

  bool left() const  { return _data & Left_Id; }
  bool right() const { return _data & Right_Id; }
  bool up() const    { return _data & Up_Id; }
  bool down() const  { return _data & Down_Id; }

private:
  int _data = 0;
};

//============================================================================//
//===                                 Tests                                ===//
//============================================================================//

class RectangleGridRasterizationTest : public ::testing::Test {
protected:
  static constexpr double Grid_Scale = 0.5; // use 0.5 to prevent FP erros
  static constexpr double Cell_Len = Grid_Scale;
  static constexpr double Half_Cell_Len = Cell_Len / 2;
  static constexpr double Quat_Cell_Len = Half_Cell_Len / 2;
public:
  RectangleGridRasterizationTest() : grid{100, 100, Grid_Scale} {}

protected: // types
  using AreaId = RegularSquaresGrid::Coord;
  using AreaIds = std::set<AreaId>;
protected: // methods

  AreaIds area_to_ids(const Point2D &center, double side_len) {
    auto half_side = side_len / 2;
    auto rect = Rectangle{-half_side, half_side,
                          -half_side, half_side}.move_center(center);
    auto raw_ids = GridRasterizedRectangle{grid, rect}.to_vector();
    return AreaIds{raw_ids.begin(), raw_ids.end()};
  }

  AreaIds area_to_ids(const Point2D &offset,
                      double bot_len, double top_len,
                      double left_len, double right_len) {
    auto rect = Rectangle{-bot_len + offset.y, top_len + offset.y,
                          -left_len + offset.x, right_len + offset.x};
    auto raw_ids = GridRasterizedRectangle{grid, rect}.to_vector();
    return AreaIds{raw_ids.begin(), raw_ids.end()};
  }

   /*
   *  ...................
   *  .     .     .     .
   *  .     .     .     .
   *  .  @@@@@@@@@@@@@  .
   *  .  @  .  ^  .  @  .
   *  .  @  .  |  .  @  .
   *  ...@.....|.....@...
   *  .  @  .  |  .  @  .
   *  .  @  . *** .  @  .
   *  .  @<---* *--->@  .
   *  .  @  . *** .  @  .
   *  .  @  .  |  .  @  .
   *  ...@.....|.....@...
   *  .  @  .  |  .  @  .
   *  .  @  .  V  .  @  .
   *  .  @@@@@@@@@@@@@  .
   *  .     .     .     .
   *  .     .     .     .
   *  ...................
   */
  void test_area_exceeding(const Directions &dirs) {
    auto area_id = AreaId{};
    for (area_id.x = -1; area_id.x <= 1; ++area_id.x) {
      for (area_id.y = -1; area_id.y <= 1; ++area_id.y) {
        // setup params
        auto area_offset = grid.cell_to_world(area_id);
        auto left = Quat_Cell_Len, right = Quat_Cell_Len,
             top = Quat_Cell_Len, bot = Quat_Cell_Len;

        int expected_d_x_start = 0, expected_d_x_end = 0,
            expected_d_y_start = 0, expected_d_y_end = 0;
        if (dirs.left()) {
          area_offset.x -= Quat_Cell_Len;
          left += Half_Cell_Len;
          expected_d_x_start = -1;
        }
        if (dirs.right()) {
          area_offset.x += Quat_Cell_Len;
          right += Half_Cell_Len;
          expected_d_x_end = 1;
        }
        if (dirs.down()) {
          area_offset.y -= Quat_Cell_Len;
          bot += Half_Cell_Len;
          expected_d_y_start = -1;
        }
        if (dirs.up()) {
          area_offset.y += Quat_Cell_Len;
          top += Half_Cell_Len;
          expected_d_y_end = 1;
        }

        // generate expected
        auto expected = AreaIds{};
        for (int d_x = expected_d_x_start; d_x <= expected_d_x_end; ++d_x) {
          for (int d_y = expected_d_y_start; d_y <= expected_d_y_end; ++d_y) {
            expected.emplace(area_id.x + d_x, area_id.y + d_y);
          }
        }

        // generate actual
        auto actual = area_to_ids(area_offset, bot, top, left, right);
        // check area
        ASSERT_EQ(expected, actual);
      }
    }
  }

  /*
   *   .     .     .     .
   *   .     .     .     .
   * ..@@@@@@@@@@@@@@@@@@@..
   *   @     .  ^  .     @
   *   @     .  |  .     @
   *   @  *************  @
   *   @  *  .     .  *  @
   *   @  *  .     .  *  @
   * ..@..*...........*..@..
   *   @  *  .     .  *  @
   *   @  *  .     .  *  @
   *   @<-*  .     .  *->@
   *   @  *  .     .  *  @
   *   @  *  .     .  *  @
   * ..@..*...........*..@..
   *   @  *  .     .  *  @
   *   @  *  .     .  *  @
   *   @  *************  @
   *   @     .  |  .     @
   *   @     .  V  .     @
   * ..@@@@@@@@@@@@@@@@@@@..
   *   .     .     .     .
   *   .     .     .     .
   */
  void test_area_alignement(const Directions &dirs) {
    auto area_id = AreaId{};
    for (area_id.x = -1; area_id.x <= 1; ++area_id.x) {
      for (area_id.y = -1; area_id.y <= 1; ++area_id.y) {
        int d_x_limit = 1, d_y_limit = 1;
        // generate expected
        auto left = Cell_Len, right = Cell_Len, top = Cell_Len, bot = Cell_Len;
        if (dirs.left())  { left  += Half_Cell_Len; }
        if (dirs.down())   { bot   += Half_Cell_Len; }

        if (dirs.right()) {
          ++d_x_limit;
          right += Half_Cell_Len;
        }

        if (dirs.up()) {
          ++d_y_limit;
          top += Half_Cell_Len;
        }

        auto expected = AreaIds{};
        for (int d_x = -1; d_x <= d_x_limit; ++d_x) {
          for (int d_y = -1; d_y <= d_y_limit; ++d_y) {
            expected.emplace(area_id.x + d_x, area_id.y + d_y);
          }
        }

        // generate actual
        auto actual = area_to_ids(grid.cell_to_world(area_id),
                                  bot, top, left, right);
        // check area
        ASSERT_EQ(expected, actual);
      }
    }
  }

protected: // fields
  RegularSquaresGrid grid;
};

bool operator<(const RegularSquaresGrid::Coord &c1,
               const RegularSquaresGrid::Coord &c2) {
  if (c1.x == c2.x) { return c1.y < c2.y; }
  return c1.x < c2.x;
}

TEST_F(RectangleGridRasterizationTest, emptyAreaInsideCell) {
  auto area_id = AreaId{};
  for (area_id.x = -1; area_id.x <= 1; ++area_id.x) {
    for (area_id.y = -1; area_id.y <= 1; ++area_id.y) {
      ASSERT_EQ(area_to_ids({grid.cell_to_world(area_id)}, 0),
                AreaIds{area_id});
    }
  }
}

TEST_F(RectangleGridRasterizationTest, emptyAreaOnBorders) {
  // left bot
  ASSERT_EQ(area_to_ids({0, 0}, 0), (AreaIds{{0, 0}}));

  // left
  ASSERT_EQ(area_to_ids({0, Half_Cell_Len}, 0), (AreaIds{{0, 0}}));

  // left top
  ASSERT_EQ(area_to_ids({0, Cell_Len}, 0), (AreaIds{{0, 1}}));

  // top
  ASSERT_EQ(area_to_ids({Half_Cell_Len, Cell_Len}, 0), (AreaIds{{0, 1}}));

  // right top
  ASSERT_EQ(area_to_ids({Cell_Len, Cell_Len}, 0), (AreaIds{{1, 1}}));

  // right
  ASSERT_EQ(area_to_ids({Cell_Len, Half_Cell_Len}, 0), (AreaIds{{1, 0}}));

  // right bot
  ASSERT_EQ(area_to_ids({Cell_Len, 0}, 0), (AreaIds{{1, 0}}));

  // bot
  ASSERT_EQ(area_to_ids({Half_Cell_Len, 0}, 0), (AreaIds{{0, 0}}));
}

//----------------------------------------------------------------------------//
//--- An area exceeds a Cell ---//

TEST_F(RectangleGridRasterizationTest, areaDoesNotExceedCell) {
  test_area_exceeding(Directions{});
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellLeft) {
  test_area_exceeding(Directions{}.set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellRight) {
  test_area_exceeding(Directions{}.set_right());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellRightLeft) {
  test_area_exceeding(Directions{}.set_right().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellUp) {
  test_area_exceeding(Directions{}.set_up());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellUpLeft) {
  test_area_exceeding(Directions{}.set_up().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellUpRight) {
  test_area_exceeding(Directions{}.set_up().set_right());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellUpRightLeft) {
  test_area_exceeding(Directions{}.set_up().set_right().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDown) {
  test_area_exceeding(Directions{}.set_down());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownLeft) {
  test_area_exceeding(Directions{}.set_down().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownRight) {
  test_area_exceeding(Directions{}.set_down().set_right());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownRightLeft) {
  test_area_exceeding(Directions{}.set_down().set_right().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownUp) {
  test_area_exceeding(Directions{}.set_down().set_up());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownUpLeft) {
  test_area_exceeding(Directions{}.set_down().set_up().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownUpRight) {
  test_area_exceeding(Directions{}.set_down().set_up().set_right());
}

TEST_F(RectangleGridRasterizationTest, areaExceedsCellDownUpRightLeft) {
  test_area_exceeding(Directions{}.set_down().set_up().set_right().set_left());
}

//----------------------------------------------------------------------------//
//--- An area is aligned with cell's border ---//

TEST_F(RectangleGridRasterizationTest, areaNotAligned) {
  // redundant, added for completeness
  test_area_alignement(Directions{});
}

TEST_F(RectangleGridRasterizationTest, areaAlignesLeft) {
  test_area_alignement(Directions{}.set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesRight) {
  test_area_alignement(Directions{}.set_right());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesRightLeft) {
  test_area_alignement(Directions{}.set_right().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesUp) {
  test_area_alignement(Directions{}.set_up());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesUpLeft) {
  test_area_alignement(Directions{}.set_up().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesUpRight) {
  test_area_alignement(Directions{}.set_up().set_right());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesUpRightLeft) {
  test_area_alignement(Directions{}.set_up().set_right().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDown) {
  test_area_alignement(Directions{}.set_down());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownLeft) {
  test_area_alignement(Directions{}.set_down().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownRight) {
  test_area_alignement(Directions{}.set_down().set_right());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownRightLeft) {
  test_area_alignement(Directions{}.set_down().set_right().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownUp) {
  test_area_alignement(Directions{}.set_down().set_up());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownUpLeft) {
  test_area_alignement(Directions{}.set_down().set_up().set_left());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownUpRight) {
  test_area_alignement(Directions{}.set_down().set_up().set_right());
}

TEST_F(RectangleGridRasterizationTest, areaAlignesDownUpRightLeft) {
  test_area_alignement(Directions{}.set_down().set_up().set_right().set_left());
}

//----------------------------------------------------------------------------//

TEST_F(RectangleGridRasterizationTest, areaInfinite) {
  grid = RegularSquaresGrid{3, 2, 1};

  constexpr auto Inf = RegularSquaresGrid::Dbl_Inf;
  auto actual = area_to_ids({0, 0}, Inf, Inf, Inf, Inf);
  auto expected = std::set<RegularSquaresGrid::Coord>{
    {-1, -1}, {-1, 0}, {0, -1},  {0, 0}, {1, -1},  {1, 0}};
  ASSERT_EQ(expected, actual);
}

//============================================================================//

int main (int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
