// __BEGIN_LICENSE__
//
// Copyright (C) 2006 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration
// (NASA).  All Rights Reserved.
// 
// This software is distributed under the NASA Open Source Agreement
// (NOSA), version 1.3.  The NOSA has been approved by the Open Source
// Initiative.  See the file COPYING at the top of the distribution
// directory tree for the complete NOSA document.
// 
// THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
// KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
// LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
// SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
// THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
// DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE.
//
// __END_LICENSE__

/// \file ImageView.h
/// 
/// Defines the core in-memory image view type.
///
#ifndef __VW_IMAGE__IMAGE_VIEW_H__
#define __VW_IMAGE__IMAGE_VIEW_H__

#include <vw/config.h>

#include <string.h> // For memset()

#include <boost/smart_ptr.hpp>
#include <boost/type_traits.hpp>

#include <vw/Core/Exception.h>

#include <vw/Image/ImageViewBase.h>
#include <vw/Image/PixelAccessors.h>

// support for conversion to and from vil_image_view
#ifdef VW_HAVE_PKG_VXL
#include <vil/vil_image_view.txx>
#include <vil/vil_copy.h>
#endif

namespace vw {

  /// The standard image container for in-memory image data.
  ///
  /// This class represents an image stored in memory or, more
  /// precisely, a view onto such an image.  That is, the ImageView
  /// object itself does not contain the image data itself but rather
  /// a pointer to it.  More than one ImageView object can point to
  /// the same data, and they can even choose to interpret that data
  /// differently.  In particular, copying an ImageView object is a
  /// shallow, lightweight operation.  The underlying image data is
  /// reference counted, so the user does not usually need to be
  /// concerned with memory allocation and deallocation.
  /// 
  /// A more complete name for this class might be something like
  /// MemoryImageView, or StandardImageView, but it is so ubiquitous
  /// that we decided to keep the name short.
  ///
  template <class PixelT>
  class ImageView : public ImageViewBase<ImageView<PixelT> >
  {
    boost::shared_array<PixelT> m_data;
    unsigned m_cols, m_rows, m_planes;
    PixelT *m_origin;
    ptrdiff_t m_cstride, m_rstride, m_pstride;
  public:

    /// The pixel type of the image.
    typedef PixelT pixel_type;

    /// The data type of the image, considered as a container.
    typedef PixelT value_type;

    /// The image's %pixel_accessor type.
    typedef MemoryStridingPixelAccessor<PixelT> pixel_accessor;

    /// Constructs an empty image with zero size.
    ImageView() : m_cols(0), m_rows(0), m_planes(0), m_origin(0), m_cstride(0), m_rstride(0), m_pstride(0) {
    }

    /// Copy-constructs a view pointing to the same data as the given ImageView.
    ImageView( ImageView const& other ) : m_data(other.m_data), m_cols(other.m_cols), m_rows(other.m_rows), m_planes(other.m_planes),
                                          m_origin(other.m_origin), m_cstride(other.m_cstride), m_rstride(other.m_rstride), m_pstride(other.m_pstride) {}

    /// Resets to an empty image with zero size.
    void reset() {
      m_data.reset();
      m_cols = m_rows = m_planes = 0;
      m_origin = 0;
      m_cstride = m_rstride = m_pstride = 0;
    }

    /// Constructs an empty image with the given dimensions.
    ImageView( unsigned cols, unsigned rows, unsigned planes=1 ) : m_cols(0), m_rows(0), m_planes(0), m_origin(0), m_cstride(0), m_rstride(0), m_pstride(0) {
      set_size( cols, rows, planes );
    }

    /// Constructs an image view and rasterizes the given view into it.
    template <class ViewT>
    ImageView( ViewT const& view ) : m_cols(0), m_rows(0), m_planes(0), m_origin(0), m_cstride(0), m_rstride(0), m_pstride(0) {
      set_size( view.cols(), view.rows(), view.planes() );
      view.rasterize( *this );
    }

    /// Rasterizes the given view into the image, adjusting the size if needed.
    template <class SrcT>
    ImageView& operator=( ImageViewBase<SrcT> const& view ) {
      set_size( view.impl().cols(), view.impl().rows(), view.impl().planes() );
      view.impl().rasterize( *this );
      return *this;
    }

    /// Rasterizes the given view into the image.
    template <class SrcT>
    ImageView const& operator=( ImageViewBase<SrcT> const& view ) const {
      view.impl().rasterize( *this );
      return *this;
    }

    value_type *data() {
      return &(operator()(0,0));
    }

    const value_type *data() const {
      return &(operator()(0,0));
    }

    /// Returns the number of columns in the image.
    inline unsigned cols() const { return m_cols; }

    /// Returns the number of rows in the image.
    inline unsigned rows() const { return m_rows; }

    /// Returns the number of planes in the image.
    inline unsigned planes() const { return m_planes; }

    /// Returns a pixel_accessor pointing to the top-left corner of the first plane.
    inline pixel_accessor origin() const {
      return pixel_accessor( m_origin, m_cstride, m_rstride, m_pstride );
    }

    /// Returns the pixel at the given position in the first plane.
    inline pixel_type& operator()( int col, int row ) const {
      return *(m_origin + col*m_cstride + row*m_rstride);
    }
  
    /// Returns the pixel at the given position in the given plane.
    inline pixel_type& operator()( int col, int row, int plane ) const {
      return *(m_origin + col*m_cstride + row*m_rstride + plane*m_pstride);
    }
  
    /// Adjusts the size of the image to match the dimensions of another image.
    template <class ImageT>
    void set_size( ImageViewBase<ImageT> &img ) {
      this->set_size(img.impl().cols(), img.impl().rows(), img.impl().planes());
    }

    /// Adjusts the size of the image, allocating a new buffer if the size has changed.
    void set_size( unsigned cols, unsigned rows, unsigned planes = 1 ) {
      if( cols==m_cols && rows==m_rows && planes==m_planes ) return;
        
      unsigned size = cols*rows*planes;
      if( size==0 ) {
        m_data.reset();
      }
      else {
        boost::shared_array<PixelT> data( new PixelT[size] );
        m_data = data;
      }

      m_cols = cols;
      m_rows = rows;
      m_planes = planes;
      m_origin = m_data.get();
      m_cstride = 1;
      m_rstride = cols;
      m_pstride = rows*cols;

      // Fundamental types might not be initialized.  Really this is
      // true of all POD types, but there's no good way to detect
      // those.  We can only hope that the user will never use a
      // custom POD pixel type.
      // 
      // Note that this is a copy of the fill algorithm that resides
      // in ImageAlgorithms.h, however including ImageAlgorithms.h
      // directly causes an include file cycle.
      if( boost::is_fundamental<pixel_type>::value ) {
        memset( m_data.get(), 0, m_rows*m_cols*m_planes*sizeof(PixelT) );
      }
    }

#ifdef VW_HAVE_PKG_VXL
    /// Constructs an image from the given VIL image.
    ImageView( vil_image_view<typename CompoundChannelType<PixelT>::type> const& src ) : 
      m_cols(0), m_rows(0), m_planes(0), m_origin(0), m_cstride(0), m_rstride(0), m_pstride(0) {
      operator=( src );
    }

    /// Copies the given VIL image into this image.
    ImageView& operator=( vil_image_view<typename CompoundChannelType<PixelT>::type> const& src ) {
      unsigned channels = CompoundNumChannels<PixelT>::value;
      if( channels != 1 && src.nplanes() != channels ) throw ArgumentErr() << "incompatible number of planes (need " << channels << ", got " << src.nplanes() << ")";
      set_size( src.ni(), src.nj(), (channels==1)?(src.nplanes()):(1) );
      vil_image_view<typename CompoundChannelType<PixelT>::type> wrapper = vil_view();
      vil_copy_reformat( src, wrapper );
      return *this;
    }
    
    /// Returns a VIL image view of this view's image data.
    vil_image_view<typename CompoundChannelType<PixelT>::type> vil_view() const {
      unsigned channels = CompoundNumChannels<PixelT>::value;
      VW_ASSERT( m_planes==1 || channels== 1, ArgumentErr() <<
                 "VIL does not support having both interleaved planes (i.e channels) and non-interleaved planes" );
      return vil_image_view<typename CompoundChannelType<PixelT>::type>( reinterpret_cast<typename CompoundChannelType<PixelT>::type*>( m_origin ),
                                                                         m_cols, m_rows, channels*m_planes, m_cstride*channels, m_rstride*channels, 
                                                                         (channels==1)?(m_pstride):(1) );
    }
#endif // VW_HAVE_PKG_VXL

    /// \cond INTERNAL
    typedef ImageView prerasterize_type;
    inline prerasterize_type prerasterize() const { return *this; }
    template <class DestT> inline void rasterize( DestT const& dest ) const { vw::rasterize( prerasterize(), dest ); }
    /// \endcond
  };

  // Image view traits
  /// \cond INTERNAL
  template <class ImageT>
  struct IsReferenceable<ImageView<ImageT> > : public boost::true_type {};

  template <class PixelT>
  struct IsResizable<ImageView<PixelT> > : public boost::true_type {};

  template <class PixelT>
  struct IsMultiplyAccessible<ImageView<PixelT> > : public boost::true_type {};
  /// \endcond

} // namespace vw

#endif /* __VW_IMAGE__IMAGE_VIEW_H__ */
