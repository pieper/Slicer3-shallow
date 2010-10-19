/*=auto=======================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights
  Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkEMSegmentParametersSetStep.cxx,v$
  Date:      $Date: 2006/01/06 17:56:51 $
  Version:   $Revision: 1.6 $

=======================================================================auto=*/

#include "vtkEMSegmentParametersSetStep.h"

#include "vtkEMSegmentGUI.h"
#include "vtkEMSegmentMRMLManager.h"

#include "vtkKWWizardWidget.h"
#include "vtkKWWizardWorkflow.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWTreeWithScrollbars.h"
#include "vtkKWTree.h"

#include "vtkKWTopLevel.h"
#include "vtkKWPushButton.h"
#include "vtkKWEntryWithLabel.h"

#include "vtkEMSegmentAnatomicalStructureStep.h"
#include "vtkSlicerApplication.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWMessageDialog.h"
#include "vtkEMSegmentLogic.h"

#include "vtkDirectory.h"
#include "vtkMRMLEMSNode.h"

#include "vtkHTTPHandler.h"

// Need to include this bc otherwise cannot find std functions  for some g ++ compilers 
#include <algorithm>

// need the ITK systemtools
// #include <vtksys/SystemTools.hxx>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEMSegmentParametersSetStep);
vtkCxxRevisionMacro(vtkEMSegmentParametersSetStep, "$Revision: 1.2 $");

//----------------------------------------------------------------------------
vtkEMSegmentParametersSetStep::vtkEMSegmentParametersSetStep()
{
  this->SetName("1/9. Define Task");
  this->SetDescription("Select a (new) task.");

  this->ParameterSetFrame       = NULL;
  this->ParameterSetMenuButton  = NULL;
  this->UpdateTasksButton       = NULL;
  this->PreprocessingMenuButton = NULL;


  this->RenameIndex = -1;
  this->RenameEntry = NULL;
  this->RenameTopLevel = NULL;
  this->RenameApply = NULL;
  this->RenameCancel = NULL;
}

//----------------------------------------------------------------------------
vtkEMSegmentParametersSetStep::~vtkEMSegmentParametersSetStep()
{
  if (this->ParameterSetMenuButton)
    {
    this->ParameterSetMenuButton->Delete();
    this->ParameterSetMenuButton = NULL;
    }

  if (this->PreprocessingMenuButton)
    {
    this->PreprocessingMenuButton->Delete();
    this->PreprocessingMenuButton = NULL;
    }

  if (this->UpdateTasksButton)
    {
    this->UpdateTasksButton->Delete();
    this->UpdateTasksButton = NULL;
    }

  if (this->ParameterSetFrame)
    {
    this->ParameterSetFrame->Delete();
    this->ParameterSetFrame = NULL;
    }

 if (this->RenameEntry)
    {
    this->RenameEntry->SetParent(NULL);
    this->RenameEntry->Delete();
    this->RenameEntry = NULL;
    }
  if (this->RenameTopLevel)
    {
    this->RenameTopLevel->SetParent(NULL);
    this->RenameTopLevel->Delete();
    this->RenameTopLevel = NULL;
    }

  if (this->RenameApply)
    {
    this->RenameApply->SetParent(NULL);
    this->RenameApply->Delete();
    this->RenameApply = NULL;
    }
  if (this->RenameCancel)
    {
    this->RenameCancel->SetParent(NULL);
    this->RenameCancel->Delete();
    this->RenameCancel = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::ShowUserInterface()
{
  this->Superclass::ShowUserInterface();


  vtkKWWizardWidget *wizardWidget = this->GetGUI()->GetWizardWidget();

  wizardWidget->GetCancelButton()->SetEnabled(0);
  wizardWidget->SetNextButtonVisibility(0);
  wizardWidget->SetBackButtonVisibility(0);
  wizardWidget->SetFinishButtonVisibility(0);

  // Create the Parameters set frame

  if (!this->ParameterSetFrame)
    {
    this->ParameterSetFrame = vtkKWFrameWithLabel::New();
    }

  if (!this->ParameterSetFrame->IsCreated())
    {
    this->ParameterSetFrame->SetParent(wizardWidget->GetClientArea());
    this->ParameterSetFrame->Create();
    this->ParameterSetFrame->SetLabelText("Select Task");
    }

  this->Script("pack %s -side top -expand n -fill both -padx 0 -pady 2",
      this->ParameterSetFrame->GetWidgetName());

  // Create the Parameters Set Menu button

  if (!this->ParameterSetMenuButton)
    {
    this->ParameterSetMenuButton = vtkKWMenuButtonWithLabel::New();
    }

  if (!this->ParameterSetMenuButton->IsCreated())
    {
    this->ParameterSetMenuButton->SetParent(
      this->ParameterSetFrame->GetFrame());
    this->ParameterSetMenuButton->Create();
    this->ParameterSetMenuButton->GetLabel()->SetWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 10);
    this->ParameterSetMenuButton->SetLabelText("Task:");
    this->ParameterSetMenuButton->GetWidget()->SetWidth(
      EMSEG_MENU_BUTTON_WIDTH + 10);
    this->ParameterSetMenuButton->SetBalloonHelpString(
      "Select Task.");
    }
  this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
               this->ParameterSetMenuButton->GetWidgetName());



  // Create the update tasks button

  if (!this->UpdateTasksButton)
    {
    this->UpdateTasksButton = vtkKWPushButton::New();
    }
  if (!this->UpdateTasksButton->IsCreated())
    {
    this->UpdateTasksButton->SetParent(this->ParameterSetFrame->GetFrame());
    this->UpdateTasksButton->Create();
    this->UpdateTasksButton->SetText("Update task list");
    this->UpdateTasksButton->SetCommand(this, "UpdateTasksCallback");
    }
  this->Script("pack %s -padx 2 -pady 2",
               this->UpdateTasksButton->GetWidgetName());


  this->UpdateLoadedParameterSets();
}

//----------------------------------------------------------------------------
// Updates the .tcl Tasks from an external website and replaces the content
// in $tmpDir/EMSegmentTask (e.g. /home/Slicer3USER/EMSegmentTask)
//----------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::UpdateTasksCallback()
{

  { // we want our own scope in this one :D
  
  //
  // ** THE URL **
  //
  // the url to the EMSegment task repository
  //std::string taskRepository = "http://people.csail.mit.edu/pohl/EMSegmentUpdates/";
  std::string taskRepository = "http://slicer.org/EMSegmentUpdates/";

  //
  // ** PATH MANAGEMENT **
  //
  // we need the slicer application to query the $tmp_dir path
  const char* tmpDir = this->GetSlicerApplication()->GetTemporaryDirectory();
  if (!tmpDir)
    {
      vtkErrorMacro("UpdateTasksCallback: Termporary directory is not defined!");
      return;
    }
  // also add the manifest filename
  std::string tmpManifestFilename(std::string(tmpDir) + std::string("/EMSegmentTasksManifest.html"));
  std::string manifestFilename = vtksys::SystemTools::ConvertToOutputPath(tmpManifestFilename.c_str());

  // and add the EmSegmentTask directory
  std::string taskDir = this->GetGUI()->GetLogic()->GetTemporaryTaskDirectory(this->GetSlicerApplication());
  //
  // ** HTTP ACCESS **
  //
  // our HTTP handler
  vtkHTTPHandler* httpHandler = vtkHTTPHandler::New();

  // prevent funny behavior on windows with the side-effect of more network resources are used
  // (o_o) who cares about traffic or the tcp/ip ports? *g
  httpHandler->SetForbidReuse(1);

  // safe-check if the handler can really handle the hardcoded uri protocal
  if (!httpHandler->CanHandleURI(taskRepository.c_str()))
    {
    vtkErrorMacro("UpdateTasksCallback: Invalid URI specified and you can't do anything about it bcuz it is *hardcoded*!")
    return;
    }

  //
  // ** THE ACTION STARTS **
  //
  // make sure we can access the task repository
  // TODO: this function will be provided by Wendy sooner or later :D
  //       for now, we just assume that we are on-line!
  
  // get the directory listing of the EMSegment task repository and save it as $tmpDir/EMSegmentTasksManifest.html.
  // the manifest always gets overwritten, but nobody should care
  httpHandler->StageFileRead(taskRepository.c_str(),manifestFilename.c_str());
  
  // sanity checks: if manifestFilename does not exist or size<1, exit here before it is too late!
  if (!vtksys::SystemTools::FileExists(manifestFilename.c_str()) || vtksys::SystemTools::FileLength(manifestFilename.c_str())<1)
    {
    vtkErrorMacro("UpdateTasksCallback: Could not get the manifest! Try again later..")
    return;
    }
  
  
  // what happens now? answer: a three-step-processing chain!!!
  // (1) now we got the manifest and we can parse it for filenames of EMSegment tasks.
  // (2) then, download these files and copy them to our $tmpDir.
  //     after we are sure we got the files (we can not be really sure but we can check if some files where downloaded), 
  // (3) we delete all old files in $taskDir and then copy our newly downloaded EMSegment tasks from $tmpDir to $taskDir.
  // sounds good!
  
  // 1) open the manifest and get the filenames
  char* htmlManifest = 0;
  std::ifstream fileStream(manifestFilename.c_str());
  if (!fileStream.fail())
    {
    fileStream.seekg(0,std::ios::end);
    size_t length = fileStream.tellg();
    fileStream.seekg(0,std::ios::beg);
    htmlManifest = new char[length+1];
    fileStream.read(htmlManifest, length);
    htmlManifest[length] = '\n';
    }
    
  fileStream.close();
    
  // when C++0x is released, we could easily do something like this to filter out the .tcl and .mrml filenames:
  //  cmatch regexResult;
  //  regex tclExpression("(\w*-*)+.tcl(?!\")");  
  //  regex_search(htmlManifest, regexResult, tclExpression); 
  //  regex mrmlExpression("(\w*-*)+.mrml(?!\")");  
  //  regex_search(htmlManifest, regexResult, mrmlExpression);   
  // but right now, we have to manually parse the string.
  // at least we can use std::string methods :D
  std::string beginTaskFilenameTag(".tcl\"> ");
  std::string endTaskFilenameTag(".tcl</a>");
  std::string beginMrmlFilenameTag(".mrml\"> ");
  std::string endMrmlFilenameTag(".mrml</a>");  
  
  bool tclFilesExist = false;
  bool mrmlFilesExist = false;
  
  std::vector<std::string> taskFilenames;
  std::vector<std::string> mrmlFilenames;

  std::string htmlManifestAsString(htmlManifest);
  
  std::string::size_type beginTaskFilenameIndex = htmlManifestAsString.find(beginTaskFilenameTag,0);
  
  // the loop for .tcl files
  while(beginTaskFilenameIndex!=std::string::npos)
    {
    // as long as we find the beginning of a filename, do the following..
    
    // find the corresponding end
    std::string::size_type endTaskFilenameIndex = htmlManifestAsString.find(endTaskFilenameTag,beginTaskFilenameIndex);
  
    if (endTaskFilenameIndex==std::string::npos)
      {
      vtkErrorMacro("UpdateTasksCallback: Error during parsing! There was no end *AAAAAAAAAAAAAAAAAAAAHHHH*")
      return;
      }
    
    // now get the string between begin and end, then add it to the vector
    taskFilenames.push_back(htmlManifestAsString.substr(beginTaskFilenameIndex+beginTaskFilenameTag.size(),endTaskFilenameIndex-(beginTaskFilenameIndex+beginTaskFilenameTag.size())));
    
    // and try to find the next beginTag
    beginTaskFilenameIndex = htmlManifestAsString.find(beginTaskFilenameTag,endTaskFilenameIndex);
    }
    
  // enable copying of .tcl files if they exist
  if (taskFilenames.size()!=0)
    {
    tclFilesExist = true;
    }
    
  std::string::size_type beginMrmlFilenameIndex = htmlManifestAsString.find(beginMrmlFilenameTag,0);
  
  // the loop for .mrml files
  while(beginMrmlFilenameIndex!=std::string::npos)
    {
    // as long as we find the beginning of a filename, do the following..
    
    // find the corresponding end
    std::string::size_type endMrmlFilenameIndex = htmlManifestAsString.find(endMrmlFilenameTag,beginMrmlFilenameIndex);
  
    if (endMrmlFilenameIndex==std::string::npos)
      {
      vtkErrorMacro("UpdateTasksCallback: Error during parsing! There was no end *AAAAAAAAAAAAAAAAAAAAHHHH*")
      return;
      }
    
    // now get the string between begin and end, then add it to the vector
    mrmlFilenames.push_back(htmlManifestAsString.substr(beginMrmlFilenameIndex+beginMrmlFilenameTag.size(),endMrmlFilenameIndex-(beginMrmlFilenameIndex+beginMrmlFilenameTag.size())));
    
    // and try to find the next beginTag
    beginMrmlFilenameIndex = htmlManifestAsString.find(beginMrmlFilenameTag,endMrmlFilenameIndex);
    }
    
  // enable copying of .mrml files if they exist
  if (mrmlFilenames.size()!=0)
    {
    mrmlFilesExist = true;
    }    
    
  // 2) loop through the vector and download the task files and the mrml files to the $tmpDir
  std::string currentTaskUrl;
  std::string currentTaskName;
  std::string currentTaskFilepath;

  std::string currentMrmlUrl;
  std::string currentMrmlName;
  std::string currentMrmlFilepath;
  
  if (tclFilesExist)
    {
    // loop for .tcl
    for (std::vector<std::string>::iterator i = taskFilenames.begin(); i != taskFilenames.end(); ++i)
      {
      
      currentTaskName = *i;
      
      // sanity checks: if the filename is "", exit here before it is too late!
      if (!strcmp(currentTaskName.c_str(),""))
        {
        vtkErrorMacro("UpdateTasksCallback: At least one filename was empty, get outta here NOW! *AAAAAAAAAAAAAAAAAHHH*")
        return;
        }
      
      // generate the url of this task
      currentTaskUrl = std::string(taskRepository + currentTaskName + std::string(".tcl"));
      
      // generate the destination filename of this task in $tmpDir
      currentTaskFilepath = std::string(tmpDir + std::string("/") + currentTaskName + std::string(".tcl"));
      
      // and get the content and save it to $tmpDir
      httpHandler->StageFileRead(currentTaskUrl.c_str(),currentTaskFilepath.c_str());
      
      // sanity checks: if the downloaded file does not exist or size<1, exit here before it is too late!
      if (!vtksys::SystemTools::FileExists(currentTaskFilepath.c_str()) || vtksys::SystemTools::FileLength(currentTaskFilepath.c_str())<1)
        {
        vtkErrorMacro("UpdateTasksCallback: At least one file was not downloaded correctly! Aborting.. *beepbeepbeep*")
        return;
        }
     
      }
    }
  
  if (mrmlFilesExist)
    {  
    // loop for .mrml
    for (std::vector<std::string>::iterator i = mrmlFilenames.begin(); i != mrmlFilenames.end(); ++i)
      {
      
      currentMrmlName = *i;
      
      // sanity checks: if the filename is "", exit here before it is too late!
      if (!strcmp(currentMrmlName.c_str(),""))
        {
        vtkErrorMacro("UpdateTasksCallback: At least one filename was empty, get outta here NOW! *AAAAAAAAAAAAAAAAAHHH*")
        return;
        }
      
      // generate the url of this mrml file
      currentMrmlUrl = std::string(taskRepository + currentMrmlName + std::string(".mrml"));
      
      // generate the destination filename of this task in $tmpDir
      currentMrmlFilepath = std::string(tmpDir + std::string("/") + currentMrmlName + std::string(".mrml"));
      
      // and get the content and save it to $tmpDir
      httpHandler->StageFileRead(currentMrmlUrl.c_str(),currentMrmlFilepath.c_str());
      
      // sanity checks: if the downloaded file does not exist or size<1, exit here before it is too late!
      if (!vtksys::SystemTools::FileExists(currentMrmlFilepath.c_str()) || vtksys::SystemTools::FileLength(currentMrmlFilepath.c_str())<1)
        {
        vtkErrorMacro("UpdateTasksCallback: At least one file was not downloaded correctly! Aborting.. *beepbeepbeep*")
        return;
        }
     
      }    
    }

  // we got the .tcl files and the .mrml files now at a safe location and they have at least some content :P
  // this makes it safe to delete all old EMSegment tasks and activate the new one :D
  
  // OMG did you realize that this is a kind of backdoor to take over your home directory?? the
  // downloaded .tcl files get sourced later and can do whatever they want to do!! but pssst let's keep it a secret
  // option for a Slicer backdoor :) on the other hand, the EMSegment tasks repository will be monitored closely and is not
  // public, but what happens if someone changes the URL to the repository *evilgrin*
  
  // 3) copy the $taskDir to a backup folder, delete the $taskDir. and create it again. then, move our downloaded files to it
  
  // purge, NOW!! but only if the $taskDir exists..
  if (vtksys::SystemTools::FileExists(taskDir.c_str()))
  {
    // create a backup of the old taskDir
    std::string backupTaskDir(taskDir + std::string("_old"));
    if (!vtksys::SystemTools::CopyADirectory(taskDir.c_str(),backupTaskDir.c_str()))
      {
      vtkErrorMacro("UpdateTasksCallback: Could not create backup " << backupTaskDir.c_str() << "! This is very bad, we abort the update..")
      return;
      }
    
    if (!vtksys::SystemTools::RemoveADirectory(taskDir.c_str()))
      {
      vtkErrorMacro("UpdateTasksCallback: Could not delete " << taskDir.c_str() << "! This is very bad, we abort the update..")
      return;
      }
  }
  
  // check if the taskDir is gone now!
  if (!vtksys::SystemTools::FileExists(taskDir.c_str()))
    {
    // the $taskDir does not exist, so create it
    bool couldCreateTaskDir = vtksys::SystemTools::MakeDirectory(taskDir.c_str());

    // sanity checks: if the directory could not be created, something is wrong!
    if (!couldCreateTaskDir)
      {
      vtkErrorMacro("UpdateTasksCallback: Could not (re-)create the EMSegmentTask directory: " << taskDir.c_str())
      return;
      }
    }
    
  std::string currentTaskDestinationFilepath;
  std::string currentMrmlDestinationFilepath;
  
  if (tclFilesExist)
    {
    // now move the downloaded .tcl files to the $taskDir
    for (std::vector<std::string>::iterator i = taskFilenames.begin(); i != taskFilenames.end(); ++i)
      {
      
      currentTaskName = *i;    
      
      // generate the destination filename of this task in $tmpDir
      currentTaskFilepath = std::string(tmpDir + std::string("/") + currentTaskName + std::string(".tcl"));
      
      // generate the destination filename of this task in $taskDir
      currentTaskDestinationFilepath = std::string(taskDir + std::string("/") + currentTaskName + std::string(".tcl"));    
      
      if (!vtksys::SystemTools::CopyFileAlways(currentTaskFilepath.c_str(),currentTaskDestinationFilepath.c_str()))
        {
        vtkErrorMacro("UpdateTasksCallback: Could not copy at least one downloaded task file. Everything is lost now! Sorry :( Just kidding: there was a backup in " << taskDir << "!")
        return;
        }
      }
    }
    
  if (mrmlFilesExist)
    {
    // now move the downloaded .mrml files to the $taskDir
    for (std::vector<std::string>::iterator i = mrmlFilenames.begin(); i != mrmlFilenames.end(); ++i)
      {
      
      currentMrmlName = *i;    
      
      // generate the destination filename of this task in $tmpDir
      currentMrmlFilepath = std::string(tmpDir + std::string("/") + currentMrmlName + std::string(".mrml"));
      
      // generate the destination filename of this task in $taskDir
      currentMrmlDestinationFilepath = std::string(taskDir + std::string("/") + currentMrmlName + std::string(".mrml"));    
      
      if (!vtksys::SystemTools::CopyFileAlways(currentMrmlFilepath.c_str(),currentMrmlDestinationFilepath.c_str()))
        {
        vtkErrorMacro("UpdateTasksCallback: Could not copy at least one downloaded mrml file. Everything is lost now! Sorry :( Just kidding: there was a backup in " << taskDir << "!")
        return;
        }
      }
    }
    
  // if we get here, we are DONE!
  this->UpdateTasksButton->SetText("Update completed!");
  this->UpdateTasksButton->SetEnabled(0);
  // Trigger the tasklist reload!!
  this->UpdateLoadedParameterSets();  
  //
  // ** ALL DONE, NOW CLEANUP **
  //
  
  // delete the HTTP handler
  httpHandler->Delete();
  
  // delete the htmlManifest char buffer
  delete[] htmlManifest;
  
  } // now go for destruction, donkey!!
  
}

//----------------------------------------------------------------------------
// defines the menu task list 
void vtkEMSegmentParametersSetStep::PopulateLoadedParameterSets()
{
  if (!this->ParameterSetMenuButton ||
     !this->ParameterSetMenuButton->IsCreated())
    {
    return;
    }

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
      vtkWarningMacro("PopulateLoadedParameterSets: returning, no mrml manager");
      return;
    }

  // Check on Parameter Files 
  this->DefineDefaultTasksList();

  // Define Menu
  vtkKWMenu *menu = this->ParameterSetMenuButton->GetWidget()->GetMenu();
  menu->DeleteAllItems();

  char buffer[256];
  int numSets = mrmlManager->GetNumberOfParameterSets();

  for(int index = 0; index < numSets; index++)
    {
    const char *name = mrmlManager->GetNthParameterSetName(index);

    if (name)
      {
      sprintf(buffer, "%s %d", "SelectedParameterSetChangedCallback", index);
      menu->AddRadioButton(name, this, buffer);
      }
    }
 
  for (int i = 0 ; i < (int)this->pssDefaultTasksName.size(); i++)
    {
    int index = 0;
    // Check if the mrml file associated with the default parameter set is already loaded in the scene
    while ((index < numSets) && strcmp(mrmlManager->GetNthParameterSetName(index),pssDefaultTasksName[i].c_str() ))
      {
      index++;
      }

    // If it is then do not add the item to the menu list bc it was already added in the previous AddRadioButton
    // and jump over this step
    if (index == numSets)
      {
      sprintf(buffer, "SelectedDefaultTaskChangedCallback %d 1", i);
      menu->AddRadioButton(pssDefaultTasksName[i].c_str(), this, buffer);
      }
    }
}

//----------------------------------------------------------------------------
// same as this->PopulateLoadedParameterSets() however the selection is stored 

void vtkEMSegmentParametersSetStep::UpdateLoadedParameterSets()
{
  if(!this->ParameterSetMenuButton ||
     !this->ParameterSetMenuButton->IsCreated())
    {
    return;
    }

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }

  vtkKWMenuButton *menuButton = this->ParameterSetMenuButton->GetWidget();
  vtksys_stl::string sel_value = "";

  // Store current selection
  if (menuButton->GetValue())
    {
    sel_value = menuButton->GetValue();
    }

  // Update Menu Task List
  this->PopulateLoadedParameterSets();

  // Reset selection to stored value 
  if (strcmp(sel_value.c_str(), "") != 0)
    {
    // Select the original
    int numSets = menuButton->GetMenu()->GetNumberOfItems();

    for (int index = 0; index < numSets; index++)
      {
      const char *name = menuButton->GetMenu()->GetItemLabel(index);

      if (name && strcmp(sel_value.c_str(), name) == 0)
        {
        menuButton->GetMenu()->SelectItem(index);
        return;
        }
      }
    }

  // if there is no previous selection, select the first loaded set,
  // or if there is no loaded set, leave it blank
  //int numSets = mrmlManager->GetNumberOfParameterSets();

  //if (numSets > 0 &&
  //   menuButton->GetMenu()->GetNumberOfItems() > 1)
  //  {
  //  this->ParameterSetMenuButton->GetWidget()->GetMenu()->SelectItem(1);
  //  }
}


//----------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::
SelectedDefaultTaskChangedCallback(int index, bool warningFlag)
{
  // cout << "SelectedDefaultTaskChangedCallback " << index << " " << warningFlag << endl;

  if (index < 0 || index >  int(this->pssDefaultTasksName.size() -1) )
    {
      vtkErrorMacro("Index is not defined");
      return;
    }

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();

  // Create New task 
  if (index ==  int(this->pssDefaultTasksName.size() -1))
    {   
      mrmlManager->RemoveAllEMSNodes(); 
      mrmlManager->CreateAndObserveNewParameterSet();
      this->PopUpRenameEntry(mrmlManager->GetNumberOfParameterSets() - 1);
      return;
    }

  this->LoadDefaultTask(index, warningFlag);
}

//----------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::
SelectedPreprocessingChangedCallback(int index, bool warningFlag)
{
  if (index < -1 || index >  int(this->DefinePreprocessingTasksName.size() -1) )
    {
      vtkErrorMacro("Index is not defined");
      return;
    }


  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();

  if (index > -1) 
    { 
      size_t found;
      cout << "Splitting: " << this->DefinePreprocessingTasksFile[index] << endl;
      found = this->DefinePreprocessingTasksFile[index].find_last_of("/\\");
      cout << " folder: " << this->DefinePreprocessingTasksFile[index].substr(0,found) << endl;
      cout << " file: " << this->DefinePreprocessingTasksFile[index].substr(found+1) << endl;
      mrmlManager->SetTclTaskFilename(this->DefinePreprocessingTasksFile[index].substr(found+1).c_str());

    } 
  else 
    {
      mrmlManager->SetTclTaskFilename(vtkMRMLEMSNode::GetDefaultTclTaskFilename());
    }



}

//----------------------------------------------------------------------------
// function for renaming a member of the task list 
void vtkEMSegmentParametersSetStep::UpdateTaskListIndex(int index) 
{
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();

  this->UpdateLoadedParameterSets();

  //Assuming the mrml manager adds the node to the end.
  if (mrmlManager->GetNthParameterSetName(index))
    {
      // Select the newly created parameter set
      vtkKWMenuButton *menuButton = this->ParameterSetMenuButton->GetWidget();
      menuButton->GetMenu()->SelectItem(index);
    }

  vtkEMSegmentAnatomicalStructureStep *anat_step = this->GetGUI()->GetAnatomicalStructureStep();  
  if (anat_step &&
      anat_step->GetAnatomicalStructureTree() &&
      anat_step->GetAnatomicalStructureTree()->IsCreated())
    {
      anat_step->GetAnatomicalStructureTree()->GetWidget()->DeleteAllNodes();
    }
}


void vtkEMSegmentParametersSetStep::SelectedParameterSetChangedCallback(int index, int flag)
{
  // cout << "vtkEMSegmentParametersSetStep::SelectedParameterSetChangedCallback " << index << " " <<  flag << endl;
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();

  // New Parameters

  if (index < 0)
    {
      vtkErrorMacro("Index has to be greater 0");
      return;
    }

  // Delete all other EMS nodes 
  vtkMRMLNode* node =  mrmlManager->GetMRMLScene()->GetNthNodeByClass(index, "vtkMRMLEMSNode");
  if (node == NULL)
    {
    vtkErrorMacro("Did not find nth template builder node in scene: " << index);
    return;
    }
   // We have to do this bc otherwise strange things can happen when more then two emsnodes exist
   mrmlManager->RemoveAllEMSNodesButOne(node);
   // Now only one is left
   mrmlManager->SetLoadedParameterSetIndex(0);

  vtkEMSegmentAnatomicalStructureStep *anat_step =
    this->GetGUI()->GetAnatomicalStructureStep();

  if (anat_step &&
      anat_step->GetAnatomicalStructureTree() &&
      anat_step->GetAnatomicalStructureTree()->IsCreated())
    {
    anat_step->GetAnatomicalStructureTree()->GetWidget()->DeleteAllNodes();
    }

  std::string tclFileName = this->GetGUI()->GetLogic()->DefineTclTaskFullPathName(this->GetSlicerApplication(),mrmlManager->GetTclTaskFilename());

  this->SourceTclFile(tclFileName.c_str());
  if (flag && (!this->SettingSegmentationMode(0)))
    {
    return ;
    }

  this->GUI->GetWizardWidget()->GetWizardWorkflow()->AttemptToGoToNextStep(); 
}


int vtkEMSegmentParametersSetStep::SettingSegmentationMode(int flag) 
{
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  dlg2->SetApplication( this->GetApplication());
  dlg2->SetMasterWindow(NULL);
  dlg2->SetOptions(vtkKWMessageDialog::InvokeAtPointer | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault);
  dlg2->SetTitle("How do you want to proceed?");
  dlg2->SetStyleToOkOtherCancel();
  dlg2->SetOKButtonText("Adjust Parameters");                   // Advanced
  dlg2->GetOKButton()->SetBalloonHelpString("Fine tune task specific parameters before segmenting the input scans");
  dlg2->SetOtherButtonText("Use Existing Setting"); // Simple
  dlg2->GetOtherButton()->SetBalloonHelpString("Simply use predefined setting of the selected task for segmenting images");
  dlg2->Create();
  // dlg2->SetSize(400, 150);
  dlg2->GetOKButton()->SetWidth(17);
  dlg2->GetOtherButton()->SetWidth(20);
  dlg2->GetCancelButton()->SetWidth(6);
  this->Script("pack %s -side left -expand yes -padx 2", 
               dlg2->GetOtherButton()->GetWidgetName());

  if (flag)
    {
      dlg2->SetText("In which mode do you want to proceed segmenting your data?\n\n Note, downloading the default setting will reset your slicer scene and might take time depending on your network connection !");
    }
  else
    {
      dlg2->SetText("In which mode do you want to proceed segmenting your data?");
    }

  dlg2->Invoke();
  int status = dlg2->GetStatus();
  dlg2->Delete();

  switch  (status)
    {
    case vtkKWMessageDialog::StatusOther : 
      this->GetGUI()->SetSegmentationModeToSimple();
      return 1;
      
    case vtkKWMessageDialog::StatusOK :
      this->GetGUI()->SetSegmentationModeToAdvanced();
      return 1;
    }

  return 0;       
}

//----------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//---------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::RenameApplyCallback(const char* newName)
{
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!this->RenameEntry || !this->RenameEntry->GetWidget() || !mrmlManager)
   {
     return;
   }
  const char* newName2 = this->RenameEntry->GetWidget()->GetValue();

  if ( strlen(newName2) < 1 ) {
    vtkKWMessageDialog::PopupMessage(this->GetApplication(),NULL,"Error", "Invalid Task Name", vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::InvokeAtPointer);
    return;
  }
  mrmlManager->SetNthParameterName(this->RenameIndex,newName2);
  this->HideRenameEntry();
  this->UpdateTaskListIndex(this->RenameIndex);
  this->GUI->GetWizardWidget()->GetWizardWorkflow()->AttemptToGoToNextStep();
}

//---------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::HideRenameEntry()
{
  if ( !this->RenameTopLevel )
    {
    return;
    }
  this->RenameTopLevel->Withdraw();
}


//---------------------------------------------------------------------------
void vtkEMSegmentParametersSetStep::PopUpRenameEntry(int index)
{
  vtkSlicerApplication *app = vtkSlicerApplication::GetInstance();
  this->RenameIndex = index;
  if ( !this->RenameTopLevel )
    {
    this->RenameTopLevel = vtkKWTopLevel::New ( );
    this->RenameTopLevel->SetApplication ( app );
    this->RenameTopLevel->ModalOn();
    this->RenameTopLevel->Create ( );
    this->RenameTopLevel->SetMasterWindow ( app->GetApplicationGUI()->GetMainSlicerWindow() );
    this->RenameTopLevel->HideDecorationOn ( );
    this->RenameTopLevel->Withdraw ( );
    this->RenameTopLevel->SetBorderWidth ( 2 );
    this->RenameTopLevel->SetReliefToGroove ( );

    vtkKWFrame *popUpFrameP = vtkKWFrame::New ( );
    popUpFrameP->SetParent ( this->RenameTopLevel );
    popUpFrameP->Create ( );
    app->Script ( "pack %s -side left -anchor w -padx 2 -pady 2 -fill x -fill y -expand n", popUpFrameP->GetWidgetName ( ) );

    this->RenameEntry = vtkKWEntryWithLabel::New();
    this->RenameEntry->SetParent( popUpFrameP );
    this->RenameEntry->Create();
    this->RenameEntry->SetLabelText( "New Task Name: " );
    this->RenameEntry->GetWidget()->SetCommandTrigger(vtkKWEntry::TriggerOnReturnKey); 
    this->RenameEntry->GetWidget()->SetCommand (this, "RenameApplyCallback");
    app->Script ( "grid %s -row 0 -column 0 -padx 2 -pady 8", this->RenameEntry->GetWidgetName() );

    // Create the Parameters Set Menu button

    if (!this->PreprocessingMenuButton)
      {
      this->PreprocessingMenuButton = vtkKWMenuButtonWithLabel::New();
      }

    if (!this->PreprocessingMenuButton->IsCreated())
      {
      this->PreprocessingMenuButton->SetParent(popUpFrameP);
      this->PreprocessingMenuButton->Create();
      this->PreprocessingMenuButton->GetLabel()->SetWidth(
        EMSEG_WIDGETS_LABEL_WIDTH - 10);
      this->PreprocessingMenuButton->SetLabelText(
        "Preprocessing:");
      this->PreprocessingMenuButton->GetWidget()->SetWidth(
        EMSEG_MENU_BUTTON_WIDTH + 10);
      this->PreprocessingMenuButton->SetBalloonHelpString(
        "Select Standard Task.");
      }

    if (!this->PreprocessingMenuButton ||
       !this->PreprocessingMenuButton->IsCreated())
      {
      return;
      }
    char buffer[256];
    vtkKWMenu *preprocessing_menu = this->PreprocessingMenuButton->GetWidget()->GetMenu();
    preprocessing_menu->DeleteAllItems();
    for (int i = 0 ; i < (int)this->DefinePreprocessingTasksName.size(); i++)
      {
      sprintf(buffer, "SelectedPreprocessingChangedCallback %d 1", i);
      preprocessing_menu->AddRadioButton(DefinePreprocessingTasksName[i].c_str(), this, buffer);
      }

    sprintf(buffer, "SelectedPreprocessingChangedCallback %d 1", -1);
    preprocessing_menu->AddRadioButton("None", this, buffer);



    app->Script ( "grid %s -row 1 -column 0 -padx 2 -pady 2", this->PreprocessingMenuButton->GetWidgetName() );
    popUpFrameP->Delete();


    vtkKWFrame *fP = vtkKWFrame::New();
    fP->SetParent ( popUpFrameP);
    fP->Create();
    app->Script ( "grid %s -row 2 -column 0 -columnspan 1 -pady 8 -sticky ew", fP->GetWidgetName() );

    this->RenameApply = vtkKWPushButton::New ();
    this->RenameApply->SetParent (fP);
    this->RenameApply->Create ( );
    this->RenameApply->SetText ("Apply");
    this->RenameApply->SetCommand (this, "RenameApplyCallback blub");
    app->Script ( "pack %s -side left -padx 4 -anchor c", RenameApply->GetWidgetName());

    this->RenameCancel = vtkKWPushButton::New();
    this->RenameCancel->SetParent (  fP );
    this->RenameCancel->Create();
    this->RenameCancel->SetText ( "Cancel");
    this->RenameCancel->SetCommand (this, "HideRenameEntry");
    // app->Script ( "pack %s  -side left -padx 4 -anchor c",this->RenameCancel->GetWidgetName() );

    fP->Delete();
    

    }    

  this->RenameEntry->GetWidget()->SetValue("");

  // Get the position of the mouse, position the popup
  int x, y;
  vtkKWTkUtilities::GetMousePointerCoordinates(this->ParameterSetMenuButton->GetWidget()->GetMenu(), &x, &y);
  this->RenameTopLevel->SetPosition(x, y);
  app->ProcessPendingEvents();
  this->RenameTopLevel->DeIconify();
  this->RenameTopLevel->Raise();

  this->RenameEntry->GetWidget()->SelectAll();
  this->RenameEntry->GetWidget()->Focus();
}

//----------------------------------------------------------------------------
int vtkEMSegmentParametersSetStep::LoadDefaultTask(int index, bool warningFlag)
{
  // cout << "vtkEMSegmentParametersSetStep::LoadDefaultTask " << index << " " << warningFlag << endl;
  if (index < 0 || index >  int(this->pssDefaultTasksName.size() -2) )
    {
      vtkErrorMacro("Index is not defined");
      return 1;
    }


  // Load Task 
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (this->LoadDefaultData(pssDefaultTasksFile[index].c_str(),warningFlag))
    {
      // Error occured 
      return 1;
    }


  // Remove the default selection entry from the menu,
  this->PopulateLoadedParameterSets();
      
  // Figure out the menu index number of the default task that was just loaded
  // and go to the next step 
  int numSets = mrmlManager->GetNumberOfParameterSets();
  for(int i = 0; i < numSets; i++)
    {
    const char *name = mrmlManager->GetNthParameterSetName(i);
    if (name && !strcmp(name,pssDefaultTasksName[index].c_str()))
      {
      // Select the Node
      this->SelectedParameterSetChangedCallback(i,0);
      break;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkEMSegmentParametersSetStep::LoadDefaultData(const char *mrmlFile, bool warningFlag)
{
  
  this->GetGUI()->SetSegmentationModeToAdvanced();
  if (warningFlag)
    {
      // do not want to proceed
      if (!this->SettingSegmentationMode(1))
      {
         return 1;
      }
    }

  // Load MRML File whose location is defined in the tcl file 
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkMRMLScene *scene = mrmlManager->GetMRMLScene();
  // vtksys_stl::string mrmlFile(vtkSlicerApplication::SafeDownCast(this->GetGUI()->GetApplication())->Script("::EMSegmenterParametersStepTcl::DefineMRMLFile"));
  scene->SetURL(mrmlFile);
 
  mrmlManager->RemoveAllEMSNodes();
  // bc of task file cannot do a connect ! if (!scene->Connect()) - also not as user friendly bc otherwise data gets lost that was loaded in beforehand 
  if (!scene->Import()) 
    {
      vtksys_stl::string msg= vtksys_stl::string("Could not load mrml file ") +  mrmlFile  ;
      vtkKWMessageDialog::PopupMessage(this->GetApplication(),NULL,"Load Error", msg.c_str(), vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::InvokeAtPointer);
      return 1;
    }

  if(scene->GetErrorCode())
    {
      vtksys_stl::string msg= vtksys_stl::string("Corrupted File", "MRML file") +  mrmlFile +  vtksys_stl::string(" was corrupted or could not be loaded");
      vtkErrorMacro("ERROR: Failed to connect to the data. Error code: " << scene->GetErrorCode()  << " Error message: " << scene->GetErrorMessage());
      vtkKWMessageDialog::PopupMessage(this->GetApplication(),NULL, "Corrupted File", msg.c_str() , vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::InvokeAtPointer);
      return 1;
    }

  cout << "==========================================================================" << endl;
  cout << "== Completed loading task data " << endl;
  cout << "==========================================================================" << endl;

  this->GetGUI()->GetApplicationGUI()->SelectModule("EMSegmenter");

  return 0;
}

void vtkEMSegmentParametersSetStep::AddDefaultTasksToList(const char* FilePath)
{
  vtkDirectory *dir = vtkDirectory::New();
  // Do not give out an error message here bc it otherwise comes up when loading slicer 
  // the path might simply not be created !
  if (!dir->Open(FilePath))
    {
      dir->Delete();
      return;
    }
    
  for (int i = 0; i < dir->GetNumberOfFiles(); i++)
    {
    vtksys_stl::string filename = dir->GetFile(i);
    //skip ., ..,  if it is not a mrml extension, or a directory
    if (strcmp(filename.c_str(), ".") == 0)
      {
      continue;
      }
    if (strcmp(filename.c_str(), "..") == 0)
      {
      continue;
      }
    if (strcmp(vtksys::SystemTools::GetFilenameExtension(filename.c_str()).c_str(), ".mrml") != 0)
      {
      continue;
      }

    //if (strcmp(filename.c_str(), vtkMRMLEMSNode::GetDefaultTclTaskFilename()) == 0)
    //  {
    //  continue;
    //  }
 
    vtksys_stl::string tmpFullFileName = vtksys_stl::string(FilePath) + vtksys_stl::string("/") + filename.c_str();
    vtksys_stl::string fullFileName = vtksys::SystemTools::ConvertToOutputPath(tmpFullFileName.c_str());
    if (vtksys::SystemTools::FileIsDirectory(fullFileName.c_str()))
      {
    continue;
      }
    
    // Generate Name of Task from File name
    vtksys_stl::string taskName = this->GetGUI()->GetMRMLManager()->TurnDefaultMRMLFileIntoTaskName(filename.c_str());

    // make sure that file is not already in the list 
    int existFlag = 0;
    for (i=0; i < int(pssDefaultTasksName.size()); i++)
      {
    if (!this->pssDefaultTasksName[i].compare(taskName))
      {
        existFlag =1;
      }
      }
    if (existFlag)
      {
    continue;
      }
    // Add to List
    this->pssDefaultTasksFile.push_back(fullFileName);
    this->pssDefaultTasksName.push_back(taskName);
    this->DefinePreprocessingTasksFile.push_back(fullFileName);
    this->DefinePreprocessingTasksName.push_back(taskName);
    }
  dir->Delete();
}

//-------------vtksys_stl::string ---------------------------------------------------------------
void vtkEMSegmentParametersSetStep::DefineDefaultTasksList()
{
  //  cout << "-------- DefineDefaultTasksList Start" << endl;
  // set define list of parameters
  this->pssDefaultTasksName.clear();
  this->pssDefaultTasksFile.clear();
  this->DefinePreprocessingTasksName.clear();
  this->DefinePreprocessingTasksFile.clear();

  this->AddDefaultTasksToList(this->GetGUI()->GetLogic()->GetTclTaskDirectory().c_str());
  this->AddDefaultTasksToList(this->GetGUI()->GetLogic()->GetTemporaryTaskDirectory(this->GetSlicerApplication()).c_str());
 
  if (!this->pssDefaultTasksFile.size()) 
    {
    vtkWarningMacro("No default tasks found");
    }
  // The last one is always "Create New" 
  this->pssDefaultTasksFile.push_back(vtksys_stl::string(""));
  this->pssDefaultTasksName.push_back("Create new task");
  // cout << "-------- DefineDefaultTasksList End" << endl;
}

void vtkEMSegmentParametersSetStep::_Validate(int flag)
{
  if (flag) {
    if (this->SettingSegmentationMode(0))
    {
      vtkKWWizardWorkflow *wizard_workflow = this->GetGUI()->GetWizardWidget()->GetWizardWorkflow();
      wizard_workflow->PushInput(vtkKWWizardStep::GetValidationFailedInput());
      wizard_workflow->ProcessInputs();
      return;
    }
  } 
}
