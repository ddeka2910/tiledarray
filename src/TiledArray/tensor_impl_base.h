#ifndef TILEDARRAY_TENSOR_IMPL_BASE_H__INCLUDED
#define TILEDARRAY_TENSOR_IMPL_BASE_H__INCLUDED

#include <TiledArray/error.h>
#include <TiledArray/distributed_storage.h>
#include <TiledArray/tiled_range.h>
#include <TiledArray/bitset.h>
#include <cstddef>

namespace TiledArray {
  namespace detail {

    template <typename TRange, typename Tile>
    class TensorImplBase : private NO_DEFAULTS{
    public:
      typedef std::size_t size_type;
      typedef TRange trange_type;
      typedef typename trange_type::range_type range_type;
      typedef Pmap<size_type> pmap_interface;
      typedef Tile value_type;
      typedef TiledArray::detail::DistributedStorage<value_type> storage_type;
      typedef typename storage_type::const_iterator const_iterator;
      typedef typename storage_type::iterator iterator;
      typedef typename storage_type::future const_reference;
      typedef typename storage_type::future reference;

    private:

      trange_type trange_;
      Bitset<> shape_;
      storage_type data_;

    public:

      /// Construct a unary tiled tensor op

      /// \param arg The argument
      /// \param op The element transform operation
      template <typename TR>
      TensorImplBase(madness::World& world, const TiledRange<TR>& trange, const Bitset<>& shape = Bitset<>(0ul)) :
        trange_(trange),
        shape_(shape),
        data_(world, trange_.tiles().volume())
      { }

      /// Initialize pmap

      /// \param pmap The process map
      void pmap(const std::shared_ptr<pmap_interface>& pmap) { data_.init(pmap); }

      /// Process map accessor

      /// \return Shared pointer to pmap
      const std::shared_ptr<pmap_interface>& pmap() const { return data_.get_pmap(); }

      /// Evaluate tensor to destination

      /// \tparam Dest The destination tensor type
      /// \param dest The destination to evaluate this tensor to
      template <typename Dest>
      void eval_to(Dest& dest) const {
        TA_ASSERT(trange() == dest.trange());

        // Add result tiles to dest
        typename pmap_interface::const_iterator end = data_.get_pmap()->end();
        for(typename pmap_interface::const_iterator it = data_.get_pmap()->begin(); it != end; ++it)
          if(! is_zero(*it))
            dest.set(*it, move(*it));
      }

      /// Tensor tile size array accessor

      /// \return The size array of the tensor tiles
      const range_type& range() const { return trange_.tiles(); }

      /// Tensor tile volume accessor

      /// \return The number of tiles in the tensor
      size_type size() const { return trange_.tiles().volume(); }

      /// Query a tile owner

      /// \param i The tile index to query
      /// \return The process ID of the node that owns tile \c i
      ProcessID owner(size_type i) const { return data_.owner(i); }

      /// Query for a locally owned tile

      /// \param i The tile index to query
      /// \return \c true if the tile is owned by this node, otherwise \c false
      bool is_local(size_type i) const { return data_.is_local(i); }

      /// Query for a zero tile

      /// \param i The tile index to query
      /// \return \c true if the tile is zero, otherwise \c false
      bool is_zero(size_type i) const {
        TA_ASSERT(range().includes(i));
        if(is_dense())
          return false;
        return ! (shape_[i]);
      }

      /// Tensor process map accessor

      /// \return A shared pointer to the process map of this tensor
      const std::shared_ptr<pmap_interface>& get_pmap() const { return data_.get_pmap(); }

      /// Query the density of the tensor

      /// \return \c true if the tensor is dense, otherwise false
      bool is_dense() const { return shape_.size() == 0ul; }

      /// Tensor shape accessor

      /// \return A reference to the tensor shape map
      const TiledArray::detail::Bitset<>& shape() const {
        TA_ASSERT(! is_dense());
        return shape_;
      }

      /// Set the shape
      void shape(const TiledArray::detail::Bitset<>& s) { TiledArray::detail::Bitset<>(s).swap(shape_); }

      /// Set shape values

      /// Modify the shape value for tile \c i to \c value
      /// \param i Tile index
      /// \param value The value of the tile
      /// \throw TiledArray::Exception When this tensor is dense
      void shape(size_type i, bool value = true) {
        TA_ASSERT(! is_dense());

        shape_.set(i, value);
      }

      /// Tiled range accessor

      /// \return The tiled range of the tensor
      const trange_type& trange() const { return trange_; }

      /// Set tiled range

      /// \tparam TR TiledRange type
      /// \param tr Tiled range to set
      template <typename TR>
      void trange(const TiledRange<TR>& tr) {
        trange_ = tr;
      }

      /// Tile accessor

      /// \param i The tile index
      /// \return Tile \c i
      const_reference operator[](size_type i) const {
        TA_ASSERT(! is_zero(i));
        return data_[i];
      }

      /// Tile accessor

      /// \param i The tile index
      /// \return Tile \c i
      reference operator[](size_type i) {
        TA_ASSERT(! is_zero(i));
        return data_[i];
      }

      /// Tile move

      /// Tile is removed after it is set.
      /// \param i The tile index
      /// \return Tile \c i
      const_reference move(size_type i) {
        TA_ASSERT(! is_zero(i));
        return data_.move(i);
      }


      /// Array begin iterator

      /// \return A const iterator to the first element of the array.
      const_iterator begin() const { return data_.begin(); }

      /// Array begin iterator

      /// \return A const iterator to the first element of the array.
      iterator begin() { return data_.begin(); }

      /// Array end iterator

      /// \return A const iterator to one past the last element of the array.
      const_iterator end() const { return data_.end(); }

      /// Array end iterator

      /// \return A const iterator to one past the last element of the array.
      iterator end() { return data_.end(); }

      madness::World& get_world() const { return data_.get_world(); }

      template <typename Value>
      void set(size_type i, const Value& value) { data_.set(i, value); }

      /// Clear the tile data

      /// Remove all tiles from the tensor.
      /// \note: Any tiles will remain in memory until the last reference
      /// is destroyed.
      void clear() { data_.clear(); }

    }; // class TensorImplBase

  }  // namespace detail
}  // namespace TiledArray


#endif // TILEDARRAY_TENSOR_IMPL_BASE_H__INCLUDED
