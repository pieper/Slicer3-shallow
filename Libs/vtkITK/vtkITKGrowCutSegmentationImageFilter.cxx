/**********************************************************************
 * vtkItkGrowCutSegmentationImageFilter
 * Implements wrapper for the itkGrowCutSegmentationImageFilter 
 * This implemnents n-class segmentation
 **********************************************************************/

#include "vtkITKGrowCutSegmentationImageFilter.h"
#include "itkGrowCutSegmentationImageFilter.h"

#include "vtkObjectFactory.h"

#include "vtkPoints.h"
//#include "vtkPointData.h"
#include "vtkImageData.h"

#include "itkImage.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageFileWriter.h"

#include "vtkProcessObject.h"

#include "itkEventObject.h"
#include "itkCommand.h"

#include <vcl_string.h>

#include <iostream>


vtkCxxRevisionMacro(vtkITKGrowCutSegmentationImageFilter, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkITKGrowCutSegmentationImageFilter);


////////// These types are not defined in itk ////////////
#ifdef vtkTemplateMacroCase_ui64
#undef vtkTemplateMacroCase_ui64
# define vtkTemplateMacroCase_ui64(typeN, type, call)
#endif
#ifdef vtkTemplateMacroCase_si64
#undef vtkTemplateMacroCase_si64
# define vtkTemplateMacroCase_si64(typeN, type, call)
#endif
#ifdef vtkTemplateMacroCase_ll
#undef vtkTemplateMacroCase_ll
# define vtkTemplateMacroCase_ll(typeN, type, call)
#endif



// Local Function: not method. 
void vtkITKImageGrowCutHandleProgressEvent(itk::Object *caller, 
                                           const itk::EventObject& vtkNotUsed(eventObject),
                                           void *clientdata)
{

    itk::ProcessObject *itkFilter = static_cast<itk::ProcessObject*>(caller);
    vtkProcessObject *vtkFilter = static_cast<vtkProcessObject*>(clientdata);
    if (itkFilter && vtkFilter )
    {
      vtkFilter->UpdateProgress( itkFilter->GetProgress() );
    }
};


//// 3D filter
 template<class IT1, class IT2, class OT>
void vtkITKImageGrowCutExecute3D(vtkImageData *inData, 
  IT1 *inPtr1, IT2 *inPtr2, IT2 *inPtr3,
  OT *output, int outExt[6], double &ObjectSize,
  double &ContrastNoiseRatio, 
  double &PriorSegmentStrength,     
  vtkPoints *GestureColors, 
                itk::CStyleCommand::Pointer progressCommand)
{

  std::ofstream debugger;
  //debugger.open("D:/projects/namic2010/debuglog.txt", std::ios::out|std::ios::app);
  //  debugger.open("D:/projects/namic2010/debuglog.txt");

  //  std::cout<<" In ExecuteData Method L4"<<std::endl;  

  typedef itk::Image<IT1, 3> InImageType;
  typename InImageType::Pointer image = InImageType::New();

  typedef itk::Image<IT2, 3> LabelImageType;
  typedef itk::Image<IT2, 3> SegImageType;

  typename LabelImageType::Pointer labelImage = LabelImageType::New();
  
  typename SegImageType::Pointer prevSegmentedImage = SegImageType::New();

  typename LabelImageType::Pointer outputImage = LabelImageType::New();

  typedef itk::Image<float, 3> WeightImageType;
  typename WeightImageType::Pointer weightImage = WeightImageType::New();

  typedef itk::Image<IT2, 3> OutImageType;
  //typedef itk::ImageFileWriter < OutImageType > WriterType;
  //WriterType::Pointer writer = WriterType::New();
 

 
  int dims[3];
  int extent[6];
  double spacing[3], origin[3];

  inData->GetDimensions(dims);
  inData->GetExtent(extent);
  inData->GetOrigin(origin);
  inData->GetSpacing(spacing);

  image->SetOrigin( origin );
  image->SetSpacing( spacing );
            
  typename InImageType::RegionType region;
  typename InImageType::IndexType index;
  typename InImageType::SizeType size;
  index[0] = extent[0];   
  index[1] = extent[2];
  index[2] = extent[4];
  region.SetIndex( index );
  size[0] = extent[1] - extent[0] + 1;
  size[1] = extent[3] - extent[2] + 1;
  size[2] = extent[5] - extent[4] + 1;
  region.SetSize( size );
  image->SetRegions(region);

  image->GetPixelContainer()->SetImportPointer(inPtr1, dims[0]*dims[1]*dims[2], false);

  labelImage->SetOrigin( origin );
  labelImage->SetSpacing( spacing );
  labelImage->SetRegions( region );
  labelImage->GetPixelContainer()->SetImportPointer(inPtr2, dims[0]*dims[1]*dims[2], false);

  prevSegmentedImage->SetOrigin( origin );
  prevSegmentedImage->SetSpacing( spacing );
  prevSegmentedImage->SetRegions( region );
  prevSegmentedImage->GetPixelContainer()->SetImportPointer(inPtr3, dims[0]*dims[1]*dims[2], false);
  
  weightImage->CopyInformation(image);  
  weightImage->SetBufferedRegion( image->GetBufferedRegion() );
  weightImage->Allocate();
  weightImage->FillBuffer( 0 );       
  
  itk::ImageRegionIterator< WeightImageType > weight(weightImage, weightImage->GetBufferedRegion() );
  itk::ImageRegionConstIterator< LabelImageType > label(labelImage, labelImage->GetBufferedRegion() );

  itk::ImageRegionConstIterator< SegImageType > plabel(prevSegmentedImage, 
    prevSegmentedImage->GetBufferedRegion() );

  if(ContrastNoiseRatio > 1.0) 
   ContrastNoiseRatio /= 100.0;
  
  if(PriorSegmentStrength > 1.0)
    PriorSegmentStrength /= 100.0;

  vtkIdType ngestures = GestureColors->GetNumberOfPoints();
  typename LabelImageType::PixelType backgroundColor = 0;
  // debugger<<" number of gestures "<<ngestures<<std::endl;
  std::cout<<" number of gestures "<<ngestures<<std::endl;

  for (vtkIdType i = 0; i < ngestures; i++)
  {
    double *p = GestureColors->GetPoint(i);
    //debugger<<" Gesture Color "<<i<<" : "<<p[0]<<std::endl;
    std::cout<<" Gesture Color "<<i<<" : "<<p[0]<<std::endl;
    if(p[0] > backgroundColor)
      backgroundColor = (typename LabelImageType::PixelType)p[0];
  }
  //debugger<<" Background Label Color "<<backgroundColor<<std::endl;
  std::cout<<" Background Label Color "<<backgroundColor<<std::endl;
  int numForegroundPixels = 0;
  int numBackgroundPixels = 0;

  for(weight.GoToBegin(), label.GoToBegin(); !weight.IsAtEnd(); 
       ++weight, ++label)
    {

      typename LabelImageType::PixelType color = label.Get();
      if(color == 0)
      {
         weight.Set(0.0);
      }
      else{
        numBackgroundPixels += ( color == backgroundColor ) ? 1 : 0;
        numForegroundPixels += ( color != backgroundColor ) ? 1 : 0;

        weight.Set( ContrastNoiseRatio );
      }

    }
  std::cout<<" number Foreground Pixels : "<<numForegroundPixels<<std::endl;
  std::cout<<" number Background Pixels : "<<numBackgroundPixels<<std::endl;
  
  for(weight.GoToBegin(), plabel.GoToBegin(); !weight.IsAtEnd(); 
      ++weight, ++plabel)
    {
      typename LabelImageType::PixelType color = plabel.Get();
      if(color != 0 && weight.Get() == 0.0)
 {
   weight.Set( PriorSegmentStrength);
 }
    }

  typedef itk::GrowCutSegmentationImageFilter<InImageType, OutImageType> FilterType;
  typename FilterType::Pointer filter = FilterType::New();
  
  filter->AddObserver(itk::ProgressEvent(), progressCommand );

  filter->SetInput( image );
  filter->SetLabelImage( labelImage );
  filter->SetStrengthImage( weightImage );

  filter->SetSeedStrength( ContrastNoiseRatio );
  //filter->SetUseSlow(true);
  //filter->SetMaxIterations((unsigned int)MaxIterations);
  filter->SetObjectRadius((unsigned int)ObjectSize);
  
  filter->Update();
  
  /*writer->SetInput(filter->GetOutput());
  writer->Update();*/
  //debugger<<"Done with filter output "<<std::endl;
  //debugger<<"Done writing output of the filter "<<std::endl;
  
  int segmentedVol = 0;
 
  outputImage = filter->GetOutput();
  itk::ImageRegionIterator< LabelImageType > out(outputImage, outputImage->GetBufferedRegion() );
  for (out.GoToBegin(); !out.IsAtEnd(); ++out)
    {
      if(out.Get() == backgroundColor)
 {
   continue;
   // out.Set(0);
 }
      else if(out.Get() != 0)
 {
   ++segmentedVol; // this is valid only for a binary segmentation
 }
    }

  std::cout<<"Segmented Volume : "<<segmentedVol<<" pixels"<<std::endl;
 // Copy to the output
 /*memcpy(output, filter->GetOutput()->GetBufferPointer(),
         filter->GetOutput()->GetBufferedRegion().GetNumberOfPixels()*sizeof(OT) );
 */
 memcpy(output, outputImage->GetBufferPointer(),
         outputImage->GetBufferedRegion().GetNumberOfPixels()*sizeof(OT) );
 

 // debugger.close();

}



//constructor
vtkITKGrowCutSegmentationImageFilter::vtkITKGrowCutSegmentationImageFilter()
{
  //MaxIterations = 5;
  ObjectSize = 10;
  GestureColors = NULL;
  ContrastNoiseRatio = 1.0;
  CnrThreshold = 0.0;
  PriorSegmentConfidence = 0.0000001;
  
}



template< class IT1, class IT2, class OT >
void ExecuteGrowCut( vtkITKGrowCutSegmentationImageFilter *self, 
          vtkImageData *input1, 
          vtkImageData *input2, 
          vtkImageData *input3,  
          vtkImageData *outData, 
          IT1 *, IT2 *, OT*)
//void ExecuteGrowCut( vtkITKGrowCutSegmentationImageFilter *self, 
//          vtkImageData *input1, 
//          vtkImageData *input2, 
//          vtkImageData *outData, 
//          IT1 *, IT2 *, OT*)

{

  std::cout<<" In ExecuteData Method L3"<<std::endl;  

  int outExt[6];
  int dims[3];
    double spacing[3], origin[3];

  input1->GetDimensions(dims);
  input1->GetOrigin(origin);
  input1->GetSpacing(spacing);
  input1->GetExtent(outExt);

  void *inPtr1 = input1->GetScalarPointerForExtent(outExt);
  void *inPtr2 =  input2->GetScalarPointerForExtent(outExt);
  void *inPtr3 = input3->GetScalarPointerForExtent(outExt);

  input1->GetWholeExtent(outExt);
  outData->SetExtent(outExt);
  outData->SetOrigin(origin);
  outData->SetSpacing(spacing);
  outData->SetDimensions(dims);
  outData->AllocateScalars();

  input1->GetExtent(outExt);

  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  // set up progress callback
  itk::CStyleCommand::Pointer progressCommand = itk::CStyleCommand::New();
  progressCommand->SetClientData(static_cast<void *>(self));
  progressCommand->SetCallback(vtkITKImageGrowCutHandleProgressEvent );


  vtkITKImageGrowCutExecute3D(input1, 
         (IT1*)(inPtr1), (IT2*)(inPtr2), (IT2*) (inPtr3),
         (OT*)(outPtr), outExt,
         self->ObjectSize, self->ContrastNoiseRatio, 
         self->PriorSegmentConfidence, 
         self->GestureColors, 
         progressCommand); 


  //vtkITKImageGrowCutExecute3D(input1, 
  //    (IT1*)(inPtr1), (IT2*)(inPtr2),
  //    (OT*)(outPtr), outExt,
  //    self->MaxIterations, self->ContrastNoiseRatio, 
  //    self->PriorSegmentConfidence, 
  //          self->GestureColors); 
  
  return;
}





template< class IT1, class IT2 >
void ExecuteGrowCut( vtkITKGrowCutSegmentationImageFilter *self, 
          vtkImageData *input1, 
          vtkImageData *input2, 
          vtkImageData *input3, 
          vtkImageData *outData, 
          IT1 *, IT2 *)
//void ExecuteGrowCut( vtkITKGrowCutSegmentationImageFilter *self, 
//          vtkImageData *input1, 
//          vtkImageData *input2, 
//          vtkImageData *outData, 
//          IT1 *, IT2 *)
{
  
  
  switch (outData->GetScalarType() )
    {
      
      vtkTemplateMacro( ExecuteGrowCut( self, input1, input2, input3, outData, 
          static_cast<IT1 *>(0), static_cast<IT2 *>(0), 
          static_cast<VTK_TT *>(0)) );
     //vtkTemplateMacro( ExecuteGrowCut( self, input1, input2, outData, 
          //static_cast<IT1 *>(0), static_cast<IT2 *>(0), 
          //static_cast<VTK_TT *>(0)) );
    break;
    }
  
  return;
}


template< class IT1 >
void ExecuteGrowCut( vtkITKGrowCutSegmentationImageFilter *self, 
          vtkImageData *input1, 
          vtkImageData *input2, 
          vtkImageData *input3, 
          vtkDataObject *outData, 
          IT1 *)
//void ExecuteGrowCut( vtkITKGrowCutSegmentationImageFilter *self, 
//          vtkImageData *input1, 
//          vtkImageData *input2, 
//          vtkDataObject *outData, 
//          IT1 *)
{
  std::cout<<" In ExecuteData Method L1"<<std::endl;  
  
  switch (input2->GetScalarType() )
  {
    vtkTemplateMacro( ExecuteGrowCut( self, input1, input2, input3, 
      vtkImageData::SafeDownCast(outData), 
      static_cast<IT1 *>(0), static_cast<VTK_TT *>(0)) );
    //
    //vtkTemplateMacro( ExecuteGrowCut( self, input1, input2, 
    //  vtkImageData::SafeDownCast(outData), 
    //  static_cast<IT1 *>(0), static_cast<VTK_TT *>(0)) );
    break;
  }

  return;
}


void vtkITKGrowCutSegmentationImageFilter::ExecuteData(
        vtkDataObject *outData)
{

  std::cout<<" In ExecuteData Method "<<std::endl;  
  vtkImageData *input1 = GetInput(0);
  vtkImageData *input2 = GetInput(1);
  vtkImageData *input3 = GetInput(2);

  vtkImageData * out = vtkImageData::SafeDownCast(outData);
  out->SetScalarType(input2->GetScalarType());
  input2->SetScalarType( input3->GetScalarType() );

  switch(input1->GetScalarType() )
  {
    
    //vtkTemplateMacro( ExecuteGrowCut( this, input1, input2, outData,
    //  static_cast<VTK_TT *>(0)) );
    vtkTemplateMacro( ExecuteGrowCut( this, input1, input2, input3, out,
      static_cast<VTK_TT *>(0)) );
    break;
  }

  return;

}


void vtkITKGrowCutSegmentationImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Object Size : " << ObjectSize<< std::endl;
//  os << indent << "Number classes : "<< GestureAttributes->GetNumberOfPoints()<<std::endl;
  os << indent << "cnr : "<<ContrastNoiseRatio<<std::endl;

}
