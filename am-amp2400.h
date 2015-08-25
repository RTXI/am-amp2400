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

class AMAmpLineEdit : public QLineEdit {

	Q_OBJECT

	public:
		AMAmpLineEdit(QWidget * =0);
		~AMAmpLineEdit(void);
		void blacken(void);

	public slots:
		void redden(void);
};

/*
class AMAmpComboBox : public QComboBox {

	Q_OBJECT

	public:
		AMAmpComboBox(QWidget * =0);
		~AMAmpComboBox(void);
		void blacken(void);

	public slots:
		void redden(void);
};
*/

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
	
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);

	private:
		double iclamp_ai_gain; // (1 V / V)
		double iclamp_ao_gain; // (2 nA / V) ...hmm
		double izero_ai_gain; // (1 V / V)
		double izero_ao_gain; // No output
		double vclamp_ai_gain; // 1 mV / pA
		double vclamp_ao_gain; // 20 mV / V

		double ai_offset, ao_offset;

/*
		double iclamp_offset, vclamp_offset, izero_offset, vcomp_offset,
		       vtest_offset, iresist_offset, ifollow_offset;
*/

		int input_channel, output_channel;
		int amp_mode, temp_mode;

		DAQ::Device *device;
	
		QRadioButton *iclampButton, *vclampButton, *izeroButton, *vcompButton, 
		             *vtestButton, *iresistButton, *ifollowButton;
/*
		QLineEdit *iclampEdit, *vclampEdit, *izeroEdit, *vcompEdit,
		          *vtestEdit, *iresistEdit, *ifollowEdit;
*/
		QButtonGroup *ampButtonGroup;
		AMAmpSpinBox *inputBox, *outputBox;
//		AMAmpComboBox *headstageBox, *outputGainBox;
		AMAmpLineEdit *aiOffsetEdit, *aoOffsetEdit;
		QLabel *aiOffsetUnits, *aoOffsetUnits;

		void doSave(Settings::Object::State &) const;
		void doLoad(const Settings::Object::State &);

	private slots:
		void setAIOffset(const QString &);
		void setAOOffset(const QString &);
		void updateOffset(int);
		void updateMode(int);
		void updateInputChannel(int);
		void updateOutputChannel(int);
};
