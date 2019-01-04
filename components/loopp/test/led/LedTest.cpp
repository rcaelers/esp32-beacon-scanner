#include <stdexcept>
#include <string>
#include <iostream>

#include "unity.h"
#include "esp_log.h"

#include "loopp/led/LedMatrix.hpp"

static void check(loopp::led::LedMatrix &matrix, int leds[][5])
{
  for (uint16_t y = 0; y < 6; y++)
    {
      for (uint16_t x = 0; x < 5; x++)
        {
          uint16_t l = matrix.convert_xy(x, y);
          std::cout << x << "," << y << " -> " << l  << " exptect " << leds[y][x] << std::endl;
          TEST_ASSERT(leds[y][x] == l);
        }
    }
}

TEST_CASE("Matrix to LED mapping: TopLeft/Horizontal/Progressive", "[cxx]")
{
  int leds[][5] = {
                    {  0,  1,  2,  3,  4 },
                    {  5,  6,  7,  8,  9 },
                    { 10, 11, 12, 13, 14 },
                    { 15, 16, 17, 18, 19 },
                    { 20, 21, 22, 23, 24 },
                    { 25, 26, 27, 28, 29 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopLeft, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomLeft/Horizontal/Progressive", "[cxx]")
{
  int leds[][5] = {
                   { 25, 26, 27, 28, 29 },
                   { 20, 21, 22, 23, 24 },
                   { 15, 16, 17, 18, 19 },
                   { 10, 11, 12, 13, 14 },
                   {  5,  6,  7,  8,  9 },
                   {  0,  1,  2,  3,  4 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomLeft, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopRight/Horizontal/Progressive", "[cxx]")
{
  int leds[][5] = {
                    {  4,  3,  2,  1,  0 },
                    {  9,  8,  7,  6,  5 },
                    { 14, 13, 12, 11, 10 },
                    { 19, 18, 17, 16, 15 },
                    { 24, 23, 22, 21, 20 },
                    { 29, 28, 27, 26, 25 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopRight, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomRight/Horizontal/Progressive", "[cxx]")
{
  int leds[][5] = {
                    { 29, 28, 27, 26, 25 },
                    { 24, 23, 22, 21, 20 },
                    { 19, 18, 17, 16, 15 },
                    { 14, 13, 12, 11, 10 },
                    {  9,  8,  7,  6,  5 },
                    {  4,  3,  2,  1,  0 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomRight, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopLeft/Vertical/Progressive", "[cxx]")
{
  int leds[][5] = {
                    {  0,  6, 12, 18, 24 },
                    {  1,  7, 13, 19, 25 },
                    {  2,  8, 14, 20, 26 },
                    {  3,  9, 15, 21, 27 },
                    {  4, 10, 16, 22, 28 },
                    {  5, 11, 17, 23, 29 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopLeft, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomLeft/Vertical/Progressive", "[cxx]")
{
  int leds[][5] = {
                   {  5, 11, 17, 23, 29 },
                   {  4, 10, 16, 22, 28 },
                   {  3,  9, 15, 21, 27 },
                   {  2,  8, 14, 20, 26 },
                   {  1,  7, 13, 19, 25 },
                   {  0,  6, 12, 18, 24 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomLeft, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopRight/Vertical/Progressive", "[cxx]")
{
  int leds[][5] = {
                   {  24, 18, 12,  6,  0 },
                   {  25, 19, 13,  7,  1 },
                   {  26, 20, 14,  8,  2 },
                   {  27, 21, 15,  9,  3 },
                   {  28, 22, 16, 10,  4 },
                   {  29, 23, 17, 11,  5 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopRight, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomRight/Vertical/Progressive", "[cxx]")
{
  int leds[][5] = {
                   {  29, 23, 17, 11,  5 },
                   {  28, 22, 16, 10,  4 },
                   {  27, 21, 15,  9,  3 },
                   {  26, 20, 14,  8,  2 },
                   {  25, 19, 13,  7,  1 },
                   {  24, 18, 12,  6,  0 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomRight, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::Progressive);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopLeft/Horizontal/ZigZag", "[cxx]")
{
  int leds[][5] = {
                    {  0,  1,  2,  3,  4 },
                    {  9,  8,  7,  6,  5 },
                    { 10, 11, 12, 13, 14 },
                    { 19, 18, 17, 16, 15 },
                    { 20, 21, 22, 23, 24 },
                    { 29, 28, 27, 26, 25 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopLeft, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomLeft/Horizontal/ZigZag", "[cxx]")
{
  int leds[][5] = {
                   { 29, 28, 27, 26, 25 },
                   { 20, 21, 22, 23, 24 },
                   { 19, 18, 17, 16, 15 },
                   { 10, 11, 12, 13, 14 },
                   {  9,  8,  7,  6,  5 },
                   {  0,  1,  2,  3,  4 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomLeft, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopRight/Horizontal/ZigZag", "[cxx]")
{
  int leds[][5] = {
                    {  4,  3,  2,  1,  0 },
                    {  5,  6,  7,  8,  9 },
                    { 14, 13, 12, 11, 10 },
                    { 15, 16, 17, 18, 19 },
                    { 24, 23, 22, 21, 20 },
                    { 25, 26, 27, 28, 29 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopRight, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomRight/Horizontal/ZigZag", "[cxx]")
{
  int leds[][5] = {
                    { 25, 26, 27, 28, 29 },
                    { 24, 23, 22, 21, 20 },
                    { 15, 16, 17, 18, 19 },
                    { 14, 13, 12, 11, 10 },
                    {  5,  6,  7,  8,  9 },
                    {  4,  3,  2,  1,  0 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomRight, loopp::led::LedMatrix::Direction::Horizontal, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopLeft/Vertical/ZigZag", "[cxx]")
{
  int leds[][5] = {
                    {  0, 11, 12, 23, 24 },
                    {  1, 10, 13, 22, 25 },
                    {  2,  9, 14, 21, 26 },
                    {  3,  8, 15, 20, 27 },
                    {  4,  7, 16, 19, 28 },
                    {  5,  6, 17, 18, 29 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopLeft, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomLeft/Vertical/ZigZag", "[cxx]")
{
  int leds[][5] = {
                   {  5,  6, 17, 18, 29 },
                   {  4,  7, 16, 19, 28 },
                   {  3,  8, 15, 20, 27 },
                   {  2,  9, 14, 21, 26 },
                   {  1, 10, 13, 22, 25 },
                   {  0, 11, 12, 23, 24 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomLeft, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: TopRight/Vertical/ZigZag", "[cxx]")
{
  int leds[][5] = {
                   {  24, 23, 12, 11,  0 },
                   {  25, 22, 13, 10,  1 },
                   {  26, 21, 14,  9,  2 },
                   {  27, 20, 15,  8,  3 },
                   {  28, 19, 16,  7,  4 },
                   {  29, 18, 17,  6,  5 }
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::TopRight, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}

TEST_CASE("Matrix to LED mapping: BottomRight/Vertical/ZigZag", "[cxx]")
{
  int leds[][5] = {
                   {  29, 18, 17,  6,  5 },
                   {  28, 19, 16,  7,  4 },
                   {  27, 20, 15,  8,  3 },
                   {  26, 21, 14,  9,  2 },
                   {  25, 22, 13, 10,  1 },
                   {  24, 23, 12, 11,  0 },
  };

  loopp::led::LedMatrix matrix(5, 6, loopp::led::LedMatrix::Origin::BottomRight, loopp::led::LedMatrix::Direction::Vertical, loopp::led::LedMatrix::Sequence::ZigZag);
  check(matrix, leds);
}


