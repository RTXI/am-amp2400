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
	{ "Mode Bit 1", "", DefaultGUIModel::OUTPUT, }, // telegraph from DAQ used in 'Auto' mode
	{ "Mode Bit 2", "", DefaultGUIModel::OUTPUT, }, // telegraph from DAQ used in 'Auto' mode
	{ "Mode Bit 4", "", DefaultGUIModel::OUTPUT, },
	{ "Input Channel", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, }, 
	{ "Output Channel", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
//	{ "Headstage Gain", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, }, 
//	{ "Output Gain", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, }, 
	{ "Amplifier Mode", "", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

// Definition of global function used to get all DAQ devices available. Copied from legacy version of program. -Ansel
static void getDevice(DAQ::Device *d, void *p) {
	DAQ::Device **device = reinterpret_cast<DAQ::Device **>(p);

	if (!*device) *device = d;
}

// Just the constructor. 
AMAmp::AMAmp(void) : DefaultGUIModel("AMAmp 200 Controller", ::vars, ::num_vars) {
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
	output_channel = 0;
	amp_mode = 1;
//	output_gain = headstage_gain = 1;

	device = 0;
	DAQ::Manager::getInstance()->foreachDevice(getDevice, &device);

	// these are amplifier-specific settings. 
	iclamp_ai_gain = 1; // (1 V/V)
	iclamp_ao_gain = 1.0 / 2e-9; // (2 nA/V)
	izero_ai_gain = 1; // (1 V/V)
	izero_ao_gain = 0; // No output
	vclamp_ai_gain = 1e-12/1e-3; // (1 mV/pA)
	vclamp_ao_gain = 1 / 20e-3; // (20 mV/V)
};


void AMAmp::update(DefaultGUIModel::update_flags_t flag) {

	switch(flag) {
		// initialize the parameters and then the GUI. 
		case INIT:
			setParameter("Input Channel", input_channel);
			setParameter("Output Channel", output_channel);
//			setParameter("Headstage Gain", headstage_gain);
//			setParameter("Output Gain", output_gain);
			setParameter("Amplifier Mode", amp_mode);

			updateGUI();
			break;
		
		case MODIFY:
			input_channel = getParameter("Input Channel").toInt();
			output_channel = getParameter("Output Channel").toInt();
//			output_gain = getParameter("Output Gain").toDouble();
//			headstage_gain = getParameter("Headstage Gain").toDouble();
			if (amp_mode != getParameter("Amplifier Mode").toInt()) {
				ampButtonGroup->button(amp_mode)->setStyleSheet("QRadioButton { font: normal; }");
				ampButtonGroup->button(getParameter("Amplifier Mode").toInt())->setStyleSheet("QRadioButton { font: bold;}");
				amp_mode = getParameter("Amplifier Mode").toInt();
			}

			updateDAQ();
			updateGUI(); // only needed here because doLoad doesn't update the gui on its own. Yes, it does cause a bug with the headstage option, but it doesn't matter to anything other than whatever OCD tendencies we all probably have. You're welcome. -Ansel

			// blacken the GUI to reflect that changes have been saved to variables.
			inputBox->blacken();
			outputBox->blacken();
			headstageBox->blacken();
			outputGainBox->blacken();
			break;

		default:
			break;
	}
}

// used solely for initializing the gui when the module is opened or loaded via doLoad
void AMAmp::updateGUI(void) {
	// set the i/o channels
	inputBox->setValue(input_channel);
	outputBox->setValue(output_channel);

	// set the amplifier mode. The mode currently set is in bold.
//	switch(amp_mode) {
//		case 1:
//			ampButtonGroup->button(1)->setChecked(true);
//			ampButtonGroup->button(1)->setStyleSheet("QRadioButton { font: bold; }");
//			ampButtonGroup->button(2)->setStyleSheet("QRadioButton { font: normal; }");
//		case 2:
//			ampButtonGroup->button(2)->setChecked(true);
//			ampButtonGroup->button(2)->setStyleSheet("QRadioButton { font: bold; }");
//			ampButtonGroup->button(1)->setStyleSheet("QRadioButton { font: normal; }");
//		default:
//			ampButtonGroup->button(1)->setChecked(true);
//			ampButtonGroup->button(1)->setStyleSheet("QRadioButton { font: bold; }");
//			ampButtonGroup->button(2)->setStyleSheet("QRadioButton { font: normal; }");
//	}
}

// update the text in the block made by createGUI whenever the mode option changes. 
void AMAmp::updateMode(int value) {
	parameter["Amplifier Mode"].edit->setText(QString::number(value));
	parameter["Amplifier Mode"].edit->setModified(true);
	return;
}

// updates the output channel text whenever the value in the gui spinbox changes.
void AMAmp::updateOutputChannel(int value) {
	parameter["Output Channel"].edit->setText(QString::number(value));
	parameter["Output Channel"].edit->setText(QString::number(value));
	return;
}

// updates input channel
void AMAmp::updateInputChannel(int value) {
	parameter["Input Channel"].edit->setText(QString::number(value));
	parameter["Input Channel"].edit->setText(QString::number(value));
	return;
}

// updates the DAQ settings whenever the 'Set DAQ' button is pressed or when Auto mode detects a need for it.
void AMAmp::updateDAQ(void) {
	if (!device) return;

	switch(amp_mode) {
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
	iclampButton = new QRadioButton("IClamp");
	ampButtonGroup->addButton(iclampButton, 1);
	vclampButton = new QRadioButton("VClamp");
	ampButtonGroup->addButton(vclampButton, 2);
	izeroButton = new QRadioButton("");

	// add widgets to custom layout
	customLayout->addWidget(ioBoxLayout, 0, 0);
	customLayout->addWidget(ampModeBoxLayout, 2, 0);
	setLayout(customLayout);

	// connect the widgets to the signals
	QObject::connect(ampButtonGroup, SIGNAL(buttonPressed(int)), this, SLOT(updateMode(int)));
	QObject::connect(outputGainBox, SIGNAL(activated(int)), this, SLOT(updateOutputGain(int)));
	QObject::connect(headstageBox, SIGNAL(activated(int)), this, SLOT(updateHeadstageGain(int)));
	QObject::connect(inputBox, SIGNAL(valueChanged(int)), this, SLOT(updateInputChannel(int)));
	QObject::connect(outputBox, SIGNAL(valueChanged(int)), this, SLOT(updateOutputChannel(int)));
}


// overload the refresh function to display Auto mode settings and update the DAQ (when in Auto)
void AMAmp::refresh(void) {
	for (std::map<QString, param_t>::iterator i = parameter.begin(); i!= parameter.end(); ++i) {
		if (i->second.type & (STATE | EVENT)) {
			i->second.edit->setText(QString::number(getValue(i->second.type, i->second.index)));
			palette.setBrush(i->second.edit->foregroundRole(), Qt::darkGray);
			i->second.edit->setPalette(palette);
		} else if ((i->second.type & PARAMETER) && !i->second.edit->isModified() && i->second.edit->text() != *i->second.str_value) {
			i->second.edit->setText(*i->second.str_value);
		} else if ((i->second.type & COMMENT) && !i->second.edit->isModified() && i->second.edit->text() != QString::fromStdString(getValueString(COMMENT, i->second.index))) {
			i->second.edit->setText(QString::fromStdString(getValueString(COMMENT, i->second.index)));
		}
	}
	pauseButton->setChecked(!getActive());

	if (getActive()) {
		switch(amp_mode) {
			case 1:
				ampModeLabel->setText("IClamp");
				break;
			case 2:
				ampModeLabel->setText("VClamp");
				break;
			default:
				break;
		}
	}
	
	if (settings_changed) {
		updateDAQ();
		settings_changed = false;
	}
}
