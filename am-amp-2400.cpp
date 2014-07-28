#include <QtGui>
#include <iostream>
#include "am-amp-2400.h"

// Create wrapper for QComboBox. Options go red when changed and black when 'Set DAQ' is hit.
AMAmpComboBox::AMAmpComboBox(QWidget *parent) : QComboBox(parent) {
	QObject::connect(this, SIGNAL(activated(int)), this, SLOT(redden(void)));
}

AMAmpComboBox::~AMAmpComboBox(void) {}

void AMAmpComboBox::blacken(void) {
	palette.setColor(QPalette::Text, Qt::black);
	this->setPalette(palette);
}

void AMAmpComboBox::redden(void) {
	palette.setColor(QPalette::Text, Qt::red);
	this->setPalette(palette);
}

// Create wrapper for spinboxes. Function is analogous to AMAmpComboBox
// SpinBox was used instead of DefaultGUILineEdit because palette.setBrush(etc...) doesn't change colors when changes are done programmatically. 
AMAmpSpinBox::AMAmpSpinBox(QWidget *parent) : QSpinBox(parent) {
	QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(redden(void)));
}

AMAmpSpinBox::~AMAmpSpinBox(void) {}

void AMAmpSpinBox::blacken(void) {
	palette.setColor(QPalette::Text, Qt::black);
	this->setPalette(palette);
}

void AMAmpSpinBox::redden(void) {
	palette.setColor(QPalette::Text, Qt::red);
	this->setPalette(palette);
}


/* This is the real deal, the definitions for all the AMAmp functions.
 */
extern "C" Plugin::Object * createRTXIPlugin(void) {
	return new AMAmp();
};

static DefaultGUIModel::variable_t vars[] = {
	{ "Mode Bit 1", "", DefaultGUIModel::OUTPUT, }, 
	{ "Mode Bit 2", "", DefaultGUIModel::OUTPUT, }, 
	{ "Mode Bit 4", "", DefaultGUIModel::OUTPUT, },
	{ "Input Channel", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, }, 
	{ "Output Channel", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
	{ "Amplifier Mode", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

// Definition of global function used to get all DAQ devices available. Copied from legacy version of program. -Ansel
static void getDevice(DAQ::Device *d, void *p) {
	DAQ::Device **device = reinterpret_cast<DAQ::Device **>(p);

	if (!*device) *device = d;
}

// Just the constructor. 
AMAmp::AMAmp(void) : DefaultGUIModel("AM Amp 2400 Controller", ::vars, ::num_vars) {
	setWhatsThis("<p>Yeah, I'll get to this later... <br>-Ansel</p>");
	DefaultGUIModel::createGUI(vars, num_vars);
	initParameters();
	customizeGUI();
	update( INIT );
	DefaultGUIModel::refresh();
};

AMAmp::~AMAmp(void) {};

void AMAmp::initParameters(void) {
	input_channel = 0;
	output_channel = 1;
	amp_mode = 2;

	device = 0;
	DAQ::Manager::getInstance()->foreachDevice(getDevice, &device);

	// these are amplifier-specific settings. 
	iclamp_ai_gain = 200e-3;
	iclamp_ao_gain = 500e6;
	izero_ai_gain = 200e-3;
	izero_ao_gain = 1;
	vclamp_ai_gain = 2e-9;
	vclamp_ao_gain = 50;
};


void AMAmp::update(DefaultGUIModel::update_flags_t flag) {

	switch(flag) {
		// initialize the parameters and then the GUI. 
		case INIT:
			setParameter("Input Channel", input_channel);
			setParameter("Output Channel", output_channel);
			setParameter("Amplifier Mode", amp_mode);

			inputBox->setValue(input_channel);
			outputBox->setValue(output_channel);
			ampButtonGroup->button(amp_mode)->setStyleSheet("QRadioButton { font: bold;}");
			ampButtonGroup->button(amp_mode)->setChecked(true);
			break;
		
		case MODIFY:
			input_channel = getParameter("Input Channel").toInt();
			output_channel = getParameter("Output Channel").toInt();

			inputBox->setValue(input_channel);
			outputBox->setValue(output_channel);
			if (amp_mode != getParameter("Amplifier Mode").toInt()) {
				ampButtonGroup->button(amp_mode)->setStyleSheet("QRadioButton { font: normal; }");
				ampButtonGroup->button(getParameter("Amplifier Mode").toInt())->setStyleSheet("QRadioButton { font: bold;}");
				amp_mode = getParameter("Amplifier Mode").toInt();
			}

			updateDAQ();

			// blacken the GUI to reflect that changes have been saved to variables.
			inputBox->blacken();
			outputBox->blacken();
			break;

		default:
			break;
	}
}

// update the text in the block made by createGUI whenever the mode option changes. 
void AMAmp::updateMode(int value) {
	parameter["Amplifier Mode"].edit->setText(QString::number(value));
	parameter["Amplifier Mode"].edit->setModified(true);

	update( MODIFY );
	return;
}

// updates the output channel text whenever the value in the gui spinbox changes.
void AMAmp::updateOutputChannel(int value) {
	parameter["Output Channel"].edit->setText(QString::number(value));
	parameter["Output Channel"].edit->setModified(true);
	return;
}

// updates input channel
void AMAmp::updateInputChannel(int value) {
	parameter["Input Channel"].edit->setText(QString::number(value));
	parameter["Input Channel"].edit->setModified(true);
	return;
}

// updates the DAQ settings whenever the 'Set DAQ' button is pressed or when Auto mode detects a need for it.
void AMAmp::updateDAQ(void) {
//	if (!device) return;

	switch(amp_mode) {
		case 1: // VClamp
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 0);
				device->setAnalogGain(DAQ::AI, input_channel, vclamp_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, vclamp_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 0.0;
			output(2) = 5.0;
			break;

		case 2: // I = 0
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, izero_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, izero_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 5.0;
			output(1) = 5.0;
			output(2) = 0.0;
			break;

		case 3: // IClamp
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 0.0;
			output(2) = 5.0;
			break;

		case 4: // VComp
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 0);
				device->setAnalogGain(DAQ::AI, input_channel, vclamp_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, vclamp_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 5.0;
			output(1) = 0.0;
			output(2) = 0.0;
			break;

		case 5: // VTest
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 0);
				device->setAnalogGain(DAQ::AI, input_channel, vclamp_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, vclamp_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 0.0;
			output(2) = 0.0;
			break;

		case 6: // IResist
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 5.0;
			output(1) = 0.0;
			output(2) = 5.0;
			break;

		case 7: // IFollow
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain);
				device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain);
				device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 5.0;
			output(2) = 5.0;
			break;

		default:
			std::cout<<"ERROR. Something went horribly wrong.\n The amplifier mode is set to an unknown value"<<std::endl;
			break;
	}
};

/* 
 * Sets up the GUI. It's a bit messy. These are the important things to remember:
 *   1. The parameter/state block created by DefaultGUIModel is HIDDEN. 
 *   2. The Unload button is hidden, Pause is renamed 'Auto', and Modify is renamed 'Set DAQ'
 *   3. All GUI changes are connected to their respective text boxes in the hidden block
 *   4. 'Set DAQ' updates the values of inner variables with GUI choices linked to the text boxes
 * 
 * Okay, here we go!
 */
void AMAmp::customizeGUI(void) {
	QGridLayout *customLayout = DefaultGUIModel::getLayout();
	
//	customLayout->itemAtPosition(1,0)->widget()->setVisible(false);
	DefaultGUIModel::pauseButton->setVisible(false);
	DefaultGUIModel::modifyButton->setText("Set DAQ");
	DefaultGUIModel::unloadButton->setVisible(false);

	// create input spinboxes
	QFormLayout *ioBoxLayout = new QFormLayout;
	inputBox = new AMAmpSpinBox; // this is the QSpinBox wrapper made earlier
	inputBox->setRange(0,100);
	outputBox = new AMAmpSpinBox;
	outputBox->setRange(0,100);
	
	QLabel *inputBoxLabel = new QLabel("Input");
	QLabel *outputBoxLabel = new QLabel("Output");
	ioBoxLayout->addRow(inputBoxLabel, inputBox);
	ioBoxLayout->addRow(outputBoxLabel, outputBox);

	// create amp mode groupbox
	QVBoxLayout *ampModeBoxLayout = new QVBoxLayout;
	ampButtonGroup = new QButtonGroup;
	vclampButton = new QRadioButton("VClamp");
	ampButtonGroup->addButton(vclampButton, 1);
	izeroButton = new QRadioButton("I = 0");
	ampButtonGroup->addButton(izeroButton, 2);
	iclampButton = new QRadioButton("IClamp");
	ampButtonGroup->addButton(iclampButton, 3);
	vcompButton = new QRadioButton("VComp");
	ampButtonGroup->addButton(vcompButton, 4);
	vtestButton = new QRadioButton("VTest");
	ampButtonGroup->addButton(vtestButton, 5);
	iresistButton = new QRadioButton("IResist");
	ampButtonGroup->addButton(iresistButton, 6);
	ifollowButton = new QRadioButton("IFollow");
	ampButtonGroup->addButton(ifollowButton, 7);

	ampModeBoxLayout->addWidget(vclampButton);
	ampModeBoxLayout->addWidget(izeroButton);
	ampModeBoxLayout->addWidget(iclampButton);
	ampModeBoxLayout->addWidget(vcompButton);
	ampModeBoxLayout->addWidget(vtestButton);
	ampModeBoxLayout->addWidget(iresistButton);
	ampModeBoxLayout->addWidget(ifollowButton);

	// add widgets to custom layout
	customLayout->addLayout(ioBoxLayout, 0, 0);
	customLayout->addLayout(ampModeBoxLayout, 2, 0, Qt::AlignCenter);
	setLayout(customLayout);

	// connect the widgets to the signals
	QObject::connect(ampButtonGroup, SIGNAL(buttonPressed(int)), this, SLOT(updateMode(int)));
	QObject::connect(inputBox, SIGNAL(valueChanged(int)), this, SLOT(updateInputChannel(int)));
	QObject::connect(outputBox, SIGNAL(valueChanged(int)), this, SLOT(updateOutputChannel(int)));
}
