/*=========================================================================

  Program:   SparseFieldLevelSetContour
  Module:    $HeadURL$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Brigham and Women's Hospital (BWH) All Rights Reserved.

  See License.txt or http://www.slicer.org/copyright/copyright.txt for details.

==========================================================================*/
#include "SparseFieldLevelSetContourCLP.h"
#include <iostream>
#include <vector>
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPolyDataWriter.h"
#include "vtkPolyData.h"
#include "vtkPluginFilterWatcher.h"
#include "ModuleEntry.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"

using namespace MeshContourEvolver;

int run_test(vtkPolyData* polyDataOutput, vtkPolyData* polyDataBaseline)
{
  std::cerr<<"Running MeshContourEvolver Test Function... \n";
  // polyDataBaseline, polyDataOutput

  if( polyDataOutput->GetNumberOfPoints() != polyDataBaseline->GetNumberOfPoints() )
  {
    return 0;
  } 
  else 
  {
    //for(int i=0; i<polyDataOutput.GetNumberOfPoints(); i++)
    {
      if ( (polyDataOutput->GetPointData()->GetArray("activeContourVertIdx") == NULL) ||
            (polyDataBaseline->GetPointData()->GetArray("activeContourVertIdx") == NULL ) )
      {
        return 0;
      }
    }
    return 1;
  }
}

int main(int argc, char* argv[] )
{
  std::cout<<"Starting...\n";
  PARSE_ARGS;

  std::cout<<"Output model: "<<OutputModel.c_str()<<"\n";
  std::cout<<"Length of contour seeds: "<<ContourSeedPts.size()<<"\n";
  std::cout << "Evolution iterations: " << evolve_its<<"\n";
  std::cout << "Mesh smoothing iterations: " << mesh_smooth_its<<"\n";
  std::cout << "Curvature averaging iterations: " << H_smooth_its << "\n";
  std::cout << "Adjacency tree levels "<< adj_levels << "\n";
  std::cout << "Right handed mesh: " << rightHandMesh << "\n";
  std::cout << "is_test: " << is_test << "\n";
  if(is_test)
  {
    std::cout<<"Running comparison test on \n";
    std::cout<<"Baseline model: "<<baselineModel.c_str()<<"\n";
  }
  
  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  std::string comment = "Reading input model " + InputSurface;
  vtkPluginFilterWatcher watchReader(reader,
                                     comment.c_str(),
                                     CLPProcessInformation);
  reader->SetFileName(InputSurface.c_str());
  reader->Update();

  InitParam init = {evolve_its, mesh_smooth_its, H_smooth_its, adj_levels, rightHandMesh };


  if (reader->GetOutput() == NULL)
    {
    std::cerr << "ERROR reading input surface file " << InputSurface.c_str();
    reader->Delete();
    return EXIT_FAILURE;
    }
  vtkSmartPointer<vtkPolyData> polyDataInput  = reader->GetOutput();

  vtkSmartPointer<vtkPolyData> polyDataOutput = vtkSmartPointer<vtkPolyData>::New();
  
  //vtkPolyData * polyDataOutput =
  
  entry_main( polyDataInput, ContourSeedPts, polyDataOutput, init );

  if(is_test != 0)  
  {
//#define SFLS_TEST_RESULT_PASS 2
  reader->SetFileName(baselineModel.c_str());
  reader->Update();
  if (reader->GetOutput() == NULL)
  {
    std::cerr <<"ERROR reading baseline surface file "<< baselineModel.c_str();
    reader->Delete();
    return EXIT_FAILURE;
  }
  vtkSmartPointer<vtkPolyData> polyDataBaseline = reader->GetOutput();
    int iOut = run_test( polyDataOutput, polyDataBaseline);
    return (iOut > 0 ) ? 2 : 1 ;
  //#undef SFLS_TEST_RESULT_PASS
  }
  vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  std::string commentWrite = "Writing output model " + OutputModel;
  vtkPluginFilterWatcher watchWriter(writer,
                                     commentWrite.c_str(),
                                     CLPProcessInformation);
  polyDataOutput->Update( );
  writer->SetIdTypeToInt32();
  writer->SetInput( polyDataOutput );
  writer->SetFileName( OutputModel.c_str() );

  writer->Update( );
  writer->Write();

  // The result is contained in the scalar colormap of the output.

  return EXIT_SUCCESS;
}

