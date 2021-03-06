/*
	Copyright (C) 2011 Georgia Institute of Technology, University of Utah, Weill Cornell Medical College

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include "am-amp2400.h"

// Wrapper for QComboBox. Turns red when changed and black when "Set DAQ" is hit.
AMAmpComboBox::AMAmpComboBox(QWidget *parent) : QComboBox(parent) {
	QObject::connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(redden(void)));
}

AMAmpComboBox::~AMAmpComboBox(void) {}

void AMAmpComboBox::blacken(void) {
	this->setStyleSheet("QComboBox { color:black; }");
}

void AMAmpComboBox::redden(void) {
	this->setStyleSheet("QComboBox { color:red; }");
}

/// Create wrapper for QLineEdit. Options go red when changed and black when 'Set DAQ' is hit.
AMAmpLineEdit::AMAmpLineEdit(QWidget *parent) : QLineEdit(parent) {
	QObject::connect(this, SIGNAL(textEdited(const QString &)), this, SLOT(redden(void)));
}

AMAmpLineEdit::~AMAmpLineEdit(void) {}

void AMAmpLineEdit::blacken(void) {
	this->setStyleSheet("QLineEdit { color:black; }");
}

void AMAmpLineEdit::redden(void) {
	this->setStyleSheet("QLineEdit { color:red; }");
}

/*
 * Create wrapper for spinboxes. Function is analogous to AMAmpComboBox
 * SpinBox was used instead of DefaultGUILineEdit because palette.setBrush(etc...) 
 * doesn't change colors when changes are done programmatically. 
 */
AMAmpSpinBox::AMAmpSpinBox(QWidget *parent) : QSpinBox(parent) {
	QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(redden(void)));
}

AMAmpSpinBox::~AMAmpSpinBox(void) {}

void AMAmpSpinBox::blacken(void) {
	this->setStyleSheet("QSpinBox { color:black; }");
}

void AMAmpSpinBox::redden(void) {
	this->setStyleSheet("QSpinBox { color:red; }");
}


/* 
 * This is the real deal.
 */
extern "C" Plugin::Object * createRTXIPlugin(void) {
	return new AMAmp();
};

static DefaultGUIModel::variable_t vars[] = {
	{ "I=0 Input from AI", "Empty signal from analog input for 'calibrating' the input channel for I=0.", DefaultGUIModel::INPUT, }, 
	{ "I=0 Input from AO", "Empty signal from analog output for 'calibrating' the output channel for I=0.", DefaultGUIModel::INPUT, }, 
	{ "Mode Bit 1", "Bit 1 of signal sent to amplfier", DefaultGUIModel::OUTPUT, }, 
	{ "Mode Bit 2", "Bit 2 of signal sent to amplfier", DefaultGUIModel::OUTPUT, }, 
	{ "Mode Bit 4", "Bit 4 of signal sent to amplfier", DefaultGUIModel::OUTPUT, },
	{ "Input Channel", "Input channel to scale (#)", DefaultGUIModel::PARAMETER | 
	  DefaultGUIModel::INTEGER, }, 
	{ "Output Channel", "Output channel to scale (#)", DefaultGUIModel::PARAMETER | 
	  DefaultGUIModel::INTEGER, },
	{ "Amplifier Mode", "Mode to telegraph to amplifier", DefaultGUIModel::PARAMETER | 
	  DefaultGUIModel::INTEGER, },
	{ "Probe Gain", "Gain - HIGH (10 MOhm) or LOW (10 GOhm)", DefaultGUIModel::PARAMETER | 
	  DefaultGUIModel::INTEGER, },
	{ "AI Offset", 
	  "Offset the amplifier scaling (entered manually or computed based on current amplifier mode)",
	   DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "AO Offset", 
	  "Offset the amplifier scaling (entered manually or computed based on current amplifier mode)",
	   DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

/*
 * Definition of global function used to get all DAQ devices available to the analogy driver. 
 * Copied from legacy version of program.
 */
static void getDevice(DAQ::Device *d, void *p) {
	DAQ::Device **device = reinterpret_cast<DAQ::Device **>(p);

	if (!*device) *device = d;
}

// Just the constructor. 
AMAmp::AMAmp(void) : DefaultGUIModel("AM Amp 2400 Controller", ::vars, ::num_vars) {
	setWhatsThis("<p>Controls the AM Amp 2400 amplifier by scaling the gains on the analog input/output channels and sending a mode telegraph to set the amplifier mode (custom).</p>");
	DefaultGUIModel::createGUI(vars, num_vars);
	initParameters();
	customizeGUI();
	update( INIT );
	DefaultGUIModel::refresh();
	QTimer::singleShot(0, this, SLOT(resizeMe()));
};

AMAmp::~AMAmp(void) {};

void AMAmp::initParameters(void) {
	input_channel = 0;
	output_channel = 0;
	amp_mode = 2;
	ai_offset = 0; 
	ao_offset = 0;
	channel = NA; 
	probe_gain = LOW;
	probe_gain_factor = 1;

	zero_offset = 0;
	data_acquired = true;
	signal_count = 0;

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

void AMAmp::execute(void) {
	if (!data_acquired) {
		switch (channel) {
		case AI:
//			zero_signal.push(input(0));
			zero_signal.push(0.0); // scaling input channels is broken as of now...
			signal_count++;
			break;
		case AO:
			zero_signal.push(input(1));
			signal_count++;
			break;
		case NA:
			std::cout<<"ERROR: NA case"<<std::endl;
			signal_count++;
			break;
		default:
			std::cout<<"ERROR: default case"<<std::endl;
			signal_count++;
			break;
		}
	
		if (signal_count > 5000) { // 5000 points is completely arbitrary
			signal_count = 0;
			data_acquired=true;
		}
	}
}

void AMAmp::update(DefaultGUIModel::update_flags_t flag) {

	switch(flag) {
		// initialize the parameters and then the GUI. 
		case INIT:
			setParameter("Input Channel", input_channel);
			setParameter("Output Channel", output_channel);
			setParameter("Amplifier Mode", amp_mode);
			setParameter("Probe Gain", probe_gain);
			setParameter("AI Offset", ai_offset);
			setParameter("AO Offset", ao_offset);

			inputBox->setValue(input_channel);
			outputBox->setValue(output_channel);
			ampButtonGroup->button(amp_mode)->setStyleSheet("QRadioButton { font: bold;}");
			ampButtonGroup->button(amp_mode)->setChecked(true);
			
			probeGainComboBox->setCurrentIndex(probe_gain);
			probeGainComboBox->blacken();

			aiOffsetEdit->setText(QString::number(ai_offset));
			aoOffsetEdit->setText(QString::number(ao_offset));
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
			
			probe_gain = static_cast<probe_gain_t>(getParameter("Probe Gain").toInt());
			std::cout<<probe_gain<<std::endl;
			switch (probe_gain) {
				case HIGH:
					probe_gain_factor = 1; //.10;
					std::cout<<"Probe Gain HIGH - "<<probe_gain_factor<<std::endl;
					break;

				case LOW:
					probe_gain_factor = 10; //1;
					std::cout<<"Probe Gain LOW - "<<probe_gain_factor<<std::endl;
					break;

				default:
					std::cout<<"ERROR: default called for probe_gain in update(MODIFY)"<<std::endl;
					break;
			}

			ai_offset = getParameter("AI Offset").toDouble();
			ao_offset = getParameter("AO Offset").toDouble();

			updateDAQ();

			// blacken the GUI to reflect that changes have been saved to variables.
			inputBox->blacken();
			outputBox->blacken();
			aiOffsetEdit->blacken();
			aoOffsetEdit->blacken();
			probeGainComboBox->blacken();

			// Disable findZeroButton if not currently I=0 mode
			if (amp_mode == 2) findZeroButton->setEnabled(true);
			else findZeroButton->setEnabled(false);
			break;

		default:
			break;
	}
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

	switch(amp_mode) {
		case 1: // VClamp
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 0);
				device->setAnalogGain(DAQ::AI, input_channel, vclamp_ai_gain);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, vclamp_ao_gain);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 5.0;
			output(2) = 0.0;
			break;

		case 2: // I = 0
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, izero_ai_gain);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, izero_ao_gain*probe_gain_factor);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 5.0;
			output(1) = 5.0;
			output(2) = 0.0;
			break;

		case 3: // IClamp
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain);
//				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain*probe_gain_factor);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain);
//				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain*probe_gain_factor);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 0.0;
			output(2) = 5.0;
			break;

		case 4: // VComp
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 0);
				device->setAnalogGain(DAQ::AI, input_channel, vclamp_ai_gain);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, vclamp_ao_gain);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 5.0;
			output(1) = 0.0;
			output(2) = 0.0;
			break;

		case 5: // VTest
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 0);
				device->setAnalogGain(DAQ::AI, input_channel, vclamp_ai_gain);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, vclamp_ao_gain);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 0.0;
			output(1) = 0.0;
			output(2) = 0.0;
			break;

		case 6: // IResist
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain*probe_gain_factor);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
			}

			output(0) = 5.0;
			output(1) = 0.0;
			output(2) = 5.0;
			break;

		case 7: // IFollow
			if(device) {
				device->setAnalogRange(DAQ::AI, input_channel, 3);
				device->setAnalogGain(DAQ::AI, input_channel, iclamp_ai_gain*probe_gain_factor);
				device->setAnalogZeroOffset(DAQ::AI, input_channel, ai_offset);
				//device->setAnalogCalibration(DAQ::AI, input_channel);
				device->setAnalogGain(DAQ::AO, output_channel, iclamp_ao_gain);
				device->setAnalogZeroOffset(DAQ::AO, output_channel, ao_offset);
				//device->setAnalogCalibration(DAQ::AO, output_channel);
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

void AMAmp::setAIOffset(const QString &text) {
	parameter["AI Offset"].edit->setText(text);
	parameter["AI Offset"].edit->setModified(true);
}

void AMAmp::setAOOffset(const QString &text) {
	parameter["AO Offset"].edit->setText(text);
	parameter["AO Offset"].edit->setModified(true);
}

void AMAmp::updateOffset(int new_mode) {
	double scaled_ai_offset = ai_offset;
	double scaled_ao_offset = ao_offset;

	switch(amp_mode) {
	case 1: // VClamp
		scaled_ai_offset = scaled_ai_offset * vclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset * vclamp_ao_gain;
		break;

	case 2: // I = 0
		scaled_ai_offset = scaled_ai_offset * izero_ai_gain;
		scaled_ao_offset = scaled_ao_offset * izero_ao_gain;
		
		findZeroButton->setEnabled(true);
		break;

	case 3: // IClamp
		scaled_ai_offset = scaled_ai_offset * iclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset * iclamp_ao_gain;
		break;

	case 4: // VComp
		scaled_ai_offset = scaled_ai_offset * vclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset * vclamp_ao_gain;
		break;

	case 5: // VTest
		scaled_ai_offset = scaled_ai_offset * vclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset * vclamp_ao_gain;
		break;

	case 6: // IResist
		scaled_ai_offset = scaled_ai_offset * iclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset * iclamp_ao_gain;
		break;

	case 7: // IFollow
		scaled_ai_offset = scaled_ai_offset * iclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset * iclamp_ao_gain;
		break;

	default:
		std::cout<<"ERROR. Something went horribly wrong.\n The amplifier mode is set to an unknown value"<<std::endl;
		break;
	}

	switch(new_mode) {
	case 1: // VClamp
		scaled_ai_offset = scaled_ai_offset / vclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset / vclamp_ao_gain;

		aiOffsetUnits->setText("1 mV/pA");
		aoOffsetUnits->setText("20 mV/V");
		
		findZeroButton->setEnabled(false);
		break;

	case 2: // I = 0
		scaled_ai_offset = scaled_ai_offset / izero_ai_gain;
		scaled_ao_offset = scaled_ao_offset / izero_ao_gain;

		aiOffsetUnits->setText("1 V/V");
		aoOffsetUnits->setText("---");
		break;

	case 3: // IClamp
		scaled_ai_offset = scaled_ai_offset / iclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset / iclamp_ao_gain;

		aiOffsetUnits->setText("1 V/V");
		aoOffsetUnits->setText("2 nA/V");
		
		findZeroButton->setEnabled(false);
		break;

	case 4: // VComp
		scaled_ai_offset = scaled_ai_offset / vclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset / vclamp_ao_gain;

		aiOffsetUnits->setText("1 mV/pA");
		aoOffsetUnits->setText("20 mV/V");
		
		findZeroButton->setEnabled(false);
		break;

	case 5: // VTest
		scaled_ai_offset = scaled_ai_offset / vclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset / vclamp_ao_gain;

		aiOffsetUnits->setText("1 mV/pA");
		aoOffsetUnits->setText("20 mV/V");
		
		findZeroButton->setEnabled(false);
		break;

	case 6: // IResist
		scaled_ai_offset = scaled_ai_offset / iclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset / iclamp_ao_gain;

		aiOffsetUnits->setText("1 V/V");
		aoOffsetUnits->setText("2 nA/V");
		
		findZeroButton->setEnabled(false);
		break;

	case 7: // IFollow
		scaled_ai_offset = scaled_ai_offset / iclamp_ai_gain;
		scaled_ao_offset = scaled_ao_offset / iclamp_ao_gain;

		aiOffsetUnits->setText("1 V/V");
		aoOffsetUnits->setText("2 nA/V");
		
		findZeroButton->setEnabled(false);
		break;

	default:
		std::cout<<"ERROR. Something went horribly wrong.\n The amplifier mode is set to an unknown value"<<std::endl;
		break;
	}

	parameter["AI Offset"].edit->setText(QString::number(scaled_ai_offset));
	parameter["AI Offset"].edit->setModified(true);
	parameter["AO Offset"].edit->setText(QString::number(scaled_ao_offset));
	parameter["AO Offset"].edit->setModified(true);
	
	aiOffsetEdit->setText(QString::number(scaled_ai_offset));
	aoOffsetEdit->setText(QString::number(scaled_ao_offset));
}

void AMAmp::findZeroOffset(void) {
	findZeroButton->setEnabled(false);
	iclampButton->setEnabled(false);
	vclampButton->setEnabled(false);
	izeroButton->setEnabled(false);
	vcompButton->setEnabled(false);
	vtestButton->setEnabled(false);
	iresistButton->setEnabled(false);
	ifollowButton->setEnabled(false);
	modifyButton->setEnabled(false);

	data_acquired = false;
	channel = AI;
	checkZeroCalc->start(1000);

	pause(false);
}

void AMAmp::calculateOffset(void) {
	if (data_acquired) {
		checkZeroCalc->stop();
		pause(true);

		switch(channel) {
		case AI: 
			zero_offset = zero_signal.mean();
			std::cout<<"AI Calc. Offst. = "<<zero_offset*izero_ai_gain<<std::endl;
			std::cout<<"AI Curr. Offst. = "<<ai_offset<<std::endl;
			std::cout<<"AI Total Offst. = "<<zero_offset*izero_ai_gain+ai_offset<<std::endl;
/*
			std::cout<<"AI Calc. Offst. = "<<zero_offset<<std::endl;
			std::cout<<"AI Curr. Offst. = "<<ai_offset<<std::endl;
			std::cout<<"AI Total Offst. = "<<ai_offset+zero_offset<<std::endl;
*/
			zero_signal.clear();

			aiOffsetEdit->setText(QString::number(zero_offset*izero_ai_gain+ai_offset));
//			aiOffsetEdit->setText(QString::number(zero_offset+ai_offset));
			aiOffsetEdit->redden();
			parameter["AI Offset"].edit->setText(QString::number(zero_offset*izero_ai_gain+ai_offset));
//			parameter["AI Offset"].edit->setText(QString::number(ai_offset+zero_offset));
			parameter["AI Offset"].edit->setModified(true);

//			modify();

			channel = AO;
			data_acquired = false;
			zero_offset = 0;
			checkZeroCalc->start();
			pause(false);
			break;

		case AO:
			zero_offset = zero_signal.mean();
			std::cout<<"AO Calc. Offst. = "<<zero_offset<<std::endl;
			std::cout<<"AO Curr. Offst. = "<<ao_offset<<std::endl;
			std::cout<<"AO Total Offst. = "<<ao_offset+zero_offset<<std::endl<<std::endl;
			zero_signal.clear();

			aoOffsetEdit->setText(QString::number(ao_offset+zero_offset));
			aoOffsetEdit->redden();
			parameter["AO Offset"].edit->setText(QString::number(zero_offset+ao_offset));
			parameter["AO Offset"].edit->setModified(true);

//			modify();

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
			std::cout<<"Case NA isn't supposed to be called."<<std::endl;
			break;

		default:
			std::cout<<"ERROR: default case called."<<std::endl;
			break;
		}

	} else {
		std::cout<<"Not done yet... "<<signal_count<<std::endl;
		return;
	}
}

void AMAmp::setProbeGain(int mode) {
	switch (mode) {
		case LOW: 
			std::cout<<"Low and go."<<std::endl;
//			probe_gain = LOW;
			parameter["Probe Gain"].edit->setText(QString::number(mode));
			parameter["Probe Gain"].edit->setModified(true);
			break;
		case HIGH: 
			std::cout<<"High and why."<<std::endl;
//			probe_gain = HIGH;
			parameter["Probe Gain"].edit->setText(QString::number(mode));
			parameter["Probe Gain"].edit->setModified(true);
			break;
		default:
			std::cout<<"ERROR: default case called in AMAmp::setProbeGain()"<<std::endl;
			break;
	}
}

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
	
	customLayout->itemAtPosition(1,0)->widget()->setVisible(false);
	DefaultGUIModel::pauseButton->setVisible(false);
	DefaultGUIModel::modifyButton->setText("Set DAQ");
	DefaultGUIModel::unloadButton->setVisible(false);

	// create input spinboxes
	QGroupBox *ioGroupBox = new QGroupBox("Channels");
	QGridLayout *ioGroupLayout = new QGridLayout;
	ioGroupBox->setLayout(ioGroupLayout);

	QLabel *inputBoxLabel = new QLabel("Input");
	inputBox = new AMAmpSpinBox; // this is the QSpinBox wrapper made earlier
	inputBox->setRange(0,100);
	ioGroupLayout->addWidget(inputBoxLabel, 0, 0);
	ioGroupLayout->addWidget(inputBox, 0, 1);

	QLabel *outputBoxLabel = new QLabel("Output");
	outputBox = new AMAmpSpinBox;
	outputBox->setRange(0,100);
	ioGroupLayout->addWidget(outputBoxLabel, 1, 0);
	ioGroupLayout->addWidget(outputBox, 1, 1);

	// create amp mode groupbox
	QGroupBox *ampModeGroupBox = new QGroupBox("Amp Mode");
	QGridLayout *ampModeGroupLayout = new QGridLayout;
	ampModeGroupBox->setLayout(ampModeGroupLayout);

	QGridLayout *offsetLayout = new QGridLayout;
	offsetLayout->setColumnStretch(0, 1);
	QLabel *aiOffsetLabel = new QLabel("AI Offset:");
	offsetLayout->addWidget(aiOffsetLabel, 0, 0);
	aiOffsetEdit = new AMAmpLineEdit();
	aiOffsetEdit->setMaximumWidth(aiOffsetEdit->minimumSizeHint().width()*3);
	aiOffsetEdit->setValidator( new QDoubleValidator(aiOffsetEdit) );
	offsetLayout->addWidget(aiOffsetEdit, 0, 1);
	aiOffsetUnits = new QLabel("1 V/V");
	offsetLayout->addWidget(aiOffsetUnits, 0, 2, Qt::AlignCenter);
	
	QLabel *aoOffsetLabel = new QLabel("AO Offset:");
	offsetLayout->addWidget(aoOffsetLabel, 1, 0);
	aoOffsetEdit = new AMAmpLineEdit();
	aoOffsetEdit->setMaximumWidth(aoOffsetEdit->minimumSizeHint().width()*3);
	aoOffsetEdit->setValidator( new QDoubleValidator(aoOffsetEdit) );
	offsetLayout->addWidget(aoOffsetEdit, 1, 1);
	aoOffsetUnits = new QLabel("---");
	offsetLayout->addWidget(aoOffsetUnits, 1, 2, Qt::AlignCenter);

	findZeroButton = new QPushButton("Zero AI/AO Channels");
	findZeroButton->setToolTip("Find offset that will make output from specified channel = 0");
	offsetLayout->addWidget(findZeroButton, 2, 0, 1, 3);

	ampModeGroupLayout->addLayout(offsetLayout, 0, 0);

	//add little bit of space betwen offsets and buttons
	ampModeGroupLayout->addItem( new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum), 2, 0);

	// add probe gain combobox
	QGridLayout *probeGainLayout = new QGridLayout;
	QLabel *probeGainLabel = new QLabel("Probe Gain:");
	probeGainComboBox = new AMAmpComboBox;
	probeGainComboBox->insertItem(0, QString::fromUtf8("Low (10 MΩ)"));
	probeGainComboBox->insertItem(1, QString::fromUtf8("High (10 GΩ)"));
	probeGainLayout->addWidget(probeGainLabel, 0, 0);
	probeGainLayout->addWidget(probeGainComboBox, 0, 1);
	ampModeGroupLayout->addLayout(probeGainLayout, 3, 0);

	//add little bit of space betwen offsets and buttons
	ampModeGroupLayout->addItem( new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum), 4, 0);

	// make the buttons
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

	QGridLayout *ampButtonGroupLayout = new QGridLayout;
	ampButtonGroupLayout->addWidget(vclampButton, 0, 0);
	ampButtonGroupLayout->addWidget(izeroButton, 0, 1);
	ampButtonGroupLayout->addWidget(iclampButton, 1, 0);
	ampButtonGroupLayout->addWidget(vcompButton, 1, 1);
	ampButtonGroupLayout->addWidget(vtestButton, 2, 0);
	ampButtonGroupLayout->addWidget(iresistButton, 2, 1);
	ampButtonGroupLayout->addWidget(ifollowButton, 3, 0);

	ampModeGroupLayout->addLayout(ampButtonGroupLayout, 5, 0);

	// add widgets to custom layout
	customLayout->addWidget(ioGroupBox, 0, 0);
	customLayout->addWidget(ampModeGroupBox, 2, 0);
	setLayout(customLayout);

	checkZeroCalc = new QTimer(this);

	// connect the widgets to the signals
	QObject::connect(ampButtonGroup, SIGNAL(buttonPressed(int)), this, SLOT(updateMode(int)));
	QObject::connect(ampButtonGroup, SIGNAL(buttonPressed(int)), this, SLOT(updateOffset(int)));
	QObject::connect(inputBox, SIGNAL(valueChanged(int)), this, SLOT(updateInputChannel(int)));
	QObject::connect(outputBox, SIGNAL(valueChanged(int)), this, SLOT(updateOutputChannel(int)));
	QObject::connect(aiOffsetEdit, SIGNAL(textEdited(const QString &)), this, SLOT(setAIOffset(const QString &)));
	QObject::connect(aoOffsetEdit, SIGNAL(textEdited(const QString &)), this, SLOT(setAOOffset(const QString &)));
	QObject::connect(findZeroButton, SIGNAL(clicked(void)), this, SLOT(findZeroOffset(void)));
	QObject::connect(checkZeroCalc, SIGNAL(timeout(void)), this, SLOT(calculateOffset(void)));
	QObject::connect(probeGainComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProbeGain(int)));
}

void AMAmp::doSave(Settings::Object::State &s) const {
	s.saveInteger("paused", pauseButton->isChecked());
	if (isMaximized()) s.saveInteger("Maximized", 1);
	else if (isMinimized()) s.saveInteger("Minimized", 1);

	QPoint pos = parentWidget()->pos();
	s.saveInteger("X", pos.x());
	s.saveInteger("Y", pos.y());
	s.saveInteger("W", width());
	s.saveInteger("H", height());

	for (std::map<QString, param_t>::const_iterator i = parameter.begin(); i != parameter.end(); ++i) {
		s.saveString((i->first).toStdString(), (i->second.edit->text()).toStdString());
	}
}

void AMAmp::doLoad(const Settings::Object::State &s) {
	for (std::map<QString, param_t>::iterator i = parameter.begin(); i != parameter.end(); ++i)
		i->second.edit->setText(QString::fromStdString(s.loadString((i->first).toStdString())));

	if (s.loadInteger("Maximized")) showMaximized();
	else if (s.loadInteger("Minimized")) showMinimized();

	// this only exists in RTXI versions >1.3
	if (s.loadInteger("W") != NULL) {
		resize(s.loadInteger("W"), s.loadInteger("H"));
		parentWidget()->move(s.loadInteger("X"), s.loadInteger("Y"));
	}

	pauseButton->setChecked(s.loadInteger("paused"));
	modify();

	inputBox->setValue(input_channel);
	outputBox->setValue(output_channel);
	ampButtonGroup->button(amp_mode)->setStyleSheet("QRadioButton { font: bold;}");
	ampButtonGroup->button(amp_mode)->setChecked(true);
	probeGainComboBox->setCurrentIndex(probe_gain);
	aiOffsetEdit->setText(QString::number(ai_offset));
	aoOffsetEdit->setText(QString::number(ao_offset));
}
