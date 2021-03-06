/*=========================================================================

  Program:   Diffusion Applications
  Module:    $HeadURL$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Brigham and Women's Hospital (BWH) All Rights Reserved.

  See License.txt or http://www.slicer.org/copyright/copyright.txt for details.

==========================================================================*/
#ifndef __itkDiffusionTensor3DInterpolateImageFunction_h
#define __itkDiffusionTensor3DInterpolateImageFunction_h

#include <itkObject.h>
#include "itkDiffusionTensor3D.h"
#include <itkOrientedImage.h>
#include <itkPoint.h>
//#include <itkSemaphore.h>
//#include <itkNumericTraits.h>
//#include "define.h"
#include <itkImageFunction.h>

namespace itk
{
/**
 * \class DiffusionTensor3DInterpolateImageFunction
 * 
 * Virtual class to implement diffusion tensor interpolation classes 
 * 
 */
template< class TData , class TCoordRep = double >
class DiffusionTensor3DInterpolateImageFunction :
   public ImageFunction< OrientedImage< DiffusionTensor3D < TData > , 3 > ,
                         DiffusionTensor3D < TData > ,
                         TCoordRep
                       >
{
public :
  typedef TData TensorType ;
  typedef DiffusionTensor3DInterpolateImageFunction Self ;
  typedef DiffusionTensor3D< TensorType > TensorDataType ;
  typedef OrientedImage< TensorDataType , 3 > DiffusionImageType ;
  typedef typename DiffusionImageType::Pointer DiffusionImageTypePointer ;
  typedef Point< double , 3 > PointType ;
  typedef SmartPointer< Self > Pointer ;
  typedef SmartPointer< const Self > ConstPointer ;
  typedef typename TensorDataType::RealValueType TensorRealType ;
  typedef ImageFunction< OrientedImage< DiffusionTensor3D < TData > , 3 > ,
                         DiffusionTensor3D < TData > ,
                         TCoordRep
                       > Superclass ;
  typedef typename Superclass::ContinuousIndexType ContinuousIndexType ;
  typedef typename Superclass::IndexType IndexType ;

/////Copied from itkInterpolateImageFunction.h

  /** Interpolate the image at a point position
   *
   * Returns the interpolated image intensity at a 
   * specified point position. No bounds checking is done.
   * The point is assume to lie within the image buffer.
   *
   * ImageFunction::IsInsideBuffer() can be used to check bounds before
   * calling the method. */
virtual TensorDataType Evaluate( const PointType& point ) const
{
  ContinuousIndexType index;
  this->GetInputImage()->TransformPhysicalPointToContinuousIndex( point, index ) ;
  return ( this->EvaluateAtContinuousIndex( index ) ) ;
}
  /** Interpolate the image at a continuous index position
   *
   * Returns the interpolated image intensity at a 
   * specified index position. No bounds checking is done.
   * The point is assume to lie within the image buffer.
   *
   * Subclasses must override this method.
   *
   * ImageFunction::IsInsideBuffer() can be used to check bounds before
   * calling the method. */
  virtual TensorDataType EvaluateAtContinuousIndex( const ContinuousIndexType & index ) const = 0 ;
  /** Interpolate the image at an index position.
   *
   * Simply returns the image value at the
   * specified index position. No bounds checking is done.
   * The point is assume to lie within the image buffer.
   *
   * ImageFunction::IsInsideBuffer() can be used to check bounds before
   * calling the method. */

virtual TensorDataType EvaluateAtIndex( const IndexType & index ) const
{
  return this->GetInputImage()->GetPixel( index ) ;
}
//  void SetDefaultPixelValue( TensorRealType defaultPixelValue ) ;
//  itkGetMacro( DefaultPixelValue , TensorRealType ) ;
protected:
  DiffusionTensor3DInterpolateImageFunction() ;
  unsigned long latestTime ;
//  TensorRealType m_DefaultPixelValue ;
//  TensorDataType m_DefaultPixel ;
};

}//end namespace itk
#ifndef ITK_MANUAL_INSTANTIATION
#include "itkDiffusionTensor3DInterpolateImageFunction.txx"
#endif

#endif
