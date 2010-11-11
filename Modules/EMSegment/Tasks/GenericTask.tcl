package require Itcl

#########################################################
#
if {0} { ;# comment

    This is function is executed by EMSegmenter

    # TODO :

}
#
#########################################################

#
# namespace procs
#

#
# This is the default processing pipeline - which does not do anything
#

namespace eval EMSegmenterPreProcessingTcl {

    #
    # Variables
    #

    ## Slicer
    variable GUI
    variable LOGIC
    variable SCENE

    ## EM GUI/MRML
    variable preGUI
    variable mrmlManager
    variable workingDN

    ## Input/Output
    variable inputAtlasNode
    # Variables used for segmentation
    # Input/Output subject specific scans - by default this is defined by the input scans which are aligned with each other
    variable subjectNode
    # spatial priors aligned to subject node
    variable outputAtlasNode

    ## Task Specific GUI variables
    variable TextLabelSize 1
    variable CheckButtonSize 0
    variable VolumeMenuButtonSize 0
    variable TextEntrySize 0

    #
    # General Utility Functions
    #
    proc DeleteNode { NODE } {
        variable SCENE
        $SCENE RemoveNode $NODE
        # Note:
        #Do not need to do it as the destructor does it automatically
        #set displayNode [$NODE GetDisplayNode]
        #[$NODE GetDisplayNode]
        # if {$displayNode} { $SCENE RemoveNode $displayNode }
    }

    # vtkMRMLVolumeNode *volumeNode, const char *name)
    proc CreateVolumeNode { volumeNode name } {
        variable SCENE
        if {$volumeNode == ""} { return "" }
        # clone the display node
        set clonedDisplayNode [vtkMRMLScalarVolumeDisplayNode New]
        $clonedDisplayNode CopyWithScene [$volumeNode GetDisplayNode]
        $SCENE AddNode $clonedDisplayNode
        set dispID [$clonedDisplayNode GetID]
        $clonedDisplayNode Delete

        set clonedVolumeNode [vtkMRMLScalarVolumeNode New]
        $clonedVolumeNode CopyWithScene $volumeNode
        $clonedVolumeNode SetAndObserveStorageNodeID ""
        $clonedVolumeNode SetName "$name"
        $clonedVolumeNode SetAndObserveDisplayNodeID $dispID

        if {0} {
            # copy over the volume's data
            $clonedVolumeData [vtkImageData New]
            $clonedVolumeData DeepCopy [volumeNode GetImageData]
            $clonedVolumeNode SetAndObserveImageData $clonedVolumeData
            $clonedVolumeNode SetModifiedSinceRead 1
            $clonedVolumeData Delete
        } else {
            $clonedVolumeNode SetAndObserveImageData ""
        }

        # add the cloned volume to the scene
        $SCENE AddNode $clonedVolumeNode
        set volID [$clonedVolumeNode GetID]
        $clonedVolumeNode Delete
        # Have to do it this way bc unlike in c++ the link to $clonedVolumeNode gets deleted
        return [$SCENE GetNodeByID $volID]
    }

    proc PrintError { TEXT } {
        variable LOGIC
        $LOGIC PrintText "TCL: ERROR: EMSegmenterPreProcessingTcl::${TEXT}"
    }


    # update subjectNode with new volumes - and delete the old ones
    proc UpdateSubjectNode { newSubjectVolumeNodeList } {
        variable subjectNode
        # Update Aligned Target Nodes
        set inputNum [$subjectNode GetNumberOfVolumes]
        for { set i 0 } {$i < $inputNum } { incr i } {
            set newVolNode [lindex $newSubjectVolumeNodeList $i]
            if {$newVolNode == "" } {
                PrintError "Run: Intensity corrected target node is incomplete !"
                return 1
            }
            set oldSubjectNode [$subjectNode GetNthVolumeNode $i]
            # Set up the new ones
            $subjectNode SetNthVolumeNodeID $i [$newVolNode GetID]
            # Remove old volumes associated with subjectNode - if you delete right away then subjectNode is decrease
            DeleteNode $oldSubjectNode
        }
        return 0
    }


    # ----------------------------------------------------------------------------
    # We have to create this function so that we can run it in command line mode
    #
    proc GetCheckButtonValueFromMRML { ID } {
        return [GetEntryValueFromMRML "C" $ID]
    }

    proc GetVolumeMenuButtonValueFromMRML { ID } {
        variable mrmlManager
        set MRMLID [GetEntryValueFromMRML "V" $ID]
        if { ("$MRMLID" != "") && ("$MRMLID" != "NULL") } {
            return [$mrmlManager MapMRMLNodeIDToVTKNodeID $MRMLID]
        }
        return 0
    }

    proc GetTextEntryValueFromMRML { ID } {
        return [GetEntryValueFromMRML "E" $ID]
    }


    proc GetEntryValueFromMRML { Type ID } {
        variable mrmlManager
        set TEXT [string range [string map { "|" "\} \{" } "[[$mrmlManager GetNode] GetTaskPreprocessingSetting]"] 1 end]
        set TEXT "${TEXT}\}"
        set index 0
        foreach ARG $TEXT {
            if {"[string index $ARG 0]" == "$Type" } {
                if { $index == $ID } {
                    return "[string range $ARG 1 end]"
                }
                incr index
            }
        }
        return ""
    }


    #
    # Preprocessing Functions
    #
    proc InitVariables { {initLOGIC ""} {initManager ""} {initPreGUI "" } } {
        variable GUI
        variable preGUI
        variable LOGIC
        variable SCENE
        variable mrmlManager
        variable workingDN
        variable subjectNode
        variable inputAtlasNode
        variable outputAtlasNode

        
        set GUI $::slicer3::Application
        if { $GUI == "" } {
            puts stderr "ERROR: GenericTask: InitVariables: GUI not defined"
            return 1
        }

        if { $initLOGIC == "" } {
            set MOD [$GUI GetModuleGUIByName "EMSegmenter"]
            if {$MOD == ""} {
                puts stderr "ERROR: GenericTask: InitVariables: EMSegmenter not defined"
                return 1
            }
            set LOGIC [$MOD GetLogic]
            if { $LOGIC == "" } {
                puts stderr "ERROR: GenericTask: InitVariables: LOGIC not defined"
                return 1
            }
        } else {
            set LOGIC $initLOGIC
        }

        # Do not move it before bc LOGIC is not defined until here 

         $LOGIC PrintText "TCL: =========================================="
         $LOGIC PrintText "TCL: == Init Variables"
         $LOGIC PrintText "TCL: =========================================="
  

        if { $initManager == "" } {
            set MOD [$::slicer3::Application GetModuleGUIByName "EMSegmenter"]
            if {$MOD == ""} {
                PrintError "InitVariables: EMSegmenter not defined"
                return 1
            }

            set mrmlManager [$MOD GetMRMLManager]
            if { $mrmlManager == "" } {
                PrintError "InitVariables: mrmManager not defined"
                return 1
            }
        } else {
            set mrmlManager $initManager

        }

        set SCENE [$mrmlManager GetMRMLScene]
        if { $SCENE == "" } {
            PrintError "InitVariables: SCENE not defined"
            return 1
        }

        set workingDN [$mrmlManager GetWorkingDataNode]
        if { $workingDN == "" } {
            $LOGIC PrintText "TCL: EMSegmenterPreProcessingTcl::InitVariables: WorkingData not defined"
            return 1
        }

        if {$initPreGUI == "" } {
            set MOD [$::slicer3::Application GetModuleGUIByName "EMSegmenter"]
            if {$MOD == ""} {
                PrintError "InitVariables: EMSegmenter not defined"
                return 1
            }

            set preGUI [$MOD GetPreProcessingStep]
            if { $preGUI == "" } {
                PrintError "InitVariables: PreProcessingStep not defined"
                return 1
            }
        } else {
            set preGUI $initPreGUI
        }

        # All other Variables are defined when running the pipeline as they are the volumes
        # Define subjectNode when initializing pipeline
        set subjectNode ""
        set inputAtlasNode ""
        set outputAtlasNode ""

        return 0
    }



    #------------------------------------------------------
    # return 0 when no error occurs
    proc ShowUserInterface { } {
        variable preGUI
        variable LOGIC

        if { [InitVariables] } {
            puts stderr "ERROR: GernicTask.tcl: ShowUserInterface: Not all variables are correctly defined!"
            return 1
        }

        $LOGIC PrintText "TCL: Preprocessing GenericTask"

        # -------------------------------------
        # Define Interface Parameters
        # -------------------------------------
        $preGUI DefineTextLabel "No preprocessing defined for this task!" 0
    }

    # ----------------------------------------------------------------
    # Make Sure that input volumes all have the same resolution
    # from StartPreprocessingTargetToTargetRegistration
    # ----------------------------------------------------------------
    proc RegisterInputImages { inputTargetNode fixedTargetImageIndex } {
        variable workingDN
        variable mrmlManager
        variable LOGIC
        variable SCENE

         $LOGIC PrintText "TCL: =========================================="
         $LOGIC PrintText "TCL: == Register Input Images --"
         $LOGIC PrintText "TCL: =========================================="
        # ----------------------------------------------------------------
        # set up rigid registration
        set alignedTarget [$workingDN GetAlignedTargetNode]
        if { $alignedTarget == "" } {
            # input scan does not have to be aligned
            set alignedTarget [$mrmlManager CloneTargetNode $inputTargetNode "Aligned"]
            $workingDN SetAlignedTargetNodeID [$alignedTarget GetID]
        } else {
            $mrmlManager SynchronizeTargetNode $inputTargetNode $alignedTarget "Aligned"
        }

        for { set i 0 } { $i < [$alignedTarget GetNumberOfVolumes] } {incr i} {
            set intputVolumeNode($i) [$inputTargetNode GetNthVolumeNode $i]
            if { $intputVolumeNode($i) == "" } {
                PrintError "RegisterInputImages: the ${i}th input node is not defined!"
                return 1
            }

            set intputVolumeData($i) [$intputVolumeNode($i) GetImageData]
            if { $intputVolumeData($i) == "" } {
                PrintError "RegisterInputImages: the ${i}the input node has no image data defined !"
                return 1
            }

            set outputVolumeNode($i) [$alignedTarget GetNthVolumeNode $i]
            if { $outputVolumeNode($i) == "" } {
                PrintError "RegisterInputImages: the ${i}th aligned input node is not defined!"
                return 1
            }

            set outputVolumeData($i) [$outputVolumeNode($i) GetImageData]
            if { $outputVolumeData($i) == "" } {
                PrintError "RegisterInputImages: the ${i}the output node has no image data defined !"
                return 1
            }
        }
        set fixedVolumeNode $outputVolumeNode($fixedTargetImageIndex)
        set fixedImageData $outputVolumeData($fixedTargetImageIndex)

        # ----------------------------------------------------------------
        # inform the user what happens next
        if {[$mrmlManager GetEnableTargetToTargetRegistration] } {
             $LOGIC PrintText "TCL: ===> Register Target To Target "
        } else {
             $LOGIC PrintText "TCL: ===> Skipping Registration of Target To Target "
        }

        # perfom "rigid registration" or "resample only"
        for { set i 0 } {$i < [$alignedTarget GetNumberOfVolumes] } { incr i } {
            if { $i == $fixedTargetImageIndex } {
                continue;
            }

            set movingVolumeNode $intputVolumeNode($i)
            set outVolumeNode $outputVolumeNode($i)

            if {[$mrmlManager GetEnableTargetToTargetRegistration] } {
                # ------------------------------------------------------------
                # Perform Rigid Registration - old style
                set backgroundLevel [$LOGIC GuessRegistrationBackgroundLevel $movingVolumeNode]
                if { 0 } {
                    # Old Style of Slicer 3.4
                    set alignType [$mrmlManager GetRegistrationTypeFromString AtlasToTargetAffineRegistrationRigidMMI]
                    set interType [$mrmlManager GetInterpolationTypeFromString InterpolationLinear]
                    set fixedRASToMovingRASTransform [vtkTransform New]
                    $LOGIC SlicerRigidRegister $fixedVolumeNode $movingVolumeNode $outVolumeNode $fixedRASToMovingRASTransform $alignType $interType $backgroundLevel
                    $fixedRASToMovingRASTransform Delete;
                } else {
                    # Using BRAINS suite
                    set transformNode [BRAINSRegistration $fixedVolumeNode $movingVolumeNode $outVolumeNode $backgroundLevel "Rigid" 0]
                    if { $transformNode == "" } {
                        PrintError "Transform node is null"
                        return 1
                    }
                     $LOGIC PrintText "TCL: === Just for debugging $transformNode [$transformNode GetName] [$transformNode GetID]"
                    set outputNode [vtkMRMLScalarVolumeDisplayNode New]
                    $outputNode SetName "blub1"
                    $SCENE AddNode $outputNode
                    set outputNodeID [$outputNode GetID]
                    $outputNode Delete

                    if { [BRAINSResample $movingVolumeNode $fixedVolumeNode [$SCENE GetNodeByID $outputNodeID] $transformNode $backgroundLevel] } {
                        return 1
                    }
                    ## $SCENE RemoveNode $transformNode
                }

                # ------------------------------------------------------------
                # Here comes new rigid registration later
            } else {
                # Just creates output with same dimension as fixed volume
                $LOGIC StartPreprocessingResampleAndCastToTarget $movingVolumeNode $fixedVolumeNode $outVolumeNode
            }
        }
        # ----------------------------------------------------------------
        # Clean up
        $workingDN SetAlignedTargetNodeIsValid 1
        return 0
    }


    #------------------------------------------------------
    # from StartPreprocessingTargetToTargetRegistration
    #------------------------------------------------------
    proc SkipAtlasRegistration { } {
        variable workingDN
        variable mrmlManager
        variable LOGIC
        variable subjectNode
        variable inputAtlasNode
        variable outputAtlasNode
         $LOGIC PrintText "TCL: =========================================="
         $LOGIC PrintText "TCL: == Skip Atlas Registration"
         $LOGIC PrintText "TCL: =========================================="

        # This function makes sure that the "output atlas" is identically to the "input atlas".
        # Each volume of the "output atlas" will then be resampled to the resolution of the "fixed target volume"
        # The "output atlas will be having the same ScalarType as the "fixed target volume'". There is no additionally cast necessary.

        set fixedTargetChannel 0

        # ----------------------------------------------------------------
        #  makes sure that the "output atlas" is identically to the "input atlas"
        # ----------------------------------------------------------------
        if { $outputAtlasNode == "" } {
             $LOGIC PrintText "TCL: Atlas was empty"
            #  $LOGIC PrintText "set outputAtlasNode \[$mrmlManager CloneAtlasNode $inputAtlasNode \"AlignedAtlas\"\] "
            set outputAtlasNode [$mrmlManager CloneAtlasNode $inputAtlasNode "Aligned"]
            $workingDN SetAlignedAtlasNodeID [$outputAtlasNode GetID]
        } else {
             $LOGIC PrintText "TCL: Atlas was just synchronized"
            $mrmlManager SynchronizeAtlasNode $inputAtlasNode $outputAtlasNode AlignedAtlas
        }

        # ----------------------------------------------------------------
        # set the fixed target volume
        # ----------------------------------------------------------------
        set fixedTargetVolumeNode [$subjectNode GetNthVolumeNode $fixedTargetChannel]
        if { [$fixedTargetVolumeNode GetImageData] == "" } {
            PrintError "SkipAtlasRegistration: Fixed image is null, skipping resampling"
            return 1;
        }


        # ----------------------------------------------------------------
        # Make Sure that atlas volumes all have the same resolution as input
        # ----------------------------------------------------------------
        for { set i 0 } {$i < [$outputAtlasNode GetNumberOfVolumes] } { incr i } {
            set movingVolumeNode [$inputAtlasNode GetNthVolumeNode $i]
            set outputVolumeNode [$outputAtlasNode GetNthVolumeNode $i]
            $LOGIC StartPreprocessingResampleAndCastToTarget $movingVolumeNode $fixedTargetVolumeNode $outputVolumeNode
        }

         $LOGIC PrintText "TCL: EMSEG: Atlas-to-target resampling complete."
        $workingDN SetAlignedAtlasNodeIsValid 1
        return 0
    }

    # -----------------------------------------------------------
    # sets up all variables
    # Define the three volume relates Input nodes to the pipeline
    # - subjectNode
    # - inputAtlasNode
    # - outputAtasNode
    # -----------------------------------------------------------
    proc InitPreProcessing { } {
        variable mrmlManager
        variable LOGIC
        variable workingDN
        variable subjectNode
        variable inputAtlasNode
        variable outputAtlasNode
        $LOGIC PrintText "TCL: =========================================="
        $LOGIC PrintText "TCL: == InitPreprocessing"
        $LOGIC PrintText "TCL: =========================================="

        # TODO: Check for
        # - environment variables  and
        # - command line executables
        #set PLUGINS_DIR "$::env(Slicer3_PLUGINS_DIR)"
        #if { $PLUGINS_DIR == "" } {
        #    PrintError "InitPreProcessing: Environmet variable not set corretly"
        #    return 1
        #}

        # -----------------------------------------------------------
        # Check and set valid variables
        if { [$mrmlManager GetGlobalParametersNode] == 0 } {
            PrintError "InitPreProcessing: Global parameters node is null, aborting!"
            return 1
        }

        $LOGIC StartPreprocessingInitializeInputData


        # -----------------------------------------------------------
        # Define subject Node
        # this should be the first step for any preprocessing
        # from StartPreprocessingTargetToTargetRegistration
        # -----------------------------------------------------------

        set inputTarget [$workingDN GetInputTargetNode]
        if {$inputTarget == "" } {
            PrintError "InitPreProcessing: InputTarget not defined"
            return 1
        }

        if {[RegisterInputImages $inputTarget 0] } {
            PrintError "InitPreProcessing: Target-to-Target failed!"
            return 1
        }

        set subjectNode [$workingDN GetAlignedTargetNode]
        if {$subjectNode == "" } {
            PrintError "InitPreProcessing: cannot retrieve Aligned Target Node !"
            return 1
        }

        # -----------------------------------------------------------
        # Define Atlas
        # -----------------------------------------------------------
        set inputAtlasNode [$workingDN GetInputAtlasNode]
        if {$inputAtlasNode == "" } {
            PrintError "InitPreProcessing: InputAtlas not defined"
            return 1
        }

        set outputAtlasNode [$workingDN GetAlignedAtlasNode]


        return 0
    }


    # returns transformation when no error occurs
    # now call commandline directly

    proc BRAINSResample { inputVolumeNode referenceVolumeNode outVolumeNode transformationNode backgroundLevel } {
        variable SCENE
        variable LOGIC

        set  ValueList ""

        if { $inputVolumeNode == "" || [$inputVolumeNode GetImageData] == "" } {
            PrintError "BRAINSResample: volume node to be warped is not correctly defined"
            return 1
        }

        if { $referenceVolumeNode == "" || [$referenceVolumeNode GetImageData] == "" } {
            PrintError "BRAINSResample: reference image node is not correctly defined"
            return 1
        }

        if { $transformationNode == "" } {
            PrintError "BRAINSResample: transformation node not correctly defined"
            return 1
        }

        if { $outVolumeNode == "" } {
            PrintError "BRAINSResample: output volume node not correctly defined"
            return 1
        }

        lappend ValueList "Float defaultValue $backgroundLevel"

        set referenceVolume [$referenceVolumeNode GetImageData]
        set scalarType [$referenceVolume GetScalarTypeAsString]
        switch -exact "$scalarType" {
            "bit" { lappend ValueList "String pixelType binary" }
            "unsigned char" { lappend ValueList "String pixelType uchar" }
            "unsigned short" { lappend ValueList "String pixelType ushort" }
            "unsigned int" { lappend ValueList "String pixelType uint" }
            "short" -
            "int" -
            "float" { lappend ValueList "String pixelType $scalarType" }
            default {
                PrintError "BRAINSResample: cannot resample a volume of type $scalarType"
                return 1
            }
        }

        lappend ValueList "String interpolationMode Linear"

        # Start calling function
        return [BRAINSResampleCLI $inputVolumeNode $referenceVolumeNode $outVolumeNode $transformationNode "$ValueList"]
    }

    proc BRAINSResampleCLI { inputVolumeNode referenceVolumeNode outVolumeNode transformationNode  ValueList } {
        variable SCENE
        variable LOGIC
         $LOGIC PrintText "TCL: =========================================="
         $LOGIC PrintText "TCL: == Resample Image CLI"
         $LOGIC PrintText "TCL: =========================================="

        set PLUGINS_DIR "$::env(Slicer3_PLUGINS_DIR)"
        set CMD "${PLUGINS_DIR}/BRAINSResample "

        set tmpFileName [WriteDataToTemporaryDir $inputVolumeNode Volume]
        set RemoveFiles "$tmpFileName"
        if { $tmpFileName == "" } {
            return 1
        }
        set CMD "$CMD --inputVolume $tmpFileName"

        set tmpFileName [WriteDataToTemporaryDir $referenceVolumeNode Volume]
        set RemoveFiles "$RemoveFiles $tmpFileName"
        if { $tmpFileName == "" } { return 1 }
        set CMD "$CMD --referenceVolume $tmpFileName"

        set tmpFileName [WriteDataToTemporaryDir $transformationNode Transform]
        set RemoveFiles "$RemoveFiles $tmpFileName"
        if { $tmpFileName == "" } { return 1 }
        set CMD "$CMD --warpTransform $tmpFileName"

        set outVolumeFileName [CreateTemporaryFileName $outVolumeNode]
        if { $outVolumeFileName == "" } { return 1 }
        set CMD "$CMD --outputVolume $outVolumeFileName"

        foreach ATT $ValueList {
            set CMD "$CMD --[lindex $ATT 1] [lindex $ATT 2]"
        }

         $LOGIC PrintText "TCL: Executing $CMD"
        catch { eval exec $CMD } errmsg
         $LOGIC PrintText "TCL: $errmsg"


        # Write results back to scene
        # This does not work $::slicer3::ApplicationLogic RequestReadData [$outVolumeNode GetID] $outVolumeFileName 0 1
        ReadDataFromDisk $outVolumeNode $outVolumeFileName Volume
        file delete -force $outVolumeFileName

        return 0
    }

    proc CMTKResampleCLI { inputVolumeNode referenceVolumeNode outVolumeNode transformDirName } {
        variable SCENE
        variable LOGIC
        $LOGIC PrintText "TCL: =========================================="
        $LOGIC PrintText "TCL: == Resample Image CLI : CMTKResampleCLI "
        $LOGIC PrintText "TCL: =========================================="


        set CMD "[$::slicer3::Application GetExtensionsInstallPath]"
        set svnrevision [$::slicer3::Application GetSvnRevision]
        if { $svnrevision == "" } {
            set CMD "$CMD/15383"
        } else {
            set CMD "$CMD/$svnrevision"
        }
        set CMD "$CMD/CMTK4Slicer/warp"

        set bgValue 0
        set CMD "$CMD -v --linear --pad-out $bgValue"


        set outVolumeFileName [CreateTemporaryFileName $outVolumeNode]
        if { $outVolumeFileName == "" } { return 1 }
        set CMD "$CMD -o $outVolumeFileName"

        set tmpFileName [WriteDataToTemporaryDir $inputVolumeNode Volume]
        set RemoveFiles "$tmpFileName"
        if { $tmpFileName == "" } {
            return 1
        }
        set CMD "$CMD --floating $tmpFileName"

        set tmpFileName [WriteDataToTemporaryDir $referenceVolumeNode Volume]
        set RemoveFiles "$RemoveFiles $tmpFileName"
        if { $tmpFileName == "" } { return 1 }
        set CMD "$CMD $tmpFileName"

        set CMD "$CMD $transformDirName"

        $LOGIC PrintText "TCL: Executing $CMD"
        catch { eval exec $CMD } errmsg
        $LOGIC PrintText "TCL: $errmsg"


        # Write results back to scene
        # This does not work $::slicer3::ApplicationLogic RequestReadData [$outVolumeNode GetID] $outVolumeFileName 0 1
        ReadDataFromDisk $outVolumeNode $outVolumeFileName Volume
        file delete -force $outVolumeFileName

        return 0
    }


    proc WaitForDataToBeRead { } {
        variable LOGIC
         $LOGIC PrintText "TCL: Size of ReadDataQueue: $::slicer3::ApplicationLogic GetReadDataQueueSize [$::slicer3::ApplicationLogic GetReadDataQueueSize]"
        set i 20
        while { [$::slicer3::ApplicationLogic GetReadDataQueueSize] && $i} {
            $LOGIC PrintText "Waiting for data to be read... [$::slicer3::ApplicationLogic GetReadDataQueueSize]"
            incr i -1
            update
            after 1000
        }
        if { $i <= 0 } {
            $LOGIC PrintText "Error: timeout waiting for data to be read"
        }
    }

    proc DeleteCommandLine {clmNode } {
        variable LOGIC
        # Wait for jobs to finish
        set waiting 1
        set needToWait { "Idle" "Scheduled" "Running" }

        while {$waiting} {
             $LOGIC PrintText "TCL: Waiting for task..."
            set waiting 0
            set status [$clmNode GetStatusString]
            $LOGIC PrintText "[$clmNode GetName] $status"
            if { [lsearch $needToWait $status] != -1 } {
                set waiting 1
                after 250
            }
        }

        WaitForDataToBeRead
        $clmNode Delete
    }

    proc Run { } {
         variable LOGIC
         $LOGIC PrintText "TCL: =========================================="
         $LOGIC PrintText "TCL: == Preprocess Data"
         $LOGIC PrintText "TCL: =========================================="
        if {[InitPreProcessing]} { return 1}
        # Simply sets the given atlas (inputAtlasNode) to the output atlas (outputAtlasNode)
        SkipAtlasRegistration
        # Remove Transformations
        variable LOGIC
        return 0
    }

    #------------------------------------------------------
    # returns transformation when no error occurs
    proc CreateTemporaryFileName { Node } {
        variable GUI
        if { [$Node GetClassName] == "vtkMRMLScalarVolumeNode" } {
        set NAME "[$Node GetID].nrrd"
        } elseif { [$Node GetClassName] == "vtkMRMLScene"  } {
            set NAME "[file tail [$Node GetURL]]"
     } else {
            # Transform node - check also for bspline
            set NAME "[$Node GetID].mat"
        }

        return "[$GUI GetTemporaryDirectory]/[expr int(rand()*10000)]_$NAME"
    }

    proc WriteDataToTemporaryDir { Node Type} {
        variable GUI
        variable SCENE

        set tmpName [CreateTemporaryFileName $Node]
        if { $tmpName == "" } { return "" }

        if { "$Type" == "Volume" } {
            set out [vtkMRMLVolumeArchetypeStorageNode New]
        } elseif { "$Type" == "Transform" } {
            set out [vtkMRMLTransformStorageNode New]
        } else {
            PrintError "WriteDataToTemporaryDir: Unkown type $Type"
            return 0
        }

        $out SetScene $SCENE
        $out SetFileName $tmpName
        set FLAG [$out WriteData $Node]
        $out Delete
        if  { $FLAG == 0 } {
            PrintError "WriteDataToTemporaryDir: could not write file $tmpName"
            return ""
        }

        return "$tmpName"
    }

    proc ReadDataFromDisk { Node FileName Type } {
        variable GUI
        variable SCENE
        if { [file exists $FileName] == 0 } {
            PrintError "ReadDataFromDisk: $FileName does not exist"
            return 0
        }

        # Load a scalar or vector volume node
        # Need to maintain the original coordinate frame established by
        # the images sent to the execution model
        if { "$Type" == "Volume" } {
            set dataReader [vtkMRMLVolumeArchetypeStorageNode New]
            $dataReader SetCenterImage 0
        } elseif { "$Type" == "Transform" } {
            set dataReader [vtkMRMLTransformStorageNode New]
        } else {
            PrintError "ReadDataFromDisk: Unkown type $Type"
            return 0
        }

        $dataReader SetScene $SCENE
        $dataReader SetFileName "$FileName"
        set FLAG [$dataReader ReadData $Node]
        $dataReader Delete

        if { $FLAG == 0 } {
            PrintError "ReadDataFromDisk : could not read file $FileName"
            return 0
        }
        return 1
    }

    proc BRAINSRegistration { fixedVolumeNode movingVolumeNode outVolumeNode backgroundLevel RegistrationType fastFlag} {
        variable SCENE
        variable LOGIC
        variable GUI
         $LOGIC PrintText "TCL: =========================================="
         $LOGIC PrintText "TCL: == Image Alignment CommandLine: $RegistrationType "
         $LOGIC PrintText "TCL: =========================================="

        set PLUGINS_DIR "$::env(Slicer3_PLUGINS_DIR)"
        set CMD "${PLUGINS_DIR}/BRAINSFit "

        if { $fixedVolumeNode == "" || [$fixedVolumeNode GetImageData] == "" } {
            PrintError "AlignInputImages: fixed volume node not correctly defined"
            return ""
        }

        set tmpFileName [WriteDataToTemporaryDir $fixedVolumeNode Volume]
        set RemoveFiles "$tmpFileName"

        if { $tmpFileName == "" } {
            return ""
        }
        set CMD "$CMD --fixedVolume $tmpFileName"

        if { $movingVolumeNode == "" || [$movingVolumeNode GetImageData] == "" } {
            PrintError "AlignInputImages: moving volume node not correctly defined"
            return ""
        }

        set tmpFileName [WriteDataToTemporaryDir $movingVolumeNode Volume]
        set RemoveFiles "$RemoveFiles $tmpFileName"

        if { $tmpFileName == "" } { return 1 }
        set CMD "$CMD --movingVolume $tmpFileName"

        #  still define this
        if { $outVolumeNode == "" } {
            PrintError "AlignInputImages: output volume node not correctly defined"
            return ""
        }
        set outVolumeFileName [CreateTemporaryFileName $outVolumeNode]

        if { $outVolumeFileName == "" } {
            return ""
        }
        set CMD "$CMD --outputVolume $outVolumeFileName"

        set RemoveFiles "$RemoveFiles $outVolumeFileName"

        # Do no worry about fileExtensions=".mat" type="linear" reference="movingVolume"
        # these are set in vtkCommandLineModuleLogic.cxx automatically
        if { [lsearch $RegistrationType "BSpline"] > -1 } {
            set transformNode [vtkMRMLBSplineTransformNode New]
            $transformNode SetName "EMSegmentBSplineTransform"
            $SCENE AddNode $transformNode
            set transID [$transformNode GetID]
            set outTransformFileName [CreateTemporaryFileName $transformNode]
            $transformNode Delete
            set CMD "$CMD --bsplineTransform $outTransformFileName --maxBSplineDisplacement 10.0"
        } else {
            set transformNode [vtkMRMLLinearTransformNode New]
            $transformNode SetName "EMSegmentLinearTransform"
            $SCENE AddNode $transformNode
            set transID [$transformNode GetID]
            set outTransformFileName [CreateTemporaryFileName $transformNode]

            $transformNode Delete
            set CMD "$CMD --outputTransform $outTransformFileName"
        }
        set RemoveFiles "$RemoveFiles $outTransformFileName"

        # -- still define this End

        # Write Parameters
        set fixedVolume [$fixedVolumeNode GetImageData]
        set scalarType [$fixedVolume GetScalarTypeAsString]
        switch -exact "$scalarType" {
            "bit" { set CMD "$CMD --outputVolumePixelType binary" }
            "unsigned char" { set CMD "$CMD --outputVolumePixelType uchar" }
            "unsigned short" { set CMD "$CMD --outputVolumePixelType ushort" }
            "unsigned int" { set CMD "$CMD --outputVolumePixelType uint" }
            "short" -
            "int" -
            "float" { set CMD "$CMD --outputVolumePixelType $scalarType" }
            default {
                PrintError "BRAINSRegistration: cannot resample a volume of type $scalarType"
                return ""
            }
        }

        # Filter options - just set it here to make sure that if default values are changed this still works as it supposed to
        set CMD "$CMD --backgroundFillValue $backgroundLevel"
        set CMD "$CMD --interpolationMode Linear"
        set CMD "$CMD --maskProcessingMode  ROIAUTO --ROIAutoDilateSize 3.0 --maskInferiorCutOffFromCenter 65.0 --initializeTransformMode useCenterOfHeadAlign"

        # might be still wrong
        foreach TYPE $RegistrationType {
            set CMD "$CMD --use${TYPE}"
        }

        if {$fastFlag} {
            set CMD "$CMD --numberOfSamples 10000"
        } else {
            set CMD "$CMD --numberOfSamples 100000"
        }

        set CMD "$CMD --numberOfIterations 1500 --minimumStepLength 0.005 --translationScale 1000.0 --reproportionScale 1.0 --skewScale 1.0 --splineGridSize 28,20,24 --fixedVolumeTimeIndex 0 --movingVolumeTimeIndex 0 --medianFilterSize 0,0,0 --numberOfHistogramBins 50 --numberOfMatchPoints 10 --useCachingOfBSplineWeightsMode ON --useExplicitPDFDerivativesMode AUTO --relaxationFactor 0.5 --maximumStepLength 0.2 --failureExitCode -1 --debugNumberOfThreads -1 --debugLevel 0 --costFunctionConvergenceFactor 1e+9 --projectedGradientTolerance 1e-5 --costMetric MMI"

        $LOGIC PrintText "TCL: Executing $CMD"
        catch { eval exec $CMD } errmsg
        $LOGIC PrintText "TCL: $errmsg"


        # Read results back to scene
        # $::slicer3::ApplicationLogic RequestReadData [$outVolumeNode GetID] $outVolumeFileName 0 1
        # Cannot do it that way bc vtkSlicerApplicationLogic needs a cachemanager,
        # which is defined through vtkSlicerCacheAndDataIOManagerGUI.cxx
        # instead:

        # Test:
        # ReadDataFromDisk $outVolumeNode /home/pohl/Slicer3pohl/463_vtkMRMLScalarVolumeNode17.nrrd Volume
        if { [ReadDataFromDisk $outVolumeNode $outVolumeFileName Volume] == 0 } {
            set nodeID [$SCENE GetNodeByID $transID]
            if { $nodeID != "" } {
                $SCENE RemoveNode $nodeID
            }
        }

        # Test:
        # ReadDataFromDisk [$SCENE GetNodeByID $transID] /home/pohl/Slicer3pohl/EMSegmentLinearTransform.mat Transform
        if { [ReadDataFromDisk [$SCENE GetNodeByID $transID] $outTransformFileName Transform] == 0 } {
            set nodeID [$SCENE GetNodeByID $transID]
            if { $nodeID != "" } {
                $SCENE RemoveNode $nodeID
            }
        }

        # Test: 
        # $LOGIC PrintText "==> [[$SCENE GetNodeByID $transID] Print]"

        foreach NAME $RemoveFiles {
            file delete -force $NAME
        }

        # Remove Transformation from image
        $movingVolumeNode SetAndObserveTransformNodeID ""
        $SCENE Edited

        # return ID or ""
        return [$SCENE GetNodeByID $transID]
    }

    proc CMTKRegistration { fixedVolumeNode movingVolumeNode outVolumeNode backgroundLevel bSplineFlag fastFlag} {
        variable SCENE
        variable LOGIC
        variable GUI
        $LOGIC PrintText "TCL: =========================================="
        $LOGIC PrintText "TCL: == Image Alignment CommandLine: $bSplineFlag "
        $LOGIC PrintText "TCL: =========================================="

        ## check arguments

        if { $fixedVolumeNode == "" || [$fixedVolumeNode GetImageData] == "" } {
            PrintError "CMTKRegistration: fixed volume node not correctly defined"
            return ""
        }

        if { $movingVolumeNode == "" || [$movingVolumeNode GetImageData] == "" } {
            PrintError "CMTKRegistration: moving volume node not correctly defined"
            return ""
        }

        if { $outVolumeNode == "" } {
            PrintError "CMTKRegistration: output volume node not correctly defined"
            return ""
        }

        set fixedVolumeFileName [WriteDataToTemporaryDir $fixedVolumeNode Volume]
        if { $fixedVolumeFileName == "" } {
            # remove files
            return ""
        }
        set RemoveFiles "$fixedVolumeFileName"


        set movingVolumeFileName [WriteDataToTemporaryDir $movingVolumeNode Volume]
        if { $movingVolumeFileName == "" } {
            #remove files
            return ""
        }
        set RemoveFiles "$RemoveFiles $movingVolumeFileName"


        set outVolumeFileName [CreateTemporaryFileName $outVolumeNode]
        if { $outVolumeFileName == "" } {
            #remove files
            return ""
        }
        set RemoveFiles "$RemoveFiles $outVolumeFileName"


        ## CMTK specific arguments

        set CMD "[$::slicer3::Application GetExtensionsInstallPath]"
        set svnrevision [$::slicer3::Application GetSvnRevision]
        if { $svnrevision == "" } {
            set CMD "$CMD/15383"
        } else {
            set CMD "$CMD/$svnrevision"
        }
        set CMD "$CMD/CMTK4Slicer/registration "

        set CMD "$CMD --verbose --initxlate --exploration 8.0 --dofs 6 --dofs 9"

        if {$fastFlag} {
            set CMD "$CMD --accuracy 0.5"
        } else {
            set CMD "$CMD --accuracy 0.1"
        }

#        # Write Parameters
#        set fixedVolume [$fixedVolumeNode GetImageData]
#        set scalarType [$fixedVolume GetScalarTypeAsString]
#        switch -exact "$scalarType" {
#            "bit" { set CMD "$CMD --outputVolumePixelType binary" }
#            "unsigned char" { set CMD "$CMD --outputVolumePixelType uchar" }
#            "unsigned short" { set CMD "$CMD --outputVolumePixelType ushort" }
#            "unsigned int" { set CMD "$CMD --outputVolumePixelType uint" }
#            "short" -
#            "int" -
#            "float" { set CMD "$CMD --outputVolumePixelType $scalarType" }
#            default {
#                PrintError "CMTKRegistration: cannot resample a volume of type $scalarType"
#                return ""
#            }
#        }

        set outLinearTransformDirName /tmp/affine.xform
        set outTransformDirName $outLinearTransformDirName

        set CMD "$CMD -o $outLinearTransformDirName"
        set CMD "$CMD --write-reformatted $outVolumeFileName"
        set CMD "$CMD $fixedVolumeFileName"
        set CMD "$CMD $movingVolumeFileName"


        ## execute affine registration

        $LOGIC PrintText "TCL: Executing $CMD"
        catch { eval exec $CMD } errmsg
        $LOGIC PrintText "TCL: $errmsg"

        if { $bSplineFlag } {

            set CMD "[$::slicer3::Application GetExtensionsInstallPath]"
            set svnrevision [$::slicer3::Application GetSvnRevision]
            if { $svnrevision == "" } {
                set CMD "$CMD/15383"
            } else {
                set CMD "$CMD/$svnrevision"
            }
            set CMD "$CMD/CMTK4Slicer/warp"

            set outNonLinearTransformDirName /tmp/bspline.xform
            set outTransformDirName $outNonLinearTransformDirName

            set CMD "$CMD --delay-refine --grid-spacing 40 --refine 4"
            set CMD "$CMD --exact-spacing --energy-weight 5e-2"
            set CMD "$CMD --exploration 16 --accuracy 0.1 --coarsest 1.5"

            set CMD "$CMD --initial $outLinearTransformDirName"
            set CMD "$CMD -o $outNonLinearTransformDirName"
            set CMD "$CMD --write-reformatted $outVolumeFileName"
            set CMD "$CMD $fixedVolumeFileName"
            set CMD "$CMD $movingVolumeFileName"

            ## execute bspline registration

            $LOGIC PrintText "TCL: Executing $CMD"
            catch { eval exec $CMD } errmsg
            $LOGIC PrintText "TCL: $errmsg"
        }

        ## Read results back to scene

        # Test:
        # ReadDataFromDisk $outVolumeNode /home/pohl/Slicer3pohl/463_vtkMRMLScalarVolumeNode17.nrrd Volume
        if { [ReadDataFromDisk $outVolumeNode $outVolumeFileName Volume] == 0 } {
            if { [file exists $outVolumeDirName] == 0 } {
                set outTransformDirName ""
            }
        }

        if { [file exists $outTransformDirName] == 0 } {
            set outTransformDirName ""
        }

        # Test: 
        # $LOGIC PrintText "==> [[$SCENE GetNodeByID $transID] Print]"

        foreach NAME $RemoveFiles {
            file delete -force $NAME
        }

        # Remove Transformation from image
        $movingVolumeNode SetAndObserveTransformNodeID ""
        $SCENE Edited

        # return transformation directory name or ""
        puts "outTransformDirName: $outTransformDirName"
        return $outTransformDirName
    }

    proc CheckAndCorrectClassCovarianceMatrix {parentNodeID } {
    variable mrmlManager
        variable LOGIC
    set n [$mrmlManager GetTreeNodeNumberOfChildren $parentNodeID ]
    set failedList ""
    for {set i 0 } { $i < $n  } { incr i } {
        set id [ $mrmlManager GetTreeNodeChildNodeID $parentNodeID $i ] 
        if { [ $mrmlManager GetTreeNodeIsLeaf $id ] } {
        if { [$mrmlManager IsTreeNodeDistributionLogCovarianceWithCorrectionInvertableAndSemiDefinite $id ] == 0 } {
            # set the off diagonal to zeo 
            $LOGIC PrintText "TCL:CheckAndCorrectClassCovarianceMatrix: Set off diagonal of the LogCovariance of [ $mrmlManager GetTreeNodeName $id] to zero - otherwise matrix not convertable and semidefinite"
            $mrmlManager SetTreeNodeDistributionLogCovarianceOffDiagonal $id  0
            # if it still fails then add to list 
            if { [$mrmlManager IsTreeNodeDistributionLogCovarianceWithCorrectionInvertableAndSemiDefinite $id ] == 0 } {
            set failedList "${failedList}$id "
            }
        }
        } else {
        set failedList "${failedList}[CheckAndCorrectClassCovarianceMatrix $id]"
        }
    }
    return "$failedList"
    }
   

    proc CheckAndCorrectTreeCovarianceMatrix { } {
    variable mrmlManager
    set rootID [$mrmlManager GetTreeRootNodeID]
    return "[CheckAndCorrectClassCovarianceMatrix $rootID]"
    }

    proc Progress {args} {
        variable LOGIC
        $LOGIC PrintTextNoNewLine "."
    }

    proc wget { url  fileName } {
        package require http
        variable LOGIC
        $LOGIC  PrintTextNoNewLine "Loading $url "
    if { [ catch { set r [http::geturl $url -binary 1 -progress ::EMSegmenterPreProcessingTcl::Progress ] } errmsg ] }  {
        $LOGIC  PrintText " " 
            PrintError "Could not download $url: $errmsg"
        return 1
    }

        set fo [open $fileName w]
        fconfigure $fo -translation binary
        puts -nonewline $fo [http::data $r]
        close $fo
        $LOGIC PrintText "\nSaving to $fileName\n"
        ::http::cleanup $r

    return 0
    }

    # returns 1 if error occured
    proc DownloadFileIfNeededAndSetURIToNULL { node origMRMLFileName forceFlag } {
    variable GUI
    variable LOGIC
        variable SCENE

    if { [$node GetClassName] != "vtkMRMLVolumeArchetypeStorageNode" } {
        PrintError "DownloadFileIfNeededAndSetURIToNULL: Wrong node type" 
        return 1 
    } 
       
    # ONLY WORKS FOR AB
    set URI "[$node GetURI ]"
    $node SetURI ""

        if {$forceFlag == 0 } {
        set oldFileName [$node GetFileName]
        if { "$oldFileName" != "" } {
        # Turn it into absolute file if it is not already
        if { "[ file pathtype oldFileName ]" != "absolute" } {
            set oldFileName "[file dirname $origMRMLFileName ]$oldFileName" 
        }

            if { [file exists $oldFileName ] && [file isfile $oldFileName ] } {
                # Must set it again bc path of scene might have changed so set it to absolute first 
                $node SetFileName $oldFileName
                return 0
            }
        }
    }
       
    if {$URI == ""} {
            PrintError "DownloadFileIfNeededAndSetURIToNULL: File does not exist and URI is NULL" 
        return 1
    }

        # Need to download file to temp directory 
        set fileName [$GUI GetTemporaryDirectory]/[expr int(rand()*10000)]_[file tail $URI]

        if { [wget $URI $fileName] } {
        return 1
    }


    $node SetFileName $fileName
    return 0 
    }

    proc ReplaceInSceneURINameWithFileName { mrmlFileName } {
       variable GUI
       variable LOGIC
       variable SCENE

       # Important so that it will write out all the nodes we are interested 
       set mrmlScene [vtkMRMLScene New]
       set num [$SCENE GetNumberOfRegisteredNodeClasses]
       for { set i 0 } { $i < $num } { incr i } {
       set node [$SCENE GetNthRegisteredNodeClass $i]
       if { ($node != "" ) } {
          set name [$node GetClassName ]
          if {[ string first "vtkMRMLFiniteElement" $name  ] < 0 } {
           $mrmlScene RegisterNodeClass $node
           }
       }
    }
    
       set parser [vtkMRMLParser New ] 
       $parser SetMRMLScene $mrmlScene
       $parser SetFileName $mrmlFileName
  
       if { [$parser Parse] } {
       set errorFlag 0
       } else {
       set errorFlag 1
       }
       $parser Delete

       if {$errorFlag == 0 } { 
       # Download all the files if needed 
       set TYPE "vtkMRMLVolumeArchetypeStorageNode"
       set n [$mrmlScene GetNumberOfNodesByClass $TYPE]

       for { set i 0 } { $i < $n } { incr i } {
           if { [DownloadFileIfNeededAndSetURIToNULL [$mrmlScene GetNthNodeByClass $i $TYPE] $mrmlFileName 0 ] } {
           set errorFlag 1
           }
       }

       if { $errorFlag == 0 } {
            # Set the new path of mrmlScene - by first setting scene to old path so that the function afterwards cen extract the file name  
                $mrmlScene SetURL $mrmlFileName
                set tmpFileName [CreateTemporaryFileName  $mrmlScene]
                $mrmlScene SetURL $tmpFileName
                $mrmlScene SetRootDirectory [file dirname $tmpFileName ] 
            $mrmlScene Commit $tmpFileName
       }
       } 
       $mrmlScene Delete

       if { $errorFlag } { 
       return "" 
       }        

       $LOGIC PrintText "TCL: Wrote modified $mrmlFileName to $tmpFileName"

       return $tmpFileName
    }
}

namespace eval EMSegmenterSimpleTcl {

    variable inputChannelGUI
    variable mrmlManager

    proc InitVariables { {GUI ""} } {
        variable inputChannelGUI
        variable mrmlManager
        if {$GUI == "" } {
            set GUI [$::slicer3::Application GetModuleGUIByName EMSegmenter]
        }
        if { $GUI == "" } {
            PrintError "InitVariables: GUI not defined"
            return 1
        }
        set mrmlManager [$GUI GetMRMLManager]
        if { $mrmlManager == "" } {
            PrintError "InitVariables: mrmManager not defined"
            return 1
        }
        set inputChannelGUI [$GUI GetInputChannelStep]
        if { $inputChannelGUI == "" } {
            PrintError "InitVariables: InputChannelStep not defined"
            return 1
        }
        return 0
    }

    proc PrintError { TEXT } {
         puts stderr "TCL: ERROR:EMSegmenterSimpleTcl::${TEXT}"
    }

    # 0 = Do not create a check list for the simple user interface
    # simply remove
    # 1 = Create one - then also define ShowCheckList and
    #     ValidateCheckList where results of checklist are transfered to Preprocessing
    proc CreateCheckList { } { return 0 }
    proc ShowCheckList { } { return 0}
    proc ValidateCheckList { } { return 0 }
}
