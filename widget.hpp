
#include <QComboBox>
#include <QRadioButton>
#include <QSpinBox>
#include <string>

#include <rtxi/math/runningstat.h>
#include <rtxi/widgets.hpp>

namespace DAQ
{
class Device;
}

// This is an generated header file. You may change the namespace, but
// make sure to do the same in implementation (.cpp) file
namespace am_amp2400
{

constexpr std::string_view MODULE_NAME = "am-amp2400";

inline std::vector<Widgets::Variable::Info> get_default_vars()
{
  return {};
}

inline std::vector<IO::channel_t> get_default_channels()
{
  return {};
}

enum probe_gain_t : std::uint8_t
{
  LOW = 0,
  HIGH
};

class AMAmpComboBox : public QComboBox
{
  Q_OBJECT
public:
  AMAmpComboBox(QWidget* = nullptr);
  AMAmpComboBox(const AMAmpComboBox&) = delete;
  AMAmpComboBox(AMAmpComboBox&&) = delete;
  AMAmpComboBox& operator=(const AMAmpComboBox&) = delete;
  AMAmpComboBox& operator=(AMAmpComboBox&&) = delete;
  ~AMAmpComboBox() override = default;
  void blacken();

public slots:
  void redden();
};

class AMAmpLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  AMAmpLineEdit(QWidget* = nullptr);
  AMAmpLineEdit(const AMAmpLineEdit&) = delete;
  AMAmpLineEdit(AMAmpLineEdit&&) = delete;
  AMAmpLineEdit& operator=(const AMAmpLineEdit&) = delete;
  AMAmpLineEdit& operator=(AMAmpLineEdit&&) = delete;
  ~AMAmpLineEdit() override = default;
  void blacken();

public slots:
  void redden();
};

class AMAmpSpinBox : public QSpinBox
{
  Q_OBJECT

public:
  AMAmpSpinBox(QWidget* = nullptr);
  AMAmpSpinBox(const AMAmpSpinBox&) = delete;
  AMAmpSpinBox(AMAmpSpinBox&&) = delete;
  AMAmpSpinBox& operator=(const AMAmpSpinBox&) = delete;
  AMAmpSpinBox& operator=(AMAmpSpinBox&&) = delete;
  ~AMAmpSpinBox() override = default;
  void blacken();

public slots:
  void redden();
};

class Panel : public Widgets::Panel
{
  Q_OBJECT
public:
  Panel(QMainWindow* main_window, Event::Manager* ev_manager);

public slots:
  void modify() override;

private slots:
  void setAIOffset(const QString&);
  void setAOOffset(const QString&);
  void updateOffset(int);
  void updateMode(int);
  void updateInputChannel(int);
  void updateOutputChannel(int);
  void setProbeGain(int index);

private:
  enum amp_mode : int8_t
  {
    VCLAMP = 0,
    IEQ0,
    ICLAMP,
    VCOMP,
    VTEST,
    IRESIST,
    IFOLLOW,
    UNKNOWN
  };
  void customizeGUI();
  void updateDAQ();
  void initParameters();
  DAQ::Device* current_device = nullptr;
  RunningStat* zero_signal_ptr = nullptr;

  QRadioButton* iclampButton = nullptr;
  QRadioButton* vclampButton = nullptr;
  QRadioButton* izeroButton = nullptr;
  QRadioButton* vcompButton = nullptr;
  QRadioButton* vtestButton = nullptr;
  QRadioButton* iresistButton = nullptr;
  QRadioButton* ifollowButton = nullptr;
  QButtonGroup* ampButtonGroup = nullptr;
  AMAmpSpinBox* inputBox = nullptr;
  AMAmpSpinBox* outputBox = nullptr;
  AMAmpSpinBox* bit1Box = nullptr;
  AMAmpSpinBox* bit2Box = nullptr;
  AMAmpSpinBox* bit4Box = nullptr;
  AMAmpLineEdit* aiOffsetEdit = nullptr;
  AMAmpLineEdit* aoOffsetEdit = nullptr;
  AMAmpComboBox* probeGainComboBox = nullptr;
  QLabel* aiOffsetUnits = nullptr;
  QLabel* aoOffsetUnits = nullptr;

  // Important parameters
  static constexpr double iclamp_ai_gain = 1.0;  // (1 V / V)
  static constexpr double iclamp_ao_gain = 1.0;  // (2 nA / V) ...hmm
  static constexpr double izero_ai_gain = 200e-3;  // (1 V / V)
  static constexpr double izero_ao_gain = 1;  // No output
  static constexpr double vclamp_ai_gain = 2e-9;  // 1 mV / pA
  static constexpr double vclamp_ao_gain = 50;  // 50 mV / V
  int input_channel = 0;
  int output_channel = 0;
  amp_mode mode = IEQ0;
  int digital_line_0 = 0;
  int digital_line_1 = 0;
  int digital_line_2 = 0;
  double probe_gain_factor;
  double ai_offset = 0;
  double ao_offset = 0;
  int signal_count = 0;
  double zero_offset = 0;
};

class Plugin : public Widgets::Plugin
{
public:
  explicit Plugin(Event::Manager* ev_manager);
};

}  // namespace am_amp2400
