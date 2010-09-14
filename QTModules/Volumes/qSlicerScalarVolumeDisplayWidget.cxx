#include "qSlicerScalarVolumeDisplayWidget.h"
#include "ui_qSlicerScalarVolumeDisplayWidget.h"

// Qt includes
#include <QDebug>

// CTK includes
#include <ctkVTKColorTransferFunction.h>
#include <ctkTransferFunctionGradientItem.h>
#include <ctkTransferFunctionScene.h>
#include <ctkTransferFunctionBarsItem.h>
#include <ctkVTKHistogram.h>

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

// STD includes
#include <limits>

//-----------------------------------------------------------------------------
class qSlicerScalarVolumeDisplayWidgetPrivate: public ctkPrivate<qSlicerScalarVolumeDisplayWidget>,
                                          public Ui_qSlicerScalarVolumeDisplayWidget
{
public:
  qSlicerScalarVolumeDisplayWidgetPrivate();
  ~qSlicerScalarVolumeDisplayWidgetPrivate();
  void init();

  ctkVTKHistogram* Histogram;
  vtkSmartPointer<vtkColorTransferFunction> ColorTransferFunction;
};

//-----------------------------------------------------------------------------
qSlicerScalarVolumeDisplayWidgetPrivate::qSlicerScalarVolumeDisplayWidgetPrivate()
{
  this->Histogram = new ctkVTKHistogram();
  this->ColorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
}

//-----------------------------------------------------------------------------
qSlicerScalarVolumeDisplayWidgetPrivate::~qSlicerScalarVolumeDisplayWidgetPrivate()
{
  delete this->Histogram;
}
//-----------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidgetPrivate::init()
{
  CTK_P(qSlicerScalarVolumeDisplayWidget);

  this->setupUi(p);

  ctkTransferFunctionScene* scene = qobject_cast<ctkTransferFunctionScene*>(
    this->TransferFunctionView->scene());
  // Transfer Function
  ctkVTKColorTransferFunction* transferFunction =
    new ctkVTKColorTransferFunction(this->ColorTransferFunction, p);

  ctkTransferFunctionGradientItem* gradientItem =
    new ctkTransferFunctionGradientItem(transferFunction);
  scene->addItem(gradientItem);
  // Histogram
  //scene->setTransferFunction(this->Histogram);
  ctkTransferFunctionBarsItem* barsItem =
    new ctkTransferFunctionBarsItem(this->Histogram);
  barsItem->setBarWidth(1.);
  scene->addItem(barsItem);

  QObject::connect(this->InterpolateCheckbox, SIGNAL(toggled(bool)),
                   p, SLOT(setInterpolate(bool)));
  QObject::connect(this->ColorTableComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                   p, SLOT(setColorNode(vtkMRMLNode*)));

  QObject::connect(this->CTBonePresetToolButton, SIGNAL(clicked()),
                   p, SLOT(onPresetButtonClicked()));
  QObject::connect(this->CTAirPresetToolButton, SIGNAL(clicked()),
                   p, SLOT(onPresetButtonClicked()));
  QObject::connect(this->PETPresetToolButton, SIGNAL(clicked()),
                   p, SLOT(onPresetButtonClicked()));
  QObject::connect(this->CTAbdomenPresetToolButton, SIGNAL(clicked()),
                   p, SLOT(onPresetButtonClicked()));
  QObject::connect(this->CTBrainPresetToolButton, SIGNAL(clicked()),
                   p, SLOT(onPresetButtonClicked()));
  QObject::connect(this->CTLungPresetToolButton, SIGNAL(clicked()),
                   p, SLOT(onPresetButtonClicked()));
}

// --------------------------------------------------------------------------
qSlicerScalarVolumeDisplayWidget::qSlicerScalarVolumeDisplayWidget(QWidget* _parent) : Superclass(_parent)
{
  CTK_INIT_PRIVATE(qSlicerScalarVolumeDisplayWidget);
  CTK_D(qSlicerScalarVolumeDisplayWidget);
  d->init();

  // disable as there is not MRML Node associated with the widget
  this->setEnabled(false);
}

// --------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* qSlicerScalarVolumeDisplayWidget::volumeNode()const
{
  CTK_D(const qSlicerScalarVolumeDisplayWidget);
  return vtkMRMLScalarVolumeNode::SafeDownCast(
    d->MRMLWindowLevelWidget->mrmlVolumeNode());
}

// --------------------------------------------------------------------------
vtkMRMLScalarVolumeDisplayNode* qSlicerScalarVolumeDisplayWidget::volumeDisplayNode()const
{
  vtkMRMLVolumeNode* volumeNode = this->volumeNode();
  return volumeNode ? vtkMRMLScalarVolumeDisplayNode::SafeDownCast(
    volumeNode->GetDisplayNode()) : 0;
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::setMRMLVolumeNode(vtkMRMLNode* node)
{
  this->setMRMLVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::setMRMLVolumeNode(vtkMRMLScalarVolumeNode* volumeNode)
{
  CTK_D(qSlicerScalarVolumeDisplayWidget);

  vtkMRMLScalarVolumeDisplayNode* oldVolumeDisplayNode = this->volumeDisplayNode();

  d->MRMLWindowLevelWidget->setMRMLVolumeNode(volumeNode);
  d->MRMLVolumeThresholdWidget->setMRMLVolumeNode(volumeNode);

  qvtkReconnect(oldVolumeDisplayNode, volumeNode ? volumeNode->GetDisplayNode() :0,
                vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));
  d->Histogram->setDataArray(volumeNode &&
                             volumeNode->GetImageData() &&
                             volumeNode->GetImageData()->GetPointData() ?
                             volumeNode->GetImageData()->GetPointData()->GetScalars() :
                             0);
  d->Histogram->build();
  this->setEnabled(volumeNode != 0);

  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::updateWidgetFromMRML()
{
  CTK_D(qSlicerScalarVolumeDisplayWidget);
  vtkMRMLScalarVolumeDisplayNode* displayNode =
    this->volumeDisplayNode();
  if (displayNode)
    {
    d->ColorTableComboBox->setCurrentNode(displayNode->GetColorNode());
    d->InterpolateCheckbox->setChecked(displayNode->GetInterpolate());
    }
  if (this->isVisible())
    {
    this->updateTransferFunction();
    }
}

//----------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::updateTransferFunction()
{
  CTK_D(qSlicerScalarVolumeDisplayWidget);
  // from vtkKWWindowLevelThresholdEditor::UpdateTransferFunction
  vtkMRMLVolumeNode* volumeNode = d->MRMLWindowLevelWidget->mrmlVolumeNode();
  Q_ASSERT(volumeNode == d->MRMLVolumeThresholdWidget->mrmlVolumeNode());
  vtkImageData* imageData = volumeNode ? volumeNode->GetImageData() : 0;
  if (imageData == 0)
    {
    d->ColorTransferFunction->RemoveAllPoints();
    return;
    }
  double range[2] = {0,255};
  imageData->GetScalarRange(range);
  // AdjustRange call will take out points that are outside of the new
  // range, but it needs the points to be there in order to work, so call
  // RemoveAllPoints after it's done
  d->ColorTransferFunction->AdjustRange(range);
  d->ColorTransferFunction->RemoveAllPoints();

  double min = d->MRMLWindowLevelWidget->level() - 0.5 * d->MRMLWindowLevelWidget->window();
  double max = d->MRMLWindowLevelWidget->level() + 0.5 * d->MRMLWindowLevelWidget->window();
  double minVal = 0;
  double maxVal = 1;
  double low   = d->MRMLVolumeThresholdWidget->isOff() ? range[0] : d->MRMLVolumeThresholdWidget->lowerThreshold();
  double upper = d->MRMLVolumeThresholdWidget->isOff() ? range[1] : d->MRMLVolumeThresholdWidget->upperThreshold();

  d->ColorTransferFunction->SetColorSpaceToRGB();

  if (low >= max || upper <= min)
    {
    d->ColorTransferFunction->AddRGBPoint(range[0], 0, 0, 0);
    d->ColorTransferFunction->AddRGBPoint(range[1], 0, 0, 0);
    }
  else
    {
    max = qMax(min+0.001, max);
    low = qMax(range[0] + 0.001, low);
    min = qMax(range[0] + 0.001, min);
    upper = qMin(range[1] - 0.001, upper);

    if (min <= low)
      {
      minVal = (low - min)/(max - min);
      min = low + 0.001;
      }

    if (max >= upper)
      {
      maxVal = (upper - min)/(max-min);
      max = upper - 0.001;
      }

    d->ColorTransferFunction->AddRGBPoint(range[0], 0, 0, 0);
    d->ColorTransferFunction->AddRGBPoint(low, 0, 0, 0);
    d->ColorTransferFunction->AddRGBPoint(min, minVal, minVal, minVal);
    d->ColorTransferFunction->AddRGBPoint(max, maxVal, maxVal, maxVal);
    d->ColorTransferFunction->AddRGBPoint(upper, maxVal, maxVal, maxVal);
    if (upper+0.001 < range[1])
      {
      d->ColorTransferFunction->AddRGBPoint(upper+0.001, 0, 0, 0);
      d->ColorTransferFunction->AddRGBPoint(range[1], 0, 0, 0);
      }
    }

  d->ColorTransferFunction->SetAlpha(1.0);
  d->ColorTransferFunction->Build();
}

// -----------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::showEvent( QShowEvent * event )
{
  this->updateTransferFunction();
  this->Superclass::showEvent(event);
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::setInterpolate(bool interpolate)
{
  vtkMRMLScalarVolumeDisplayNode* displayNode =
    this->volumeDisplayNode();
  if (!displayNode)
    {
    return;
    }
  displayNode->SetInterpolate(interpolate);
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::setColorNode(vtkMRMLNode* colorNode)
{
  vtkMRMLScalarVolumeDisplayNode* displayNode =
    this->volumeDisplayNode();
  if (!displayNode || !colorNode)
    {
    return;
    }
  Q_ASSERT(vtkMRMLColorNode::SafeDownCast(colorNode));
  displayNode->SetAndObserveColorNodeID(colorNode->GetID());
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::onPresetButtonClicked()
{
  QToolButton* preset = qobject_cast<QToolButton*>(this->sender());
  this->setPreset(preset->accessibleName());
}

// --------------------------------------------------------------------------
void qSlicerScalarVolumeDisplayWidget::setPreset(const QString& presetName)
{
  CTK_D(qSlicerScalarVolumeDisplayWidget);
  QString colorNodeID;
  double window = -1.;
  double level = std::numeric_limits<double>::max();
  if (presetName == "CT-Bone")
    {
    colorNodeID = "vtkMRMLColorTableNodeGrey";
    window = 1000.;
    level = 400.;
    }
  else if (presetName == "CT-Air")
    {
    colorNodeID = "vtkMRMLColorTableNodeGrey";
    window = 1000.;
    level = -426.;
    }
  else if (presetName == "PET")
    {
    colorNodeID = "vtkMRMLColorTableNodeRainbow";
    window = 10000.;
    level = 6000.;
    }
  else if (presetName == "CT-Abdomen")
    {
    colorNodeID = "vtkMRMLColorTableNodeGrey";
    window = 350.;
    level = 40.;
    }
  else if (presetName == "CT-Brain")
    {
    colorNodeID = "vtkMRMLColorTableNodeGrey";
    window = 100.;
    level = 50.;
    }
  else if (presetName == "CT-Lung")
    {
    colorNodeID = "vtkMRMLColorTableNodeGrey";
    window = 1400.;
    level = -500.;
    }

  vtkMRMLNode* colorNode = this->mrmlScene()->GetNodeByID(colorNodeID.toLatin1());
  if (colorNode)
    {
    this->setColorNode(colorNode);
    }
  if (window != -1 || level!= std::numeric_limits<double>::max())
    {
    d->MRMLWindowLevelWidget->setAutoWindowLevel(qMRMLWindowLevelWidget::Manual);
    }
  if (window != -1 && level != std::numeric_limits<double>::max())
    {
    d->MRMLWindowLevelWidget->setWindowLevel(window, level);
    }
  else if (window != -1)
    {
    d->MRMLWindowLevelWidget->setWindow(window);
    }
  else if (level != std::numeric_limits<double>::max())
    {
    d->MRMLWindowLevelWidget->setLevel(level);
    }
}
