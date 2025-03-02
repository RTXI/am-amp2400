#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTimer>

#include "widget.hpp"

#include <rtxi/daq.hpp>
#include <rtxi/debug.hpp>

Q_DECLARE_METATYPE(DAQ::Device*)

am_amp2400::AMAmpComboBox::AMAmpComboBox(QWidget* parent)
    : QComboBox(parent)
{
  QObject::connect(
      this, SIGNAL(currentIndexChanged(int)), this, SLOT(redden()));
}

void am_amp2400::AMAmpComboBox::blacken()
{
  this->setStyleSheet("QComboBox { color:black; }");
}

void am_amp2400::AMAmpComboBox::redden()
{
  this->setStyleSheet("QComboBox { color:red; }");
}

/// Create wrapper for QLineEdit. Options go red when changed and black when
/// 'Set DAQ' is hit.
am_amp2400::AMAmpLineEdit::AMAmpLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
  QObject::connect(
      this, SIGNAL(textEdited(const QString&)), this, SLOT(redden()));
}

void am_amp2400::AMAmpLineEdit::blacken()
{
  this->setStyleSheet("QLineEdit { color:black; }");
}

void am_amp2400::AMAmpLineEdit::redden()
{
  this->setStyleSheet("QLineEdit { color:red; }");
}

am_amp2400::AMAmpSpinBox::AMAmpSpinBox(QWidget* parent)
    : QSpinBox(parent)
{
  QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(redden()));
}

void am_amp2400::AMAmpSpinBox::blacken()
{
  this->setStyleSheet("QSpinBox { color:black; }");
}

void am_amp2400::AMAmpSpinBox::redden()
{
  this->setStyleSheet("QSpinBox { color:red; }");
}

am_amp2400::Plugin::Plugin(Event::Manager* ev_manager)
    : Widgets::Plugin(ev_manager, std::string(am_amp2400::MODULE_NAME))
{
}

am_amp2400::Panel::Panel(QMainWindow* main_window, Event::Manager* ev_manager)
    : Widgets::Panel(
          std::string(am_amp2400::MODULE_NAME), main_window, ev_manager)
{
  setWhatsThis(
      "<p>Controls the AM Amp 2400 amplifier by scaling the gains on the "
      "analog input/output channels and sending a mode telegraph to set the "
      "amplifier mode (custom).</p>");
  // We generate the default parameters in the panel but avoid displaying it
  // as this module creates it's own custom gui elements
  // createGUI(am_amp2400::get_default_vars(), {});
  // Get list of devices to control amplifier

  this->customizeGUI();
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

void am_amp2400::Panel::initParameters()
{
  input_channel = 0;
  output_channel = 0;
  mode = amp_mode::IEQ0;
  ai_offset = 0.0;
  ao_offset = 0.0;
  digital_line_0 = 0;
  digital_line_1 = 0;
  digital_line_2 = 0;
  zero_offset = 0;
  signal_count = 0;

  // these are amplifier-specific settings.
  // These values used to be assignable in past iterations of this plugin.
  // However what is the point of mutable variables when they are inherent
  // to the amplifier and never modified? now they are set at
  // compile-time and constant.
  //
  // iclamp_ai_gain = 200e-3;
  // iclamp_ao_gain = 500e6;
  // izero_ai_gain = 200e-3;
  // izero_ao_gain = 1;
  // vclamp_ai_gain = 2e-9;
  // vclamp_ao_gain = 50;
}

void am_amp2400::Panel::customizeGUI()
{
  // QGridLayout* customLayout = DefaultGUIModel::getLayout();
  // auto* widget_layout = dynamic_cast<QVBoxLayout*>(this->layout());
  // Since we don't use the default layout construction provided by RTXI we
  // opt to create our own layout. Note that this is essentially just a
  // QVBoxLayout class that rtxi gives us, so that's what we'll create here.
  auto* widget_layout = new QVBoxLayout;

  // We get a list of devices so that they are available for selection in
  // the combobox
  Event::Object device_list_request(Event::Type::DAQ_DEVICE_QUERY_EVENT);
  this->getRTXIEventManager()->postEvent(&device_list_request);
  auto devices = std::any_cast<std::vector<DAQ::Device*>>(
      device_list_request.getParam("devices"));

  auto* devicesComboBox = new QComboBox();
  for (auto* device : devices) {
    devicesComboBox->addItem(QString::fromStdString(device->getName()),
                             QVariant::fromValue(device));
  }

  widget_layout->addWidget(devicesComboBox);

  // create input spinboxes
  auto* ioGroupBox = new QGroupBox("Channels");
  auto* ioGroupLayout = new QGridLayout;
  ioGroupBox->setLayout(ioGroupLayout);

  auto* inputBoxLabel = new QLabel("Input");
  inputBox = new AMAmpSpinBox;  // this is the QSpinBox wrapper made earlier
  inputBox->setRange(0, 100);
  ioGroupLayout->addWidget(inputBoxLabel, 0, 0);
  ioGroupLayout->addWidget(inputBox, 0, 1);

  auto* outputBoxLabel = new QLabel("Output");
  outputBox = new AMAmpSpinBox;
  outputBox->setRange(0, 100);
  ioGroupLayout->addWidget(outputBoxLabel, 1, 0);
  ioGroupLayout->addWidget(outputBox, 1, 1);

  auto* bit1BoxLabel = new QLabel("Mode Bit 1");
  bit1Box = new AMAmpSpinBox;
  bit1Box->setRange(0, 100);
  ioGroupLayout->addWidget(bit1BoxLabel, 2, 0);
  ioGroupLayout->addWidget(bit1Box, 2, 1);

  auto* bit2BoxLabel = new QLabel("Mode Bit 2");
  bit2Box = new AMAmpSpinBox;
  bit2Box->setRange(0, 100);
  ioGroupLayout->addWidget(bit2BoxLabel, 3, 0);
  ioGroupLayout->addWidget(bit2Box, 3, 1);

  auto* bit4BoxLabel = new QLabel("Mode Bit 4");
  bit4Box = new AMAmpSpinBox;
  bit4Box->setRange(0, 100);
  ioGroupLayout->addWidget(bit4BoxLabel, 4, 0);
  ioGroupLayout->addWidget(bit4Box, 4, 1);

  // create amp mode groupbox
  auto* ampModeGroupBox = new QGroupBox("Amp Mode");
  auto* ampModeGroupLayout = new QGridLayout;
  ampModeGroupBox->setLayout(ampModeGroupLayout);

  auto* offsetLayout = new QGridLayout;
  offsetLayout->setColumnStretch(0, 1);
  auto* aiOffsetLabel = new QLabel("AI Offset:");
  offsetLayout->addWidget(aiOffsetLabel, 0, 0);
  aiOffsetEdit = new AMAmpLineEdit();
  aiOffsetEdit->setMaximumWidth(aiOffsetEdit->minimumSizeHint().width() * 3);
  aiOffsetEdit->setValidator(new QDoubleValidator(aiOffsetEdit));
  offsetLayout->addWidget(aiOffsetEdit, 0, 1);
  aiOffsetUnits = new QLabel;
  aiOffsetUnits->setText("1 V/V");
  offsetLayout->addWidget(aiOffsetUnits, 0, 2, Qt::AlignCenter);

  auto* aoOffsetLabel = new QLabel("AO Offset:");
  offsetLayout->addWidget(aoOffsetLabel, 1, 0);
  aoOffsetEdit = new AMAmpLineEdit();
  aoOffsetEdit->setMaximumWidth(aoOffsetEdit->minimumSizeHint().width() * 3);
  aoOffsetEdit->setValidator(new QDoubleValidator(aoOffsetEdit));
  offsetLayout->addWidget(aoOffsetEdit, 1, 1);
  aoOffsetUnits = new QLabel;
  aoOffsetUnits->setText("---");
  offsetLayout->addWidget(aoOffsetUnits, 1, 2, Qt::AlignCenter);

  ampModeGroupLayout->addLayout(offsetLayout, 0, 0);

  // add little bit of space betwen offsets and buttons
  ampModeGroupLayout->addItem(
      new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum),
      2,
      0);

  // add probe gain combobox
  auto* probeGainLayout = new QGridLayout;
  auto* probeGainLabel = new QLabel("Probe Gain:");
  probeGainComboBox = new AMAmpComboBox;
  probeGainComboBox->insertItem(0, QString::fromUtf8("Low (10 MΩ)"));
  probeGainComboBox->insertItem(1, QString::fromUtf8("High (10 GΩ)"));
  probeGainLayout->addWidget(probeGainLabel, 0, 0);
  probeGainLayout->addWidget(probeGainComboBox, 0, 1);
  ampModeGroupLayout->addLayout(probeGainLayout, 3, 0);

  // add little bit of space betwen offsets and buttons
  ampModeGroupLayout->addItem(
      new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum),
      4,
      0);

  // make the buttons
  ampButtonGroup = new QButtonGroup;
  vclampButton = new QRadioButton("VClamp");
  ampButtonGroup->addButton(vclampButton, VCLAMP);
  izeroButton = new QRadioButton("I = 0");
  ampButtonGroup->addButton(izeroButton, IEQ0);
  iclampButton = new QRadioButton("IClamp");
  ampButtonGroup->addButton(iclampButton, ICLAMP);
  vcompButton = new QRadioButton("VComp");
  ampButtonGroup->addButton(vcompButton, VCOMP);
  vtestButton = new QRadioButton("VTest");
  ampButtonGroup->addButton(vtestButton, VTEST);
  iresistButton = new QRadioButton("IResist");
  ampButtonGroup->addButton(iresistButton, IRESIST);
  ifollowButton = new QRadioButton("IFollow");
  ampButtonGroup->addButton(ifollowButton, IFOLLOW);

  auto* ampButtonGroupLayout = new QGridLayout;
  ampButtonGroupLayout->addWidget(vclampButton, 0, 0);
  ampButtonGroupLayout->addWidget(izeroButton, 0, 1);
  ampButtonGroupLayout->addWidget(iclampButton, 1, 0);
  ampButtonGroupLayout->addWidget(vcompButton, 1, 1);
  ampButtonGroupLayout->addWidget(vtestButton, 2, 0);
  ampButtonGroupLayout->addWidget(iresistButton, 2, 1);
  ampButtonGroupLayout->addWidget(ifollowButton, 3, 0);

  // We add our own set daq button
  auto* setDaqButton = new QPushButton("Set DAQ");

  ampModeGroupLayout->addLayout(ampButtonGroupLayout, 5, 0);

  // add widgets to custom layout
  widget_layout->addWidget(ioGroupBox);
  widget_layout->addWidget(ampModeGroupBox);
  widget_layout->addWidget(setDaqButton);
  setLayout(widget_layout);

  // connect the widgets to the signals
  QObject::connect(
      ampButtonGroup, SIGNAL(buttonPressed(int)), this, SLOT(updateMode(int)));
  QObject::connect(ampButtonGroup,
                   &QButtonGroup::idClicked,
                   this,
                   &am_amp2400::Panel::updateOffset);
  QObject::connect(inputBox,
                   QOverload<int>::of(&AMAmpSpinBox::valueChanged),
                   this,
                   &am_amp2400::Panel::updateInputChannel);
  QObject::connect(outputBox,
                   QOverload<int>::of(&AMAmpSpinBox::valueChanged),
                   this,
                   &am_amp2400::Panel::updateOutputChannel);
  QObject::connect(aiOffsetEdit,
                   &AMAmpLineEdit::textEdited,
                   this,
                   &am_amp2400::Panel::setAIOffset);
  QObject::connect(aoOffsetEdit,
                   &AMAmpLineEdit::textEdited,
                   this,
                   &am_amp2400::Panel::setAOOffset);
  QObject::connect(probeGainComboBox,
                   QOverload<int>::of(&QComboBox::currentIndexChanged),
                   this,
                   &am_amp2400::Panel::setProbeGain);
  QObject::connect(
      setDaqButton, &QPushButton::clicked, this, &am_amp2400::Panel::updateDAQ);
}

void am_amp2400::Panel::setProbeGain(int index)
{
  if (index > 2) {
    ERROR_MSG(
        "am_amp2400::Panel::setProbeGain : Invalid index passed. Check amp "
        "implementation");
    return;
  }
  mode = amp_mode(index);
}

void am_amp2400::Panel::updateDAQ()
{
  switch (this->mode) {
    case amp_mode::VCLAMP:  // VClamp
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 0);
        current_device->setAnalogGain(
            DAQ::ChannelType::AI, input_channel, vclamp_ai_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(
            DAQ::ChannelType::AO, output_channel, vclamp_ao_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }

      current_device->writeinput(digital_line_0, 0.0);
      current_device->writeinput(digital_line_1, 5.0);
      current_device->writeinput(digital_line_2, 0.0);
      break;

    case amp_mode::IEQ0:  // I = 0
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 3);
        current_device->setAnalogGain(
            DAQ::ChannelType::AI, input_channel, izero_ai_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(DAQ::ChannelType::AO,
                                      output_channel,
                                      izero_ao_gain * probe_gain_factor);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }

      current_device->writeinput(digital_line_0, 5.0);
      current_device->writeinput(digital_line_1, 5.0);
      current_device->writeinput(digital_line_2, 0.0);

      break;

    case amp_mode::ICLAMP:  // IClamp
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 3);
        current_device->setAnalogGain(
            DAQ::ChannelType::AI, input_channel, iclamp_ai_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(
            DAQ::ChannelType::AO, output_channel, iclamp_ao_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }
      current_device->writeinput(digital_line_0, 0.0);
      current_device->writeinput(digital_line_1, 0.0);
      current_device->writeinput(digital_line_2, 5.0);
      break;

    case amp_mode::VCOMP:  // VComp
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 0);
        current_device->setAnalogGain(
            DAQ::ChannelType::AI, input_channel, vclamp_ai_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(
            DAQ::ChannelType::AO, output_channel, vclamp_ao_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }

      current_device->writeinput(digital_line_0, 5.0);
      current_device->writeinput(digital_line_1, 0.0);
      current_device->writeinput(digital_line_2, 0.0);
      break;

    case amp_mode::VTEST:  // VTest
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 0);
        current_device->setAnalogGain(
            DAQ::ChannelType::AI, input_channel, vclamp_ai_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(
            DAQ::ChannelType::AO, output_channel, vclamp_ao_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }
      current_device->writeinput(digital_line_0, 0.0);
      current_device->writeinput(digital_line_1, 0.0);
      current_device->writeinput(digital_line_2, 0.0);
      break;

    case amp_mode::IRESIST:  // IResist
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 3);
        current_device->setAnalogGain(
            DAQ::ChannelType::AI, input_channel, iclamp_ai_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(DAQ::ChannelType::AO,
                                      output_channel,
                                      iclamp_ao_gain * probe_gain_factor);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }
      current_device->writeinput(digital_line_0, 5.0);
      current_device->writeinput(digital_line_1, 0.0);
      current_device->writeinput(digital_line_2, 5.0);

      break;

    case amp_mode::IFOLLOW:  // IFollow
      if (current_device != nullptr) {
        current_device->setAnalogRange(DAQ::ChannelType::AI, input_channel, 3);
        current_device->setAnalogGain(DAQ::ChannelType::AI,
                                      input_channel,
                                      iclamp_ai_gain * probe_gain_factor);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AI, input_channel, ai_offset);
        current_device->setAnalogGain(
            DAQ::ChannelType::AO, output_channel, iclamp_ao_gain);
        current_device->setAnalogZeroOffset(
            DAQ::ChannelType::AO, output_channel, ao_offset);
      }

      current_device->writeinput(digital_line_0, 0.0);
      current_device->writeinput(digital_line_1, 5.0);
      current_device->writeinput(digital_line_2, 5.0);
      break;

    default:
      ERROR_MSG(
          "ERROR. Something went horribly wrong. The amplifier mode "
          "is set to an unknown value");
      break;
  }
};

void am_amp2400::Panel::modify()
{
  input_channel = inputBox->text().toInt();
  output_channel = outputBox->text().toInt();

  ampButtonGroup->button(mode)->setStyleSheet("QRadioButton { font: normal; }");
  ampButtonGroup->button(mode)->setStyleSheet("QRadioButton { font: bold;}");
  const auto probe_gain = probe_gain_t(probeGainComboBox->currentIndex());
  switch (probe_gain) {
    case HIGH:
      probe_gain_factor = 1;  //.10;
      break;
    case LOW:
      probe_gain_factor = 10;  // 1;
      break;
    default:
      ERROR_MSG("ERROR: default called for probe_gain in update(MODIFY)");
      break;
  }

  ai_offset = this->aiOffsetEdit->text().toDouble();
  ao_offset = this->aoOffsetEdit->text().toDouble();

  updateDAQ();

  // blacken the GUI to reflect that changes have been saved to
  inputBox->blacken();
  outputBox->blacken();
  aiOffsetEdit->blacken();
  aoOffsetEdit->blacken();
  probeGainComboBox->blacken();
}

void am_amp2400::Panel::setAIOffset(const QString& offset)
{
  ai_offset = offset.toDouble();
}

void am_amp2400::Panel::setAOOffset(const QString& offset)
{
  ao_offset = offset.toDouble();
}

void am_amp2400::Panel::updateOffset(int new_mode)
{
  double scaled_ai_offset = ai_offset;
  double scaled_ao_offset = ao_offset;

  switch (mode) {
    case VCLAMP:  // VClamp
    case VCOMP:  // VComp
    case VTEST:  // VTest
      scaled_ai_offset = scaled_ai_offset * vclamp_ai_gain;
      scaled_ao_offset = scaled_ao_offset * vclamp_ao_gain;
      break;
    case IEQ0:  // I = 0
      scaled_ai_offset = scaled_ai_offset * izero_ai_gain;
      scaled_ao_offset = scaled_ao_offset * izero_ao_gain;
      break;
    case ICLAMP:  // IClamp
    case IRESIST:  // IResist
    case IFOLLOW:  // IFollow
      scaled_ai_offset = scaled_ai_offset * iclamp_ai_gain;
      scaled_ao_offset = scaled_ao_offset * iclamp_ao_gain;
      break;
    default:
      ERROR_MSG(
          "ERROR. Something went horribly wrong.\n The amplifier mode "
          "is set to an unknown value");
      break;
  }

  switch (new_mode) {
    case VCLAMP:  // VClamp
    case VCOMP:  // VComp
    case VTEST:  // VTest
      scaled_ai_offset = scaled_ai_offset / vclamp_ai_gain;
      scaled_ao_offset = scaled_ao_offset / vclamp_ao_gain;
      aiOffsetUnits->setText("1 mV/pA");
      aoOffsetUnits->setText("20 mV/V");
      break;
    case IEQ0:  // I = 0
      scaled_ai_offset = scaled_ai_offset / izero_ai_gain;
      scaled_ao_offset = scaled_ao_offset / izero_ao_gain;
      aiOffsetUnits->setText("1 V/V");
      aoOffsetUnits->setText("---");
      break;
    case IRESIST:  // IResist
    case IFOLLOW:  // IFollow
    case ICLAMP:  // IClamp
      scaled_ai_offset = scaled_ai_offset / iclamp_ai_gain;
      scaled_ao_offset = scaled_ao_offset / iclamp_ao_gain;
      aiOffsetUnits->setText("1 V/V");
      aoOffsetUnits->setText("2 nA/V");
      break;
    default:
      ERROR_MSG(
          "ERROR. Something went horribly wrong.\n The amplifier mode "
          "is set to an unknown value");
      break;
  }

  aiOffsetEdit->setText(QString::number(scaled_ai_offset));
  aiOffsetEdit->setModified(true);
  aoOffsetEdit->setText(QString::number(scaled_ao_offset));
  aoOffsetEdit->setModified(true);
}

void am_amp2400::Panel::updateMode(int value)
{
  mode = amp_mode(value);
}

void am_amp2400::Panel::updateOutputChannel(int value)
{
  output_channel = value;
}

void am_amp2400::Panel::updateInputChannel(int value)
{
  input_channel = value;
}

/*
void am_amp2400::Panel::findZeroOffset()
{
  findZeroButton->setEnabled(false);
  iclampButton->setEnabled(false);
  vclampButton->setEnabled(false);
  izeroButton->setEnabled(false);
  vcompButton->setEnabled(false);
  vtestButton->setEnabled(false);
  iresistButton->setEnabled(false);
  ifollowButton->setEnabled(false);

  pause(false);
}
*/

// It is not clear why this was created. It is not neccessary to calibrate
// the output signal with the new drivers since it is forcebly set when you
// write to AO. Analog inputs may be calibrated, but the input analog
// calibration in previous iterations of this plugin did not actually work! It
// seems redundant and unused feature. If the user wishes to automatically
// calibrate input analog channels then perhaps we can implement it here, but
// for now this is disabled.

/*
void am_amp2400::Panel::calculateOffset()
{
  if (!data_acquired) {
    std::cout << "Not done yet... " << signal_count << std::endl;
    return;
  }
  checkZeroCalc->stop();
  pause(true);

  switch (channel) {
    case AI:
      zero_offset = zero_signal.mean();
      std::cout << "AI Calc. Offst. = " << zero_offset * izero_ai_gain
                << std::endl;
      std::cout << "AI Curr. Offst. = " << ai_offset << std::endl;
      std::cout << "AI Total Offst. = "
                << zero_offset * izero_ai_gain + ai_offset << std::endl;
      zero_signal.clear();

      aiOffsetEdit->setText(
          QString::number(zero_offset * izero_ai_gain + ai_offset));
      aiOffsetEdit->redden();
      parameter["AI Offset"].edit->setText(
          QString::number(zero_offset * izero_ai_gain + ai_offset));
      parameter["AI Offset"].edit->setModified(true);

      channel = AO;
      data_acquired = false;
      zero_offset = 0;
      checkZeroCalc->start();
      pause(false);
      break;

    case AO:
      zero_offset = zero_signal.mean();
      std::cout << "AO Calc. Offst. = " << zero_offset << std::endl;
      std::cout << "AO Curr. Offst. = " << ao_offset << std::endl;
      std::cout << "AO Total Offst. = " << ao_offset + zero_offset <<
std::endl
                << std::endl;
      zero_signal.clear();

      aoOffsetEdit->setText(QString::number(ao_offset + zero_offset));
      aoOffsetEdit->redden();
      parameter["AO Offset"].edit->setText(
          QString::number(zero_offset + ao_offset));
      parameter["AO Offset"].edit->setModified(true);

      findZeroButton->setEnabled(true);
      iclampButton->setEnabled(true);
      vclampButton->setEnabled(true);
      izeroButton->setEnabled(true);
      vcompButton->setEnabled(true);
      vtestButton->setEnabled(true);
      iresistButton->setEnabled(true);
      ifollowButton->setEnabled(true);
      modifyButton->setEnabled(true);

      channel = NA;
      zero_offset = 0;
      break;

    case NA:
      std::cout << "Case NA isn't supposed to be called." << std::endl;
      break;

    default:
      std::cout << "ERROR: default case called." << std::endl;
      break;
  }
}*/

std::unique_ptr<Widgets::Plugin> createRTXIPlugin(Event::Manager* ev_manager)
{
  return std::make_unique<am_amp2400::Plugin>(ev_manager);
}

Widgets::Panel* createRTXIPanel(QMainWindow* main_window,
                                Event::Manager* ev_manager)
{
  return new am_amp2400::Panel(main_window, ev_manager);
}

std::unique_ptr<Widgets::Component> createRTXIComponent(
    Widgets::Plugin* host_plugin)
{
  return nullptr;
}

Widgets::FactoryMethods fact;

extern "C"
{
Widgets::FactoryMethods* getFactories()
{
  fact.createPanel = &createRTXIPanel;
  fact.createComponent = &createRTXIComponent;
  fact.createPlugin = &createRTXIPlugin;
  return &fact;
}
};

//////////// END //////////////////////
