#ifndef __vtkMRMLEMSTargetNode_h
#define __vtkMRMLEMSTargetNode_h

#include "vtkMRML.h"
#include "vtkMRMLNode.h"
#include "vtkEMSegment.h"
#include "vtkMRMLEMSVolumeCollectionNode.h"

class vtkMRMLEMSIntensityNormalizationParametersNode;

class VTK_EMSEGMENT_EXPORT vtkMRMLEMSTargetNode : 
  public vtkMRMLEMSVolumeCollectionNode
{
public:
  static vtkMRMLEMSTargetNode *New();
  vtkTypeMacro(vtkMRMLEMSTargetNode,vtkMRMLEMSVolumeCollectionNode);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "EMSTarget";}

protected:
  vtkMRMLEMSTargetNode() {} ;
  ~vtkMRMLEMSTargetNode() {};
  vtkMRMLEMSTargetNode(const vtkMRMLEMSTargetNode&);
  void operator=(const vtkMRMLEMSTargetNode&);
};

#endif
