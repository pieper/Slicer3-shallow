/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageStash.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
///  vtkImageStash - 
///  Store an image data in a compressed form to save memory

#ifndef __vtkImageStash_h
#define __vtkImageStash_h

#include "vtkSlicerBaseLogic.h"

#include "vtkImageData.h"

class vtkUnsignedCharArray;
class vtkZLibDataCompressor;

class VTK_SLICER_BASE_LOGIC_EXPORT vtkImageStash : public vtkObject
{
public:
  static vtkImageStash *New();
  vtkTypeRevisionMacro(vtkImageStash,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// 
  /// The stash image: 
  /// This image data will have the Scalars removed
  /// and they will be stored in a local compressed data
  /// array inside this class when the Stash method is called.
  /// You must call Unstash to have the scalars put back into
  /// this image data.
  vtkSetObjectMacro(StashImage, vtkImageData);
  vtkGetObjectMacro(StashImage, vtkImageData);

  /// 
  /// compress and strip the scalars
  void Stash();

  /// 
  /// decompress and restore the scalars
  void Unstash();
 
protected:
  vtkImageStash();
  ~vtkImageStash();

  vtkImageData *StashImage;
  vtkUnsignedCharArray *StashedScalars;
  vtkZLibDataCompressor *Compressor;

private:
  vtkIdType NumberOfTuples;

  vtkImageStash(const vtkImageStash&);  /// Not implemented.
  void operator=(const vtkImageStash&);  /// Not implemented.
};



#endif



