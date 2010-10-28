#ifndef __vtkMRMLPharmacokineticsCurveAnalysisNode_h
#define __vtkMRMLPharmacokineticsCurveAnalysisNode_h

#include "vtkPharmacokineticsWin32Header.h"
#include "vtkMRML.h"
#include "vtkMRMLNode.h"
#include "vtkMRMLStorageNode.h"

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"

#include "vtkMatrix4x4.h"
#include "vtkTransform.h"

class VTK_Pharmacokinetics_EXPORT vtkMRMLPharmacokineticsCurveAnalysisNode : public vtkMRMLNode
{
 public:
  //BTX
  typedef std::map<std::string, double>          ValueMapType;
  typedef std::map<std::string, vtkDoubleArray*> ArrayMapType;
  //ETX

  public:
  static vtkMRMLPharmacokineticsCurveAnalysisNode *New();
  vtkTypeMacro(vtkMRMLPharmacokineticsCurveAnalysisNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName()
    {return "CurveAnalysis";};

  vtkSetObjectMacro( TargetCurve, vtkDoubleArray );
  vtkGetObjectMacro( TargetCurve, vtkDoubleArray );
  vtkSetObjectMacro( FittedCurve, vtkDoubleArray );
  vtkGetObjectMacro( FittedCurve, vtkDoubleArray );

  void SetMethodName(const char* name)
  {
    this->MethodName = name;
  }
  
  const char* GetMethodName()
  {
    return this->MethodName.c_str();
  }


  void SetMethodDescription(const char* desc)
  {
    this->MethodDescription = desc;
  }
  
  const char* GetMethodDescription()
  {
    return this->MethodName.c_str();
  }

  // Input curve data
  void SetInputArray(const char* name, vtkDoubleArray* data);
  void AddInputArrayName(const char* name) { SetInputArray(name, NULL); };
  vtkDoubleArray* GetInputArray(const char* name);
  vtkStringArray* GetInputArrayNameArray();
  void ClearInputArrays();

  // Initial optimization parameters
  void SetInitialParameter(const char* name, double value);
  double GetInitialParameter(const char* name);
  vtkStringArray* GetInitialParameterNameArray();
  void ClearInitialParameters();

  // Constant
  void SetConstant(const char *name, const double value);
  double GetConstant(const char* name);
  vtkStringArray* GetConstantNameArray();
  void ClearConstants();
  
  // Output values
  void SetOutputValue(const char *name, const double value);
  double GetOutputValue(const char* name);
  vtkStringArray* GetOutputValueNameArray();
  void ClearOutputValues();


private:
  vtkMRMLPharmacokineticsCurveAnalysisNode();
  ~vtkMRMLPharmacokineticsCurveAnalysisNode();
  vtkMRMLPharmacokineticsCurveAnalysisNode(const vtkMRMLPharmacokineticsCurveAnalysisNode&);
  void operator=(const vtkMRMLPharmacokineticsCurveAnalysisNode&);
  
  vtkDoubleArray* TargetCurve;
  vtkDoubleArray* FittedCurve;

  ValueMapType    Constants;
  ValueMapType    OutputValues;
  ArrayMapType    InputArrays;
  ValueMapType    InitialParameters;

  //BTX
  //std::string FunctionName;
  std::string MethodName;
  std::string MethodDescription;
  //ETX

};

#endif

