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

#include <daq.h>
#include <default_gui_model.h>
#include <runningstat.h>

class AMAmpComboBox : public QComboBox {

	Q_OBJECT

	public:
		AMAmpComboBox(QWidget * =0);
		~AMAmpComboBox(void);
		void blacken(void);

	public slots:
		void redden(void);
};

class AMAmpLineEdit : public QLineEdit {

	Q_OBJECT

	public:
		AMAmpLineEdit(QWidget * =0);
		~AMAmpLineEdit(void);
		void blacken(void);

	public slots:
		void redden(void);
};

class AMAmpSpinBox : public QSpinBox {

	Q_OBJECT

	public:
		AMAmpSpinBox(QWidget * =0);
		~AMAmpSpinBox(void);
		void blacken(void);

	public slots:
		void redden(void);
};


class AMAmp : public DefaultGUIModel {
	
	Q_OBJECT
	
	public:
		AMAmp(void);
		virtual ~AMAmp(void);
	
		void initParameters(void);
		void customizeGUI(void);
		void updateDAQ(void);
		void updateGUI(void);
		void execute(void);
	
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);

	private:
		friend class AMAmpLineEdit;
		friend class AMAmpComboBox;
		friend class AMAmpSpinBox;

		double iclamp_ai_gain; // (1 V / V)
		double iclamp_ao_gain; // (2 nA / V) ...hmm
		double izero_ai_gain; // (1 V / V)
		double izero_ao_gain; // No output
		double vclamp_ai_gain; // 1 mV / pA
		double vclamp_ao_gain; // 20 mV / V
		double probe_gain_factor;

		double ai_offset, ao_offset;

		enum channel_t {AI, AO, NA } channel;
		enum probe_gain_t {LOW, HIGH } probe_gain;

		RunningStat zero_signal;
		int signal_count;
		bool data_acquired;
		double zero_offset;

		int input_channel, output_channel;
		int amp_mode, temp_mode;

		DAQ::Device *device;
	
		QRadioButton *iclampButton, *vclampButton, *izeroButton, *vcompButton, 
		             *vtestButton, *iresistButton, *ifollowButton;
		QButtonGroup *ampButtonGroup;
		AMAmpSpinBox *inputBox, *outputBox;
		AMAmpLineEdit *aiOffsetEdit, *aoOffsetEdit;
		AMAmpComboBox *probeGainComboBox;
		QLabel *aiOffsetUnits, *aoOffsetUnits;
		QPushButton *findZeroButton;
		QTimer *checkZeroCalc;

		void doSave(Settings::Object::State &) const;
		void doLoad(const Settings::Object::State &);

	private slots:
		void setAIOffset(const QString &);
		void setAOOffset(const QString &);
		void updateOffset(int);
		void updateMode(int);
		void updateInputChannel(int);
		void updateOutputChannel(int);
		void findZeroOffset(void);
		void calculateOffset(void);
		void setProbeGain(int);
};
