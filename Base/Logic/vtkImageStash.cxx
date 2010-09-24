/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkImageStash.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStash.h"

#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkZLibDataCompressor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkImageStash, "$Revision: 12690 $");
vtkStandardNewMacro(vtkImageStash);

//----------------------------------------------------------------------------
vtkImageStash::vtkImageStash()
{
  this->StashImage = NULL;
  this->StashedScalars = NULL;
  this->Compressor = vtkZLibDataCompressor::New();
}

//----------------------------------------------------------------------------
vtkImageStash::~vtkImageStash()
{
  this->SetStashImage (NULL);
  if (this->StashedScalars)
    {
    this->StashedScalars->Delete();
    }
  this->Compressor->Delete();
}

//----------------------------------------------------------------------------
void vtkImageStash::Stash()
{
  //
  // put a compressed version of the scalars into the compressed 
  // buffer, and then set the scalar size to zero
  //
  if (!this->StashImage)
    {
    vtkErrorMacro ("Cannot stash - no image data");
    return;
    }

  vtkDataArray *scalars = this->StashImage->GetPointData()->GetScalars();  
  if (!scalars)
    {
    vtkErrorMacro ("Cannot stash - image has no scalars");
    return;
    }

  this->NumberOfTuples = scalars->GetNumberOfTuples();
  vtkIdType numPrims = this->NumberOfTuples * scalars->GetNumberOfComponents();
  vtkIdType size = vtkDataArray::GetDataTypeSize(scalars->GetDataType());
  vtkIdType scalarSize = size * numPrims;

  if (this->StashedScalars)
    {
    this->StashedScalars->Delete();
    }
  unsigned char *p = static_cast<unsigned char *>(scalars->WriteVoidPointer(0, numPrims));
  this->StashedScalars = this->Compressor->Compress(p, scalarSize);

  // this will realloc a zero sized buffer
  scalars->SetNumberOfTuples(0);
  scalars->Squeeze();
}

//----------------------------------------------------------------------------
void vtkImageStash::Unstash()
{
  //
  // put the decompressed values back into the scalar array
  //  - note: image data cannot have been touched since calling Stash
  //
  if (!this->StashImage)
    {
    vtkErrorMacro ("Cannot unstash - no image data");
    return;
    }

  vtkDataArray *scalars = this->StashImage->GetPointData()->GetScalars();  
  if (!scalars)
    {
    vtkErrorMacro ("Cannot unstash - image has no scalars");
    return;
    }

  if (!this->StashedScalars)
    {
    vtkErrorMacro ("Cannot unstash - nothing in the stash");
    return;
    }

  // we saved the original number of tuples before squeezing
  //   - the number of components and the datatype are unchanged from before
  //     so we know the right size for the output buffer
  vtkIdType numPrims = this->NumberOfTuples * scalars->GetNumberOfComponents();
  vtkIdType size = vtkDataArray::GetDataTypeSize(scalars->GetDataType());
  vtkIdType scalarSize = size * numPrims;

  // setting the number of tuples reallocates the right amount of data
  // so we can uncompress directly into the buffer
  scalars->SetNumberOfTuples(this->NumberOfTuples);
  vtkIdType stashedSize = this->StashedScalars->GetNumberOfTuples();
  unsigned char *scalar_p = 
      static_cast<unsigned char *>(scalars->WriteVoidPointer(0, numPrims));
  unsigned char *stash_p = 
      static_cast<unsigned char *>(this->StashedScalars->WriteVoidPointer(0, stashedSize));
  this->Compressor->Uncompress(stash_p, stashedSize, scalar_p, scalarSize);
}

//----------------------------------------------------------------------------
void vtkImageStash::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "StashImage: " << this->GetStashImage() << "\n";
  os << indent << "Stashed Scalars: " << this->StashedScalars << "\n";
  if ( this->StashedScalars) this->StashedScalars->PrintSelf(os,indent);
  os << indent << "Compressor: \n";
  this->Compressor->PrintSelf(os,indent);
}

