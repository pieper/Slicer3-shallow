#include "vtkEMSegmentGUI.h"
#include "vtkEMSegmentLogic.h"
#include "vtkMRMLEMSNode.h"
#include "vtkMRMLScene.h"

#include "vtkSlicerApplication.h"

#include "vtkKWMessageDialog.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWWizardWidget.h"
#include "vtkKWWizardWorkflow.h"
#include "vtkKWLabel.h"

// For PopulateTestingData()
#include "vtkSlicerVolumesGUI.h"
#include "vtkSlicerVolumesLogic.h"
#include "vtkMRMLVolumeNode.h"
#include "vtkDirectory.h"
#include "vtkIntArray.h"

#include "vtkEMSegmentParametersSetStep.h"
#include "vtkEMSegmentAnatomicalStructureStep.h"
#include "vtkEMSegmentSpatialPriorsStep.h"
#include "vtkEMSegmentNodeParametersStep.h"
#include "vtkEMSegmentIntensityDistributionsStep.h"
#include "vtkEMSegmentIntensityImagesStep.h"
#include "vtkEMSegmentRegistrationParametersStep.h"
#include "vtkEMSegmentRunSegmentationStep.h"

#include "vtkEMSegmentInputChannelsStep.h"
#include "vtkEMSegmentPreProcessingStep.h"

#include "vtkKWIcon.h"
#include "vtkKWLabel.h"

#include <vtksys/stl/string>
#include <vtksys/SystemTools.hxx>

vtkCxxSetObjectMacro(vtkEMSegmentGUI,Node,vtkMRMLEMSNode);
vtkCxxSetObjectMacro(vtkEMSegmentGUI,Logic,vtkEMSegmentLogic);
vtkCxxSetObjectMacro(vtkEMSegmentGUI,MRMLManager,vtkEMSegmentMRMLManager);

//----------------------------------------------------------------------------
vtkEMSegmentGUI* vtkEMSegmentGUI::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkEMSegmentGUI");
  if (ret)
    {
    return (vtkEMSegmentGUI*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEMSegmentGUI;
}

//----------------------------------------------------------------------------
vtkEMSegmentGUI::vtkEMSegmentGUI()
{
  this->MRMLManager  = NULL;
  this->Logic        = NULL;
  this->Node         = NULL;
  this->ModuleName   = NULL;

  this->WizardWidget = vtkKWWizardWidget::New();

  this->ParametersSetStep          = NULL;
  this->InputChannelStep           = NULL;
  this->PreProcessingStep        = NULL;

  this->AnatomicalStructureStep    = NULL;
  this->SpatialPriorsStep          = NULL;
  this->IntensityImagesStep        = NULL;
  this->IntensityDistributionsStep = NULL;
  this->NodeParametersStep         = NULL;
  this->RegistrationParametersStep = NULL;
  this->RunSegmentationStep        = NULL;

  //vtkKWIcon* logo = vtkKWIcon::New();
  //logo->SetImage(image_CSAILLogo,
  //                image_CSAILLogo_width, image_CSAILLogo_height,
  //                image_CSAILLogo_pixel_size, image_CSAILLogo_length,
  //                0);
  // this->Logo = logo;
  // logo->Delete();

  this->NACLabel = NULL;
  this->StartSegmentStep= NULL;
  this->SegmentationMode = SegmentationModeAdvanced;
}

//----------------------------------------------------------------------------
vtkEMSegmentGUI::~vtkEMSegmentGUI()
{
  this->RemoveMRMLNodeObservers();
  this->RemoveLogicObservers();

  this->SetMRMLManager(NULL);
  this->SetLogic(NULL);
  this->SetNode(NULL);

  if (this->WizardWidget)
    {
    this->WizardWidget->Delete();
    this->WizardWidget = NULL;
    }

  if (this->ParametersSetStep)
    {
    this->ParametersSetStep->Delete();
    this->ParametersSetStep = NULL;
    }

  if (this->InputChannelStep)
    {
    this->InputChannelStep->Delete();
    this->InputChannelStep = NULL;
    }

  if (this->PreProcessingStep)
    {
    this->PreProcessingStep->Delete();
    this->PreProcessingStep = NULL;
    }

  if (this->AnatomicalStructureStep)
    {
    this->AnatomicalStructureStep->Delete();
    this->AnatomicalStructureStep = NULL;
    }

  if (this->SpatialPriorsStep)
    {
    this->SpatialPriorsStep->Delete();
    this->SpatialPriorsStep = NULL;
    }

  if (this->IntensityImagesStep)
    {
    this->IntensityImagesStep->Delete();
    this->IntensityImagesStep = NULL;
    }
  
  if (this->IntensityDistributionsStep)
    {
    this->IntensityDistributionsStep->Delete();
    this->IntensityDistributionsStep = NULL;
    }

  if (this->NodeParametersStep)
    {
    this->NodeParametersStep->Delete();
    this->NodeParametersStep = NULL;
    }

  if (this->RegistrationParametersStep)
    {
    this->RegistrationParametersStep->Delete();
    this->RegistrationParametersStep = NULL;
    }

  if (this->RunSegmentationStep)
    {
    this->RunSegmentationStep->Delete();
    this->RunSegmentationStep = NULL;
    }

  if (this->NACLabel)
    {
      this->NACLabel->Delete();
      this->NACLabel = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentGUI::SetModuleLogic(vtkSlicerLogic* logic)
{
  this->SetLogic( dynamic_cast<vtkEMSegmentLogic*> (logic) );
  this->GetLogic()->GetMRMLManager()->SetMRMLScene( this->GetMRMLScene() ); 
  this->SetMRMLManager( this->GetLogic()->GetMRMLManager() );
}

//----------------------------------------------------------------------------
void vtkEMSegmentGUI::RemoveMRMLNodeObservers()
{
}

//----------------------------------------------------------------------------
void vtkEMSegmentGUI::RemoveLogicObservers()
{
}

//----------------------------------------------------------------------------
void vtkEMSegmentGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::AddGUIObservers() 
{
  // observe when nodes are added or removed from the scene
  vtkIntArray* events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  if (this->GetMRMLScene() != NULL)
    {
    this->SetAndObserveMRMLSceneEvents(this->GetMRMLScene(), events);
    }
  events->Delete();
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::RemoveGUIObservers()
{
#if 0
  this->ApplyButton->RemoveObservers(vtkKWPushButton::InvokedEvent,  
                                     (vtkCommand *)this->GUICallbackCommand);
#endif
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::ProcessGUIEvents(vtkObject *caller,
                                                      unsigned long event,
                       void *callData) 
{
  this->IntensityDistributionsStep->ProcessManualIntensitySamplingGUIEvents(
    caller, event, callData);
  //this->RunSegmentationStep->ProcessRunRegistrationOutputGUIEvents(caller, event, callData);
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::ProcessLogicEvents (
                      vtkObject *caller, unsigned long event, void *vtkNotUsed(callData) )
{
  if ( !caller || !this->WizardWidget)
    {
    return;
    }

  // process Logic changes
  vtkEMSegmentLogic *callbackLogic = 
    vtkEMSegmentLogic::SafeDownCast(caller);
  
  if ( callbackLogic == this->GetLogic ( ) && 
    event == vtkCommand::ProgressEvent) 
    {
    this->UpdateRegistrationProgress();
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentGUI::UpdateRegistrationProgress()
{
  double progress = this->Logic->GetProgressGlobalFractionCompleted();
  if(progress>=0 && progress <=1)
    {
    this->GetApplicationGUI()->GetMainSlicerWindow()->GetProgressGauge()->
      SetValue(progress*100);
    }
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::UpdateMRML()
{
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::UpdateGUI()
{
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::ProcessMRMLEvents(vtkObject *caller,
                                       unsigned long event,
                                       void *callData) 
{
  // TODO: map the object and event to strings for tcl
  
  //vtksys_stl::cout << "ProcessMRMLEvents()" << vtksys_stl::endl;
  // if parameter node has been changed externally, update GUI widgets
  // with new values 
  vtkMRMLEMSNode* node
    = vtkMRMLEMSNode::SafeDownCast(caller);
  if (node != NULL && this->GetNode() == node) 
    {
    this->UpdateGUI();
    }

  // If there is an EMS MRML node changed event, the wizard should
  // update right away on any step
  if ( vtkMRMLScene::SafeDownCast(caller) == this->MRMLScene 
    && (event == vtkMRMLScene::NodeAddedEvent 
    || event == vtkMRMLScene::NodeRemovedEvent ) )
    {
    vtkMRMLNode *mrmlNode = (vtkMRMLNode*)(callData);
    if (mrmlNode != NULL && mrmlNode->IsA("vtkMRMLEMSNode"))
      {
      // current node removed
      if(mrmlNode == this->GetNode() && event == vtkMRMLScene::NodeRemovedEvent)
        {
        vtkKWMessageDialog::PopupMessage( 
          this->GetApplication(), 
          this->GetApplicationGUI()->GetMainSlicerWindow(),
          "EM Segment", "Current MRML node is removed!",
          vtkKWMessageDialog::WarningIcon);
        }
      this->ParametersSetStep->UpdateLoadedParameterSets();
      this->WizardWidget->GetWizardWorkflow()->
        GetCurrentStep()->ShowUserInterface();
      }
    }

  if (this->RunSegmentationStep)
    {
      this->RunSegmentationStep->ProcessMRMLEvents(caller, event, callData);  
    }
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::BuildGUI() 
{
  vtkSlicerApplication *app = (vtkSlicerApplication *)this->GetApplication();

  this->Logic->RegisterMRMLNodesWithScene();

  this->UIPanel->AddPage("EMSegment", 
                         "EMSegment", NULL);
  vtkKWWidget *module_page = 
    this->UIPanel->GetPageWidget("EMSegment");

  //this->PopulateTestingData();

  // -----------------------------------------------------------------------
  // Help

  // vtkSlicerModuleCollapsibleFrame *help_frame = 
  //   vtkSlicerModuleCollapsibleFrame::New();
  // help_frame->SetParent(module_page);
  // help_frame->Create();
  // help_frame->CollapseFrame();
  // help_frame->SetLabelText("Help");
  // help_frame->Delete();

  // app->Script("pack %s -side top -anchor nw -fill x -padx 2 -pady 2 -in %s", help_frame->GetWidgetName(), module_page->GetWidgetName());

 // configure the parent classes help text widget

//   this->HelpText->SetParent(help_frame->GetFrame());
//   this->HelpText->Create();
//   this->HelpText->SetHorizontalScrollbarVisibility(0);
//   this->HelpText->SetVerticalScrollbarVisibility(1);
//   this->HelpText->GetWidget()->SetText(help);
//   this->HelpText->GetWidget()->SetReliefToFlat();
//   this->HelpText->GetWidget()->SetWrapToWord();
//   this->HelpText->GetWidget()->ReadOnlyOn();
//   this->HelpText->GetWidget()->QuickFormattingOn();
// 
//   app->Script("pack %s -side top -fill x -expand y -anchor w -padx 2 -pady 4",
//               this->HelpText->GetWidgetName());
//  
  // configure the parent classes help text widget
  const char *help = 
    "**EMSegment Module:**  Segment a set of set of images (target images) using the tree-based EM segmentation algorithm\n\nUse the pull down menu to select from a collection of tasks or create a new one.\n\nUse the 'Back' and 'Next' to navigate through the stages of filling in the algorithm parameters.\n\nWhen all parameters are specified, use the 'segmentation' button. \n\nFor latest updates, new tasks, and detail help please visit <a>http://www.slicer.org/slicerWiki/index.php/Modules:EMSegmenter-3.6</a> \n\n **The work was reported in**: \nK.M. Pohl et. A hierarchical algorithm for MR brain image parcellation. IEEE Transactions on Medical Imaging, 26(9),pp 1201-1212, 2007.";
    const char *about = "This module is currently maintained by Daniel Haehn, Dominique Belhachemi, and Kilian Pohl (SBIA,UPenn). The work is currently supported by an ARRA supplement to NAC and the Slicer Community (see also <a>http://www.slicer.org</a>). \n\nThe work was reported in  \nK.M. Pohl et. A hierarchical algorithm for MR brain image parcellation. IEEE Transactions on Medical Imaging, 26(9),pp 1201-1212, 2007.";
 this->BuildHelpAndAboutFrame (module_page, help, about );

  this->NACLabel = vtkKWLabel::New();
  this->NACLabel->SetParent ( this->GetLogoFrame() );
  this->NACLabel->Create();
  this->NACLabel->SetImageToIcon ( this->GetAcknowledgementIcons()->GetNACLogo() );
  app->Script ( "grid %s -row 0 -column 0 -padx 2 -pady 2 -sticky w", this->NACLabel->GetWidgetName());


  // -----------------------------------------------------------------------
  // Wizard

  vtkSlicerModuleCollapsibleFrame *wizard_frame = 
    vtkSlicerModuleCollapsibleFrame::New();
  wizard_frame->SetParent(module_page);
  wizard_frame->Create();
  wizard_frame->SetLabelText("Wizard");
  wizard_frame->ExpandFrame();

  app->Script("pack %s -side top -anchor nw -fill x -padx 2 -pady 2 -in %s",
              wizard_frame->GetWidgetName(), 
              module_page->GetWidgetName());
   
  this->WizardWidget->SetParent(wizard_frame->GetFrame());
  this->WizardWidget->Create();
  this->WizardWidget->GetSubTitleLabel()->SetHeight(1);
  this->WizardWidget->SetClientAreaMinimumHeight(320);
  //this->WizardWidget->SetButtonsPositionToTop();
  this->WizardWidget->HelpButtonVisibilityOn();
  app->Script("pack %s -side top -anchor nw -fill both -expand y",
              this->WizardWidget->GetWidgetName());
  wizard_frame->Delete();


  this->WizardWidget->GetFinishButton()->SetCommand(this, "StartSegmentation");
  this->WizardWidget->GetFinishButton()->SetText("Segment");
  this->WizardWidget->GetFinishButton()->SetBalloonHelpString("Start Segmentation");

  vtkKWWizardWorkflow *wizard_workflow =  this->WizardWidget->GetWizardWorkflow();

  // -----------------------------------------------------------------
  // Parameter Set step

  if (!this->ParametersSetStep)
    {
    this->ParametersSetStep = vtkEMSegmentParametersSetStep::New();
    this->ParametersSetStep->SetGUI(this);
    }
  wizard_workflow->AddStep(this->ParametersSetStep);

  // -----------------------------------------------------------------
  // Anatomical Structure step

    if (!this->InputChannelStep)
    {
    this->InputChannelStep = vtkEMSegmentInputChannelsStep::New();
    this->InputChannelStep->SetGUI(this);
    }
    wizard_workflow->AddNextStep(this->InputChannelStep);
  this->ParametersSetStep->SetNextStep(this->InputChannelStep);

  if (!this->AnatomicalStructureStep)
    {
    this->AnatomicalStructureStep = vtkEMSegmentAnatomicalStructureStep::New();
    this->AnatomicalStructureStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->AnatomicalStructureStep);


  // -----------------------------------------------------------------
  // Spatial Priors step

  if (!this->SpatialPriorsStep)
    {
    this->SpatialPriorsStep = vtkEMSegmentSpatialPriorsStep::New();
    this->SpatialPriorsStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->SpatialPriorsStep);

  // -----------------------------------------------------------------
  // Registration Parameters step
  if (!this->RegistrationParametersStep)
    {
    this->RegistrationParametersStep = 
      vtkEMSegmentRegistrationParametersStep::New();
    this->RegistrationParametersStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->RegistrationParametersStep);

  // -----------------------------------------------------------------
  // Prior Processing Step
  if (!this->PreProcessingStep)
    {
    this->PreProcessingStep = vtkEMSegmentPreProcessingStep::New();
    this->PreProcessingStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->PreProcessingStep);

  if (!this->IntensityDistributionsStep)
    {
    this->IntensityDistributionsStep = 
      vtkEMSegmentIntensityDistributionsStep::New();
    this->IntensityDistributionsStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->IntensityDistributionsStep);
  this->PreProcessingStep->SetNextStep(this->IntensityDistributionsStep);

  // -----------------------------------------------------------------
  // Node Parameters step

  if (!this->NodeParametersStep)
    {
    this->NodeParametersStep = vtkEMSegmentNodeParametersStep::New();
    this->NodeParametersStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->NodeParametersStep);

  // -----------------------------------------------------------------
  // Run Segmentation step

  if (!this->RunSegmentationStep)
    {
    this->RunSegmentationStep = vtkEMSegmentRunSegmentationStep::New();
    this->RunSegmentationStep->SetGUI(this);
    }
  wizard_workflow->AddNextStep(this->RunSegmentationStep);

  // -----------------------------------------------------------------
  // Initial and finish step

  wizard_workflow->SetFinishStep(this->RunSegmentationStep);

  // wizard_workflow->CreateGoToTransitionsToFinishStep();

  wizard_workflow->SetInitialStep(this->ParametersSetStep);
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::TearDownGUI() 
{
  if (this->ParametersSetStep)
    {
    this->ParametersSetStep->SetGUI(NULL);
    }


  if (this->InputChannelStep)
    {
    this->InputChannelStep->SetGUI(NULL);
    }
  if (this->PreProcessingStep)
    {
      this->PreProcessingStep->SetGUI(NULL);
    }

  if (this->AnatomicalStructureStep)
    {
    this->AnatomicalStructureStep->SetGUI(NULL);
    }

  if (this->SpatialPriorsStep)
    {
    this->SpatialPriorsStep->SetGUI(NULL);
    }

  if (this->IntensityImagesStep)
    {
    this->IntensityImagesStep->SetGUI(NULL);
    }

  if (this->IntensityDistributionsStep)
    {
    this->IntensityDistributionsStep->SetGUI(NULL);
    }

  if (this->NodeParametersStep)
    {
    this->NodeParametersStep->SetGUI(NULL);
    }

  if (this->RegistrationParametersStep)
    {
    this->RegistrationParametersStep->SetGUI(NULL);
    }

  if (this->RunSegmentationStep)
    {
    this->RunSegmentationStep->SetGUI(NULL);
    }
}

//---------------------------------------------------------------------------
unsigned long vtkEMSegmentGUI::
AddObserverByNumber(vtkObject *observee, unsigned long event) 
{
  return (observee->AddObserver(event, 
                                (vtkCommand *)this->GUICallbackCommand));
} 


//---------------------------------------------------------------------------
void vtkEMSegmentGUI::PopulateTestingData() 
{
  this->Logic->PopulateTestingData();

  vtkSlicerModuleGUI *m = vtkSlicerApplication::SafeDownCast(
    this->GetApplication())->GetModuleGUIByName("Volumes"); 

  if ( m != NULL ) 
    {
    vtkSlicerVolumesLogic* volume_logic = 
      vtkSlicerVolumesGUI::SafeDownCast(m)->GetLogic();
    vtksys_stl::string file_path = vtksys::SystemTools::GetEnv("HOME");
#ifdef _WIN32
    file_path.append("\\tmp\\EMSegmentTestImages");
    if (!vtksys::SystemTools::FileIsDirectory(file_path.c_str()))
      {
      file_path = vtksys::SystemTools::GetEnv("HOME");
      file_path.append("\\temp\\EMSegmentTestImages");
      }
    file_path.append("\\");
#else
    file_path.append("/tmp/EMSegmentTestImages/");
#endif
    
    vtkDirectory *dir = vtkDirectory::New();
    if (!dir->Open(file_path.c_str()))
      {
      dir->Delete();
      return;
      }
    
    for (int i = 0; i < dir->GetNumberOfFiles(); i++)
      {
      vtksys_stl::string filename = dir->GetFile(i);
      //skip . and ..
      if (strcmp(filename.c_str(), ".") == 0)
        {
        continue;
        }
      else if (strcmp(filename.c_str(), "..") == 0)
        {
        continue;
        }

      vtksys_stl::string fullName = file_path;
      fullName.append(filename.c_str());
      if (strcmp(vtksys::SystemTools::
                 GetFilenameExtension(fullName.c_str()).c_str(), ".mhd") != 0)
        {
        continue;
        }

      if (vtksys::SystemTools::FileExists(fullName.c_str()) &&
          !vtksys::SystemTools::FileIsDirectory(fullName.c_str()))
        {
        //volume_logic->AddArchetypeVolume((char*)(fullName.c_str()), 1, 0, 
        //                                 filename.c_str()); 
        int loadingOption = 2;
        volume_logic->AddArchetypeVolume(fullName.c_str(), filename.c_str(), loadingOption); 

        }
      }
    dir->Delete();
       
    this->MRMLManager->SetTreeNodeSpatialPriorVolumeID(
      this->MRMLManager->GetTreeRootNodeID(), 
      this->MRMLManager->GetVolumeNthID(0));

    this->MRMLManager->SetRegistrationAtlasVolumeID(
      this->MRMLManager->GetVolumeNthID(0));
    this->MRMLManager->AddTargetSelectedVolume(
      this->MRMLManager->GetVolumeNthID(1));

    this->MRMLManager->SetSaveWorkingDirectory(file_path.c_str());
    this->MRMLManager->
      SetSaveTemplateFilename(file_path.append("EMSTemplate.mrml").c_str());
    }
} 

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::Init()
{
  vtkMRMLScene *scene = this->Logic->GetMRMLScene();

  vtkIntArray *emsEvents = vtkIntArray::New();
  emsEvents->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  emsEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  this->Logic->SetAndObserveMRMLSceneEvents(scene, emsEvents);
  emsEvents->Delete();
}

//---------------------------------------------------------------------------
void vtkEMSegmentGUI::StartSegmentation() 
  {
    cout << "vtkEMSegmentGUI::StartSegmentation()  start"  << endl;
    vtkKWWizardWorkflow *wizard_workflow =  this->WizardWidget->GetWizardWorkflow();
    vtkKWWizardStep *currentStep =  wizard_workflow->GetCurrentStep();

    // It is called for the first time 
    if (!this->StartSegmentStep) 
      {
        this->StartSegmentStep = currentStep;    
      }

    if (currentStep ==  wizard_workflow->GetFinishStep())
      {
        // At the final step execute the OK button
        this->WizardWidget->GetOKButton()->CommandCallback();       
        // Now go back to the location where the button was pressed 
        if (this->StartSegmentStep) 
        {
          while (this->StartSegmentStep != currentStep)
          {
            wizard_workflow->AttemptToGoToPreviousStep();
            if ((currentStep == wizard_workflow->GetCurrentStep()) ||  wizard_workflow->GetCurrentStep() == wizard_workflow->GetInitialStep())
            {
                break;
            }
            currentStep = wizard_workflow->GetCurrentStep();
          }
          currentStep->ShowUserInterface();
          this->StartSegmentStep = NULL;
        }
    std::string msg = this->Logic->GetErrorMessage(); 
    if (msg.size()) 
      {
        vtkKWMessageDialog::PopupMessage(this->GetApplication(),NULL,"Error In Segmentation", msg.c_str() , vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::InvokeAtPointer);
      }

        return;
      }

    wizard_workflow->AttemptToGoToNextStep();

    if (currentStep != wizard_workflow->GetCurrentStep()) {
      this->StartSegmentation();
    } else {
      // Error occurred
      this->StartSegmentStep = NULL;
    }
  }

