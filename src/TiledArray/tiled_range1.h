/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2013  Virginia Tech
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TILEDARRAY_TILED_RANGE1_H__INCLUDED
#define TILEDARRAY_TILED_RANGE1_H__INCLUDED

#include <TiledArray/error.h>
#include <TiledArray/type_traits.h>
#include <vector>
#include <initializer_list>
#include <mutex>

namespace TiledArray {

  /// TiledRange1 class defines a non-uniformly-tiled, contiguous, one-dimensional
  /// range. The tiling data is constructed with and stored in an array with
  /// the format {a0, a1, a2, ...}, where 0 <= a0 < a1 < a2 < ... Each tile is
  /// defined as [a0,a1), [a1,a2), ... The number of tiles in the range will be
  /// equal to one less than the number of elements in the array.
  class TiledRange1 {
  private:
    struct Enabler { };
  public:
    typedef std::size_t size_type;
    typedef std::pair<size_type, size_type> range_type;
    typedef std::vector<range_type>::const_iterator const_iterator;

    /// Default constructor, range of 0 tiles and elements.
    TiledRange1() :
        range_(0,0), elements_range_(0,0),
        tiles_ranges_(), elem2tile_()
    {
    }

    /// Constructs a range with the boundaries provided by [first, last).
    /// Start_tile_index is the index of the first tile.
    template <typename RandIter,
        typename std::enable_if<detail::is_random_iterator<RandIter>::value>::type* = nullptr>
    TiledRange1(RandIter first, RandIter last) :
        range_(), elements_range_(), tiles_ranges_(), elem2tile_()
    {
      init_tiles_(first, last, 0);
    }

    /// Copy constructor
    TiledRange1(const TiledRange1& rng) = default;

    /// Move constructor
    TiledRange1(TiledRange1&& rng) = default;

    /// Construct a 1D tiled range.

    /// This will construct a 1D tiled range with tile boundaries {t0, t_rest}
    /// The number of tile boundaries is n + 1, where n is the number of tiles.
    /// Tiles are defined as [t0, t1), [t1, t2), [t2, t3), ...
    /// \param t0 The starting index of the first tile
    /// \param t_rest The rest of tile boundaries
    template<typename... _sizes>
    explicit TiledRange1(const size_type& t0, const _sizes&... t_rest)
    {
      const size_type n = sizeof...(_sizes) + 1;
      size_type tile_boundaries[n] = {t0, static_cast<size_type>(t_rest)...};
      init_tiles_(tile_boundaries, tile_boundaries+n, 0);
    }

    /// Construct a 1D tiled range.

    /// This will construct a 1D tiled range with tile boundaries {t0, t_rest}
    /// The number of tile boundaries is n + 1, where n is the number of tiles.
    /// Tiles are defined as [t0, t1), [t1, t2), [t2, t3), ...
    /// \param list The list of tile boundaries in order from smallest to largest
    explicit TiledRange1(const std::initializer_list<size_type>& list)
    {
      init_tiles_(list.begin(), list.end(), 0);
    }

    /// Copy assignment operator
    TiledRange1& operator =(const TiledRange1& rng) = default;

    /// Move assignment operator
    TiledRange1& operator =(TiledRange1&& rng) = default;

    /// Returns an iterator to the first tile in the range.
    const_iterator begin() const { return tiles_ranges_.begin(); }

    /// Returns an iterator to the end of the range.
    const_iterator end() const { return tiles_ranges_.end(); }

    /// Return tile iterator associated with ordinal_index
    const_iterator find(const size_type& e) const{
      if(! includes(elements_range_, e))
        return tiles_ranges_.end();
      const_iterator result = tiles_ranges_.begin();
      result += element_to_tile(e);
      return result;
    }

    /// \deprecated use TiledRange1::tiles_range()
    DEPRECATED const range_type& tiles() const { return range_; }

    /// Tile range accessor

    /// \return a reference to the tile index range
    const range_type& tiles_range() const { return range_; }

    /// \deprecated use TiledRange1::elements_range()
    const range_type& elements() const { return elements_range_; }

    /// Elements range accessor

    /// \return a reference to the element index range
    const range_type& elements_range() const { return elements_range_; }

    /// Tile range extent accessor
    size_type tile_extent() const { return range_.second - range_.first; }

    /// Elements range extent accessor
    size_type extent() const { return elements_range_.second - elements_range_.first; }

    /// Tile range accessor

    /// \param i The coordinate index for the tile range to be returned
    /// \return A const reference to a the tile range for tile \c i
    /// \throw std::out_of_range When \c i \c >= \c tiles().size()
    const range_type& tile(const size_type i) const {
      TA_ASSERT(includes(range_, i));
      return tiles_ranges_[i - range_.first];
    }

    /// Maps element index to tile index

    /// \param i element index
    /// \return tile index
    /// \pre
    /// \code
    /// assert(i >= elements_range().first && i < elements_range().second);
    /// \endcode
    /// \note element->tile map is memoized, thus complexity of the first call is linear in the number of elements,
    ///       complexity of subsequent calls is constant. Note that the memoization assumes infrequent (or serial) use as it
    ///       is serialized across ALL TiledRange1 instances.
    const size_type& element_to_tile(const size_type& i) const {
      TA_ASSERT( includes(elements_range_, i) );
      if (elem2tile_.empty()) {
        init_elem2tile_();
      }
      return elem2tile_[i - elements_range_.first];
    }

    DEPRECATED const size_type& element2tile(const size_type& i) const {
      return element_to_tile(i);
    }

    void swap(TiledRange1& other) { // no throw
      std::swap(range_, other.range_);
      std::swap(elements_range_, other.elements_range_);
      std::swap(tiles_ranges_, other.tiles_ranges_);
      std::swap(elem2tile_, other.elem2tile_);
    }

  private:

    static bool includes(const range_type& r, size_type i) { return (i >= r.first) && (i < r.second); }

    /// Validates tile_boundaries
    template <typename RandIter>
    static void valid_(RandIter first, RandIter last) {
      // Verify at least 2 elements are present if the vector is not empty.
      TA_USER_ASSERT((std::distance(first, last) >= 2),
          "TiledRange1 construction failed: You need at least 2 elements in the tile boundary list.");
      // Verify the requirement that a0 < a1 < a2 < ...
      for (; first != (last - 1); ++first)
        TA_USER_ASSERT(*first < *(first + 1),
            "TiledRange1 construction failed: Invalid tile boundary, tile boundary i must be greater than tile boundary i+1 for all i. ");
    }

    /// Initialize tiles use a set of tile offsets
    template <typename RandIter>
    void init_tiles_(RandIter first, RandIter last, size_type start_tile_index) {
#ifndef NDEBUG
      valid_(first, last);
#endif // NDEBUG
      range_.first = start_tile_index;
      range_.second = start_tile_index + last - first - 1;
      elements_range_.first = *first;
      elements_range_.second = *(last - 1);
      for (; first != (last - 1); ++first)
        tiles_ranges_.emplace_back(*first, *(first + 1));
    }

    /// Initialize elem2tile
    void init_elem2tile_() const {
      // check for 0 size range.
      if((elements_range_.second - elements_range_.first) == 0)
        return;

      static std::mutex mtx;
      {
        std::lock_guard<std::mutex> lock(mtx);
        if (elem2tile_.empty()) {
          // initialize elem2tile map
          elem2tile_.resize(elements_range_.second - elements_range_.first);
          const size_type end = range_.second - range_.first;
          for (size_type t = 0; t < end; ++t)
            for (size_type e = tiles_ranges_[t].first; e < tiles_ranges_[t].second; ++e)
              elem2tile_[e - elements_range_.first] = t + range_.first;
        }
      }
    }

    friend std::ostream& operator <<(std::ostream&, const TiledRange1&);

    // TiledRange1 data
    range_type range_; ///< the range of tile indices
    range_type elements_range_; ///< the range of element indices
    std::vector<range_type> tiles_ranges_; ///< ranges of each tile.
    mutable std::vector<size_type> elem2tile_; ///< maps element index to tile index (memoized data).

  }; // class TiledRange1

  /// Exchange the data of the two given ranges.
  inline void swap(TiledRange1& r0, TiledRange1& r1) { // no throw
    r0.swap(r1);
  }

  /// Equality operator
  inline bool operator ==(const TiledRange1& r1, const TiledRange1& r2) {
    return std::equal(r1.begin(), r1.end(), r2.begin()) &&
        (r1.tiles_range() == r2.tiles_range()) && (r1.elements_range() == r2.elements_range());
  }

  /// Inequality operator
  inline bool operator !=(const TiledRange1& r1, const TiledRange1& r2){
    return ! operator ==(r1, r2);
  }

  /// TiledRange1 ostream operator
  inline std::ostream& operator <<(std::ostream& out, const TiledRange1& rng) {
    out << "( tiles = [ " << rng.tiles_range().first << ", " << rng.tiles_range().second
        << " ), elements = [ " << rng.elements_range().first << ", " << rng.elements_range().second << " ) )";
    return out;
  }

} // namespace TiledArray

#endif // TILEDARRAY_TILED_RANGE1_H__INCLUDED
