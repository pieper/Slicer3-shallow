from SlicerScriptedModule import ScriptedModuleGUI
import Slicer
from Slicer import slicer

vtkSlicerNodeSelectorWidget_NodeSelectedEvent = 11000
vtkKWPushButton_InvokedEvent = 10000
vtkMRMLTransformableNode_TransformModifiedEvent = 15000

class IGTRecorderGUI(ScriptedModuleGUI):

    def __init__(self):
        ScriptedModuleGUI.__init__(self)
        self.vtkScriptedModuleGUI.SetCategory("IGT")
        self.TransformNodeSelector = slicer.vtkSlicerNodeSelectorWidget()
        self.StartStopButton = slicer.vtkKWPushButton()
        self.ObservedTransform = 0
    
    def Destructor(self):
        pass
    
    def RemoveMRMLNodeObservers(self):
        pass
    
    def RemoveLogicObservers(self):
        pass

    def AddGUIObservers(self):
        self.TransformNodeSelectorTag = self.AddObserverByNumber(self.TransformNodeSelector,vtkSlicerNodeSelectorWidget_NodeSelectedEvent)
        self.StartStopButtonTag = self.AddObserverByNumber(self.StartStopButton,vtkKWPushButton_InvokedEvent)

    def RemoveGUIObservers(self):
        self.RemoveObserver(self.TransformNodeSelectorTag)
        self.RemoveObserver(self.StartStopButtonTag)

    def ProcessGUIEvents(self,caller,event):
        if caller == self.StartStopButton and event == vtkKWPushButton_InvokedEvent:
            if self.StartStopButton.GetText() == "Start Recording":
                self.Start()
            else:
                self.Stop()

        if caller == self.TransformNodeSelector and event == vtkSlicerNodeSelectorWidget_NodeSelectedEvent:
            # in case we are recording, stop
            self.Stop()

        # this is not a GUI event, but it is more easily handled in a scripted
        # module by pretending it is a GUI event
        if caller == self.ObservedTransform and event == vtkMRMLTransformableNode_TransformModifiedEvent:
            self.RecordTransform() 

    def Status(self,text):
        slicer.ApplicationGUI.GetMainSlicerWindow().SetStatusText(text)

    def ErrorDialog(self,text):
        slicer.Application.InformationMessage(text)
        dialog = slicer.vtkKWMessageDialog()
        parent = slicer.ApplicationGUI.GetMainSlicerWindow()
        dialog.SetParent(parent)
        dialog.SetMasterWindow(parent)
        dialog.SetStyleToMessage()
        dialog.SetText(text)
        dialog.Create()
        dialog.Invoke()


    def BuildGUI(self):
        self.GetUIPanel().AddPage("IGTRecorder","IGTRecorder","")
        pageWidget = self.GetUIPanel().GetPageWidget("IGTRecorder")
        helpText = """
This module creates a polyline model that records the locations of a transform over time.  See <a>http://www.slicer.org/slicerWiki/index.php/Modules:IGTRecorder-Documentation-3.6</a> for more information.

Pick the Transform to be tracked then click Start Recording create the model if needed or append to the existing one.

Pressing Stop Recording will stop listenting to the transform and will stop modifying the polyline model.
"""
        aboutText = "This work is supported by NA-MIC, NAC, NCIGT, and the Slicer Community. See http://www.slicer.org for details.  Module implemented by Steve Pieper."
        self.BuildHelpAndAboutFrame(pageWidget,helpText,aboutText)
    
        moduleFrame = slicer.vtkSlicerModuleCollapsibleFrame()
        moduleFrame.SetParent(self.GetUIPanel().GetPageWidget("IGTRecorder"))
        moduleFrame.Create()
        moduleFrame.SetLabelText("IGTRecorder")
        moduleFrame.ExpandFrame()
        widgetName = moduleFrame.GetWidgetName()
        pageWidgetName = self.GetUIPanel().GetPageWidget("IGTRecorder").GetWidgetName()
        slicer.TkCall("pack %s -side top -anchor nw -fill x -padx 2 -pady 2 -in %s" % (widgetName,pageWidgetName))

        self.TransformNodeSelector.SetNodeClass("vtkMRMLTransformNode","","","")
        self.TransformNodeSelector.SetParent(moduleFrame.GetFrame())
        self.TransformNodeSelector.Create()
        self.TransformNodeSelector.SetMRMLScene(self.GetLogic().GetMRMLScene())
        self.TransformNodeSelector.UpdateMenu()
        self.TransformNodeSelector.SetBorderWidth(2)
        self.TransformNodeSelector.SetLabelText("Transform: ")
        self.TransformNodeSelector.SetBalloonHelpString("select a camera that will fly along this path.")
        slicer.TkCall("pack %s -side top -anchor e -padx 20 -pady 4 -expand true -fill x" % self.TransformNodeSelector.GetWidgetName())
    
        self.StartStopButton.SetParent(moduleFrame.GetFrame())
        self.StartStopButton.Create()
        self.StartStopButton.SetText("Start Recording")
        self.StartStopButton.SetWidth(18)
        self.StartStopButton.SetBalloonHelpString("create the path and show the fly through controller.")
        slicer.TkCall("pack %s -side top -anchor e -padx 20 -pady 10" % self.StartStopButton.GetWidgetName())

    def TearDownGUI(self):
        self.Stop()
        self.StartStopButton.SetParent(None)
        self.StartStopButton = None
   
        if self.GetUIPanel().GetUserInterfaceManager():
            pageWidget = self.GetUIPanel().GetPageWidget("IGTRecorder")
            self.GetUIPanel().RemovePage("IGTRecorder")


    def Stop(self):
        if self.ObservedTransform != 0:
            self.RemoveObserver(self.ObservedTransformTag)
            self.ObservedTransform = 0
        self.StartStopButton.SetText("Start Recording")

    def Start(self):
        transform = self.TransformNodeSelector.GetSelected()
        if not transform:
            self.ErrorDialog("Create a Transform Node in order to use IGTRecorder.")
            self.ObservedTransform = 0
            return

        self.ObservedTransform = transform
        self.ObservedTransformTag = self.AddObserverByNumber(self.ObservedTransform,vtkMRMLTransformableNode_TransformModifiedEvent)

        self.StartStopButton.SetText("Stop Recording")
        self.Status("Starting IGTRecorder for %s..." % self.ObservedTransform.GetName())

    def RecordTransform(self):
        if self.ObservedTransform == 0:
          return

        xlate = self.ObservedTransform.GetTransformToParent().TransformPoint(0,0,0)
        self.Status("IGTRecorder update for %g %g %g..." % (xlate[0], xlate[1], xlate[2]))

        try:
            self.modelID
        except AttributeError:
            self.modelID = ""

        modelNode = Slicer.slicer.MRMLScene.GetNodeByID(self.modelID)

        if not modelNode:
            # model node does not exist in scene, create a new one

            # create a vtkPolyData for a polyline
            vtk = Slicer.slicer
            points = vtk.vtkPoints()
            polyData = vtk.vtkPolyData()
            polyData.SetPoints(points)

            lines = vtk.vtkCellArray()
            polyData.SetLines(lines)
            linesIDArray = lines.GetData()
            linesIDArray.Reset()
            linesIDArray.InsertNextTuple1(0)

            polygons = vtk.vtkCellArray()
            polyData.SetPolys( polygons )
            idArray = polygons.GetData()
            idArray.Reset()
            idArray.InsertNextTuple1(0)

            # create model node
            modelNode = vtk.vtkMRMLModelNode()
            modelNode.SetScene(Slicer.slicer.MRMLScene)
            modelNode.SetName("Record-of-%s" % self.ObservedTransform.GetName())
            modelNode.SetAndObservePolyData(polyData)

            # create display node
            modelDisplay = vtk.vtkMRMLModelDisplayNode()
            modelDisplay.SetColor(1,.5,0) # yellow-ish-red
            modelDisplay.SetScene(Slicer.slicer.MRMLScene)
            Slicer.slicer.MRMLScene.AddNodeNoNotify(modelDisplay)
            modelNode.SetAndObserveDisplayNodeID(modelDisplay.GetID())

            # add to scene
            modelDisplay.SetPolyData(modelNode.GetPolyData())
            Slicer.slicer.MRMLScene.AddNode(modelNode)
            self.modelID = modelNode.GetID()

        # now we have the model node, so we can add a point to it
        # based on the current location of the transform
        polyData = modelNode.GetPolyData()
        points = polyData.GetPoints()
        lines = polyData.GetLines()
        linesIDArray = lines.GetData()

        pointIndex = points.InsertNextPoint (xlate[0], xlate[1], xlate[2])
        linesIDArray.InsertNextTuple1(pointIndex)
        linesIDArray.SetTuple1( 0, linesIDArray.GetNumberOfTuples() - 1 )
        lines.SetNumberOfCells(1)

        polyData.Modified()

