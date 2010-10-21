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
# Remember to source first GenericTask.tcl as it has all the variables/basic structure defined
#
namespace eval EMSegmenterPreProcessingTcl {

    #
    # Variables Specific to this Preprocessing
    #
    variable TextLabelSize 1
    variable CheckButtonSize 2
    variable VolumeMenuButtonSize 0
    variable TextEntrySize 0

    # Check Button
    variable atlasAlignedFlagID 0
    variable inhomogeneityCorrectionFlagID 1

    # Text Entry
    # not defined for this task

    #
    # OVERWRITE DEFAULT
    #

    # -------------------------------------
    # Define GUI
    # return 1 when error occurs
    # -------------------------------------
    proc ShowUserInterface { } {
        variable preGUI
        variable atlasAlignedFlagID
        variable iccMaskSelectID
        variable inhomogeneityCorrectionFlagID
        variable LOGIC

        # Always has to be done initially so that variables are correctly defined
        if { [InitVariables] } {
            puts stderr "ERROR: MRI-HumanBrain: ShowUserInterface: Not all variables are correctly defined!"
            return 1
        }
        $LOGIC PrintText  "TCLMRI: Preprocessing MRI Human Brain - ShowUserInterface"

        $preGUI DefineTextLabel "This task only applies to non-skull stripped scans! \n\nShould the EMSegmenter " 0
        $preGUI DefineCheckButton "- register the atlas to the input scan ?" 0 $atlasAlignedFlagID
        # $preGUI DefineCheckButton "Are the input scans skull stripped ?" 0 $skullStrippedFlagID
        # $preGUI DefineVolumeMenuButton "Define ICC mask of the atlas ?" 0 $iccMaskSelectID
        $preGUI DefineCheckButton "- perform image inhomogeneity correction on input scan ?" 0 $inhomogeneityCorrectionFlagID

        # Define this at the end of the function so that values are set by corresponding MRML node
        $preGUI SetButtonsFromMRML
    }

    # -------------------------------------
    # Define Preprocessing Pipeline
    # return 1 when error occurs
    # -------------------------------------
    proc Run { } {
        variable preGUI
        variable workingDN
        variable subjectNode
        variable inputAtlasNode
        variable mrmlManager
        variable LOGIC

        variable atlasAlignedFlagID
        variable iccMaskSelectID
        variable inhomogeneityCorrectionFlagID

         $LOGIC PrintText "TCLMRI: =========================================="
         $LOGIC PrintText "TCLMRI: == Preprocress Data"
         $LOGIC PrintText "TCLMRI: =========================================="
        # ---------------------------------------
        # Step 1 : Initialize/Check Input
        if {[InitPreProcessing]} { 
            return 1
        }

        # ----------------------------------------------------------------------------
        # We have to create this function so that we can run it in command line mode
        #
        set atlasAlignedFlag [ GetCheckButtonValueFromMRML $atlasAlignedFlagID ]
        set skullStrippedFlag 0
        set iccMaskVTKID 0
        # [GetVolumeMenuButtonValueFromMRML $iccMaskSelectID]
        set inhomogeneityCorrectionFlag [GetCheckButtonValueFromMRML $inhomogeneityCorrectionFlagID]

         $LOGIC PrintText "TCLMRI: ==> Preprocessing Setting: $atlasAlignedFlag $inhomogeneityCorrectionFlag"

        if { ($atlasAlignedFlag == 0) && ($skullStrippedFlag == 1) } {
            PrintError "Run: We currently cannot align the atlas to skull stripped image"
            return 1
        }

        if { $iccMaskVTKID } {
            set inputAtlasICCMaskNode [$mrmlManager GetVolumeNode $iccMaskVTKID]
            if { $inputAtlasICCMaskNode == "" } {
                PrintError "Run: inputAtlasICCMaskNode is not defined"
                return 1
            }
        } else {
            set inputAtlasICCMaskNode ""
        }

        # -------------------------------------
        # Step 2: Generate ICC Mask Of input images
        if { $inputAtlasICCMaskNode != "" && 0} {
            set inputAtlasVolumeNode [$inputAtlas GetNthVolumeNode 0]
            set subjectVolumeNode [$subjectNode GetNthVolumeNode 0]

            set subjectICCMaskNode [GenerateICCMask $inputAtlasVolumeNode $inputAtlasICCMaskNode $subjectVolumeNode]

            if { $subjectICCMaskNode == "" } {
                PrintError "Run: Generating ICC mask for Input failed!"
                return 1
            }
        } else {
            #  $LOGIC PrintText "TCLMRI: Skipping ICC Mask generation! - Not yet implemented"
            set subjectICCMaskNode ""
        }

        # -------------------------------------
        # Step 4: Perform Intensity Correction
        if { $inhomogeneityCorrectionFlag == 1 } {

            set subjectIntensityCorrectedNodeList [PerformIntensityCorrection $subjectICCMaskNode]
            if { $subjectIntensityCorrectedNodeList == "" } {
                PrintError "Run: Intensity Correction failed !"
                return 1
            }
            if { [UpdateSubjectNode "$subjectIntensityCorrectedNodeList"] } {
                return 1
            }
        } else {
             $LOGIC PrintText "TCLMRI: Skipping intensity correction"
        }

        # write results over to subjectNode

        # -------------------------------------
        # Step 5: Atlas Alignment - you will also have to include the masks
        # Defines $workingDN GetAlignedAtlasNode
        if { [RegisterAtlas $atlasAlignedFlag] } {
            PrintError "Run: Atlas alignment failed !"
            return 1
        }


        # -------------------------------------
        # Step 6: Perform autosampling to define intensity distribution
        if { [ComputeIntensityDistributions] } {
            PrintError "Run: Could not automatically compute intensity distribution !"
            return 1
        }

        # -------------------------------------
        # Step 7: Check validity of Distributions 
        set failedIDList [CheckAndCorrectTreeCovarianceMatrix]
        if { $failedIDList != "" } {
        set MSG "Log Covariance matrices for the following classes seemed incorrect:\n "
        foreach ID $failedIDList {
        set MSG "${MSG}[$mrmlManager GetTreeNodeName $ID]\n"
        }
        set MSG "${MSG}This can cause failure of the automatic segmentation. To address the issue, please visit the web site listed under Help"
        $preGUI PopUpWarningWindow "$MSG"
    }

        return 0
    }

    #
    # TASK SPECIFIC FUNCTIONS
    #

    # -------------------------------------
    # Generate ICC Mask for input image
    # if succesfull returns ICC Mask Node
    # otherwise returns nothing
    # -------------------------------------
    proc GenerateICCMask { inputAtlasVolumeNode inputAtlasICCMaskNode subjectVolumeNode } {
        variable LOGIC
         $LOGIC PrintText "TCLMRI: =========================================="
         $LOGIC PrintText "TCLMRI: == Generate ICC MASK (not yet implemented)"
         $LOGIC PrintText "TCLMRI: =========================================="
        set EXE_DIR "$::env(Slicer3_HOME)/bin"
        set PLUGINS_DIR "$::env(Slicer3_HOME)/lib/Slicer3/Plugins"

        # set CMD "$PLUGINS_DIR/DemonsRegistration --fixed_image $Scan2Image --moving_image $Scan1Image --output_image $Scan1ToScan2Image --output_field $Scan1ToScan2Deformation --num_levels 3 --num_iterations 20,20,20 --def_field_sigma 1 --use_histogram_matching --verbose"

        set CMD "$PLUGINS_DIR/DemonsRegistration --fixed_image $Scan2Image --moving_image $Scan1Image --output_image $Scan1ToScan2Image --output_field $Scan1ToScan2Deformation --num_levels 3 --num_iterations 20,20,20 --def_field_sigma 1 --use_histogram_matching --verbose"

        return 1
    }

    # -------------------------------------
    # Perform intensity correction
    # if succesfull returns a list of intensity corrected subject volume nodes
    # otherwise returns nothing
    #     ./Slicer3 --launch N4ITKBiasFieldCorrection --inputimage ../Slicer3/Testing/Data/Input/MRMeningioma0.nrrd --maskimage /projects/birn/fedorov/Meningioma_anonymized/Cases/Case02/Case02_Scan1ICC.nrrd corrected_image.nrrd recovered_bias_field.nrrd
    # -------------------------------------
    proc PerformIntensityCorrection { subjectICCMaskNode } {
        variable LOGIC
        variable subjectNode
        variable SCENE
         $LOGIC PrintText "TCLMRI: =========================================="
         $LOGIC PrintText "TCLMRI: == Intensity Correction "
         $LOGIC PrintText "TCLMRI: =========================================="
        set n4Module ""
        foreach gui [vtkCommandLineModuleGUI ListInstances] {
            if { [$gui GetGUIName] == "N4ITK MRI Bias correction" } {
                set n4Module $gui
            }
        }
        # not in gui mode
        if { $n4Module == "" } {
            return [N4ITKBiasFieldCorrectionCLI $subjectNode $subjectICCMaskNode]
        }

        # in gui mode
        $n4Module Enter

        # create a new node and add information to this node
        set n4Node [$::slicer3::MRMLScene CreateNodeByClass vtkMRMLCommandLineModuleNode]
        $::slicer3::MRMLScene AddNode $n4Node
        $n4Node SetModuleDescription "N4ITK MRI Bias correction"
        $n4Module SetCommandLineModuleNode $n4Node
        [$n4Module GetLogic] SetCommandLineModuleNode $n4Node
        if { $subjectICCMaskNode != "" } {
            $n4Node SetParameterAsString "maskImageName" [$subjectICCMaskNode GetID]
        } else {
            $n4Node SetParameterAsString "maskImageName" ""
        }

        # initialize
        set result ""

        # Run the algorithm on each subject image
        for { set i 0 } {$i < [$subjectNode GetNumberOfVolumes] } { incr i } {
            # Define input
            set inputNode [$subjectNode GetNthVolumeNode $i]
            if { $inputNode == "" } {
                PrintError "PerformIntensityCorrection: the ${i}th subject node is not defined!"
                foreach NODE $result { DeleteNode $NODE }
                return ""
            }

            set inputVolume [$inputNode GetImageData]
            if { $inputVolume == "" } {
                PrintError "PerformIntensityCorrection: the ${i}th subject node has not input data defined!"
                foreach NODE $result { DeleteNode $NODE }
                return ""
            }

            # Define output
            set outputVolume [vtkImageData New]
            set outputNode [CreateVolumeNode $inputNode "[$inputNode GetName]_N4corrected"]
            $outputNode SetAndObserveImageData $outputVolume
            $outputVolume Delete

            # Define parameters
            $n4Node SetParameterAsString "inputImageName" [$inputNode GetID]
            $n4Node SetParameterAsString "outputImageName" [$outputNode GetID]
            # $n4Node SetParameterAsString "outputBiasFieldName" [$outputBiasVolume GetID]

            # run algorithm
            [$n4Module GetLogic] LazyEvaluateModuleTarget $n4Node
            [$n4Module GetLogic] ApplyAndWait $n4Node

            # Make sure that input and output are of the same type !
            set outputVolume [$outputNode GetImageData]
            if {[$inputVolume GetScalarType] != [$outputVolume GetScalarType] } {
                set cast [vtkImageCast New]
                $cast SetInput $outputVolume
                $cast SetOutputScalarType [$inputVolume GetScalarType]
                $cast Update
                $outputVolume DeepCopy [$cast GetOutput]
                $cast Delete
            }

            # still in for loop, create a list of outputNodes
            set result "${result}$outputNode "
        }

        # delete command line node from mrml scene
        $::slicer3::MRMLScene RemoveNode $n4Node

        DeleteCommandLine $n4Node
        $n4Module Exit

        return "$result"
    }


    proc N4ITKBiasFieldCorrectionCLI { subjectNode subjectICCMaskNode } {
        variable SCENE
        variable LOGIC
         $LOGIC PrintText "TCLMRI: =========================================="
         $LOGIC PrintText "TCLMRI: ==     N4ITKBiasFieldCorrectionCLI      =="
         $LOGIC PrintText "TCLMRI: =========================================="

        set PLUGINS_DIR "$::env(Slicer3_HOME)/lib/Slicer3/Plugins"
        set CMD "${PLUGINS_DIR}/N4ITKBiasFieldCorrection "

        # initialize
        set result ""

        # Run the algorithm on each subject image
        for { set i 0 } {$i < [$subjectNode GetNumberOfVolumes] } { incr i } {

            set inputNode [$subjectNode GetNthVolumeNode $i]
            set inputVolume [$inputNode GetImageData]
            if { $inputVolume == "" } {
                PrintError "PerformIntensityCorrection: the ${i}th subject node has not input data defined!"
                foreach NODE $result {
                    DeleteNode $NODE
                }
                return ""
            }

            set tmpFileName [WriteDataToTemporaryDir $inputNode Volume ]
            set RemoveFiles "\"$tmpFileName\""
            if { $tmpFileName == "" } {
                return 1
            }
            set CMD "$CMD --inputimage $tmpFileName"

            # set tmpFileName [WriteDataToTemporaryDir $subjectICCMaskNode Volume ]
            # set RemoveFiles "$RemoveFiles \"$tmpFileName\""
            # if { $tmpFileName == "" } { 
            # return 1
            #     }
            # set CMD "$CMD --maskimag $tmpFileName"

            # create a new node for our output-list 
            set outVolumeNode [CreateVolumeNode $inputNode "[$inputNode GetName]_N4corrected"]
            set outputVolume [vtkImageData New]
            $outVolumeNode SetAndObserveImageData $outputVolume
            $outputVolume Delete

            set outVolumeFileName [ CreateTemporaryFileName $outVolumeNode ]
             $LOGIC PrintText "$outVolumeFileName"
            if { $outVolumeFileName == "" } {
                return 1
            }
            set CMD "$CMD --outputimage \"$outVolumeFileName\""
            set RemoveFiles "$RemoveFiles \"$outVolumeFileName\""

            # for test purposes(reduces execution time)
            # set CMD "$CMD --iterations \"3,2,1\""

            # set outbiasVolumeFileName [ CreateTemporaryFileName $outbiasVolumeFileName ]
            # if { $outbiasVolumeFileName == "" } {
            #     return 1
            # }
            # set CMD "$CMD --outputbiasfield $outbiasVolumeFileName"

            # execute algorithm
             $LOGIC PrintText "TCLMRI: Executing $CMD"
            catch { eval exec $CMD } errmsg
             $LOGIC PrintText "TCLMRI: $errmsg"

            # Read results back, we have to read 2 results

            ReadDataFromDisk $outVolumeNode $outVolumeFileName Volume
            file delete -force $outVolumeFileName

            # ReadDataFromDisk $outbiasVolumeNode $outbiasVolumeFileName Volume  
            # file delete -force $outbiasVolumeFileName

            # still in for loop, create a list of Volumes
            set result "${result}$outVolumeNode "
             $LOGIC PrintText "TCLMRI: List of volume nodes: $result"
        }
        # return a newSubjectVolumeNodeList
        return "$result"
    }




    # -------------------------------------
    # Compute intensity distribution through auto sampling
    # if succesfull returns 0
    # otherwise returns 1
    # -------------------------------------
    proc ComputeIntensityDistributions { } {
        variable LOGIC
        variable GUI
        variable mrmlManager
         $LOGIC PrintText "TCLMRI: =========================================="
         $LOGIC PrintText "TCLMRI: == Update Intensity Distribution "
         $LOGIC PrintText "TCLMRI: =========================================="

        # return [$mrmlManager ComputeIntensityDistributionsFromSpatialPrior [$LOGIC GetModuleShareDirectory] [$preGUI GetApplication]]
        if { [$LOGIC ComputeIntensityDistributionsFromSpatialPrior $GUI] } {
            return 1
        }
        return 0
    }

    # -------------------------------------
    # Register Atlas to Subject
    # if succesfull returns 0
    # otherwise returns 1
    # -------------------------------------
    proc RegisterAtlas { alignFlag } {
        variable workingDN
        variable mrmlManager
        variable LOGIC
        variable subjectNode
        variable inputAtlasNode
        variable outputAtlasNode

        set affineFlag [expr ([$mrmlManager GetRegistrationAffineType] != [$mrmlManager GetRegistrationTypeFromString AtlasToTargetAffineRegistrationOff])]
        set bSplineFlag [expr ([$mrmlManager GetRegistrationDeformableType] != [$mrmlManager GetRegistrationTypeFromString AtlasToTargetDeformableRegistrationOff])]

        if {($alignFlag == 0) || (( $affineFlag == 0 ) && ( $bSplineFlag == 0 )) } {
            return [SkipAtlasRegistration]
        }

         $LOGIC PrintText "TCLMRI: =========================================="
         $LOGIC PrintText "TCLMRI: == Register Atlas ($affineFlag / $bSplineFlag) "
         $LOGIC PrintText "TCLMRI: =========================================="


        # ----------------------------------------------------------------
        # Setup
        # ----------------------------------------------------------------
        if { $outputAtlasNode == "" } {
             $LOGIC PrintText "TCLMRI: Aligned Atlas was empty"
            #  $LOGIC PrintText "TCLMRI: set outputAtlasNode \[ $mrmlManager CloneAtlasNode $inputAtlasNode \"AlignedAtlas\"\] "
            set outputAtlasNode [ $mrmlManager CloneAtlasNode $inputAtlasNode "Aligned"]
            $workingDN SetAlignedAtlasNodeID [$outputAtlasNode GetID]
        } else {
             $LOGIC PrintText "TCLMRI: Atlas was just synchronized"
            $mrmlManager SynchronizeAtlasNode $inputAtlasNode $outputAtlasNode "Aligned"
        }

        set fixedTargetChannel 0
        set fixedTargetVolumeNode [$subjectNode GetNthVolumeNode $fixedTargetChannel]
        if { [$fixedTargetVolumeNode GetImageData] == "" } {
            PrintError "RegisterAtlas: Fixed image is null, skipping registration"
            return 1;
        }

        set atlasRegistrationVolumeIndex -1;
        if {[[$mrmlManager GetGlobalParametersNode] GetRegistrationAtlasVolumeKey] != "" } {
            set atlasRegistrationVolumeKey [[$mrmlManager GetGlobalParametersNode] GetRegistrationAtlasVolumeKey]
            set atlasRegistrationVolumeIndex [$inputAtlasNode GetIndexByKey $atlasRegistrationVolumeKey]
        }

        if {$atlasRegistrationVolumeIndex < 0 } {
            PrintError "RegisterAtlas: Attempt to register atlas image but no atlas image selected!"
            return 1
        }

        set movingAtlasVolumeNode [$inputAtlasNode GetNthVolumeNode $atlasRegistrationVolumeIndex]
        set movingAtlasImageData [$movingAtlasVolumeNode GetImageData]

        set outputAtlasVolumeNode [$outputAtlasNode GetNthVolumeNode $atlasRegistrationVolumeIndex]
        set outAtlasImageData [$outputAtlasVolumeNode GetImageData]

        if { $movingAtlasImageData == "" } {
            PrintError "RegisterAtlas: Moving image is null, skipping"
            return 1
        }

        if {$outAtlasImageData == "" } {
            PrintError "RegisterAtlas: Registration output is null, skipping"
            return 1
        }

        set affineType [ $mrmlManager GetRegistrationAffineType ]
        set deformableType [ $mrmlManager GetRegistrationDeformableType ]
        set interpolationType [ $mrmlManager GetRegistrationInterpolationType ]

        set fixedRASToMovingRASTransformAffine [ vtkTransform New]
        set fixedRASToMovingRASTransformDeformable ""

         $LOGIC PrintText "TCLMRI: ========== Info ========="
         $LOGIC PrintText "TCLMRI: = Fixed:   [$fixedTargetVolumeNode GetName] "
         $LOGIC PrintText "TCLMRI: = Moving:  [$movingAtlasVolumeNode GetName] "
         $LOGIC PrintText "TCLMRI: = Affine:  $affineType"
         $LOGIC PrintText "TCLMRI: = BSpline: $deformableType"
         $LOGIC PrintText "TCLMRI: = Interp:  $interpolationType"
         $LOGIC PrintText "TCLMRI: ========================="

        # ----------------------------------------------------------------
        # affine registration
        # ----------------------------------------------------------------
        # old Style
        if { 0 } {
            if { $affineType == [$mrmlManager GetRegistrationTypeFromString AtlasToTargetAffineRegistrationOff] } {
                 $LOGIC PrintText "TCLMRI: Skipping affine registration of atlas image."
            } else {
                 $LOGIC PrintText "TCLMRI: Registering atlas image rigid..."
                $LOGIC SlicerRigidRegister $fixedTargetVolumeNode $movingAtlasVolumeNode "" $fixedRASToMovingRASTransformAffine $affineType $interpolationType 0
                 $LOGIC PrintText "TCLMRI: Atlas-to-target transform (fixedRAS -->> movingRAS): "
                for { set r 0 } { $r < 4 } { incr r } {
                     $LOGIC PrintText -nonewline "    "
                    for { set c 0 } { $c < 4 } { incr c } {
                         $LOGIC PrintText -nonewline "[[$fixedRASToMovingRASTransformAffine GetMatrix] GetElement $r $c]   "
                    }
                     $LOGIC PrintText " "
                }
            }
        }

        # ----------------------------------------------------------------
        # deformable registration
        # ----------------------------------------------------------------

        if { 0 } {
            # old Style
            set OffType [$mrmlManager GetRegistrationTypeFromString AtlasToTargetDeformableRegistrationOff]

             $LOGIC PrintText "TCLMRI: Deformable registration $deformableType Off: $OffType"
            if { $deformableType == $OffType } {
                 $LOGIC PrintText "TCLMRI: Skipping deformable registration of atlas image"
            } else {
                 $LOGIC PrintText "TCLMRI: Registering atlas image B-Spline..."
                set fixedRASToMovingRASTransformDeformable [vtkGridTransform New]
                $fixedRASToMovingRASTransformDeformable SetInterpolationModeToCubic
                $LOGIC SlicerBSplineRegister $fixedTargetVolumeNode $movingAtlasVolumeNode "" $fixedRASToMovingRASTransformDeformable $fixedRASToMovingRASTransformAffine $deformableType $interpolationType 0
            }
        } else {
            # New type
            set registrationType "CenterOfHeadAlign Rigid  ScaleVersor3D ScaleSkewVersor3D Affine"
            set fastFlag 0
            if { $affineFlag } {
                if { $affineType == [$mrmlManager GetRegistrationTypeFromString AtlasToTargetAffineRegistrationRigidMMIFast] } {
                    set fastFlag 1
                } else {
                    set fastFlag 0
                }
            }

            if { $bSplineFlag } {
                set registrationType "${registrationType} BSpline"
                if { $deformableType == [$mrmlManager GetRegistrationTypeFromString AtlasToTargetDeformableRegistrationBSplineMMIFast] } {
                    set fastFlag 1
                } else {
                    set fastFlag 0
                }
            }

            set backgroundLevel [$LOGIC GuessRegistrationBackgroundLevel $movingAtlasVolumeNode]
            set transformNode [BRAINSRegistration $fixedTargetVolumeNode $movingAtlasVolumeNode $outputAtlasVolumeNode $backgroundLevel "$registrationType" $fastFlag]
            if { $transformNode == "" } {
                return 1
            }
        }

        # ----------------------------------------------------------------
        # resample
        # ----------------------------------------------------------------

        for { set i 0 } {$i < [$outputAtlasNode GetNumberOfVolumes] } { incr i } {
            if { $i == $atlasRegistrationVolumeIndex} { continue }
            set movingVolumeNode [$inputAtlasNode GetNthVolumeNode $i]
            set outputVolumeNode [$outputAtlasNode GetNthVolumeNode $i]

            if {[$movingVolumeNode GetImageData] == ""} {
                PrintError "RegisterAtlas: Moving image is null, skipping: $i"
                return 1
            }
            if { [$outputVolumeNode GetImageData] == ""} {
                PrintError "RegisterAtlas: Registration output is null, skipping: $i"
                return 1
            }
             $LOGIC PrintText "TCLMRI: Resampling atlas image $i ..."

            set backgroundLevel [$LOGIC GuessRegistrationBackgroundLevel $movingVolumeNode]
             $LOGIC PrintText "TCLMRI: Guessed background level: $backgroundLevel"

            if { 0 } {
                # resample moving image
                # old style
                if {$fixedRASToMovingRASTransformDeformable != "" } {
                    $LOGIC SlicerImageResliceWithGrid $movingVolumeNode $outputVolumeNode $fixedTargetVolumeNode $fixedRASToMovingRASTransformDeformable $interpolationType $backgroundLevel
                } else {
                    $LOGIC SlicerImageReslice $movingVolumeNode $outputVolumeNode $fixedTargetVolumeNode $fixedRASToMovingRASTransformAffine $interpolationType $backgroundLevel
                }
            } else {
                if { [BRAINSResample $movingVolumeNode $fixedTargetVolumeNode $outputVolumeNode $transformNode $backgroundLevel] } {
                    return 1
                }
            }
        }

        if { 0 } {
            $fixedRASToMovingRASTransformAffine Delete
            if { $fixedRASToMovingRASTransformDeformable != "" } {
                $fixedRASToMovingRASTransformDeformable Delete
            }
        }

         $LOGIC PrintText "TCLMRI: Atlas-to-target registration complete."
        $workingDN SetAlignedAtlasNodeIsValid 1
        return 0
    }
}


namespace eval EMSegmenterSimpleTcl {
    # 0 = Do not create a check list for the simple user interface
    # simply remove
    # 1 = Create one - then also define ShowCheckList and
    #     ValidateCheckList where results of checklist are transfered to Preprocessing

    proc CreateCheckList { } {
        return 1
    }

    proc ShowCheckList { } {
        variable inputChannelGUI
        # Always has to be done initially so that variables are correctly defined
        if { [InitVariables] } {
            PrintError "ShowCheckList: Not all variables are correctly defined!"
            return 1
        }

        $inputChannelGUI DefineTextLabel "Please insure that input scans are not skull stripped" 0
        $inputChannelGUI DefineCheckButton "Perform image inhomogeneity correction on input scans ?" 0 $EMSegmenterPreProcessingTcl::inhomogeneityCorrectionFlagID

        # Define this at the end of the function so that values are set by corresponding MRML node
        $inputChannelGUI SetButtonsFromMRML
        return 0

    }

    proc ValidateCheckList { } {
        return 0
    }
}
