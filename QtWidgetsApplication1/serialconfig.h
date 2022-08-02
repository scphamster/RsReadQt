#pragma once

#include <QDialog>
#include "ui_serialconfig.h"
#include "Serial.h"

template<typename ConfigsT>
class Configurator {
  public:
    // void PutValuesToDropDown(QComboBox &dropDown, const ConfigsT &values)
    //{
    //     std::for_each(values.begin(), values.end(), [&dropDown](const auto &pair) { dropDown.addItem(pair.second);
    //     });
    // }

    void PutValuesToDropDown(QComboBox *dropDown, const ConfigsT &values)
    {
        std::for_each(values.begin(), values.end(), [&dropDown](const auto &pair) { dropDown->addItem(pair.second); });
    }
};

class SerialConfig : public QDialog,
                     public Ui::SerialConfigClass,
                     private SerialConfigs,
                     Configurator<SerialConfigs::configs_container_t> {
    Q_OBJECT

  public:
    SerialConfig(std::shared_ptr<void> &databridgeConfigs, QWidget *parent = nullptr);
    ~SerialConfig();

    void RetreiveSettingsFromSystem();
    void SaveSettingsToSystem();

  private slots:

    void OnSelportSelected(const QString &newSelection);
    void OnSelbaudrateSelected(const QString &newSelection);
    void OnSeldatabitsSelected(const QString &newSelection);
    void OnSelstopbitsSelected(const QString &newSelection);
    void OnSelparitySelected(const QString &newSelection);
    void OnMsgLenValueChanged(int value);

    void OnApplyClick();
    void OnCancelClick();

    void Init();

  private:
    using configs_container_t = superMap<const std::string, single_config_t>;

    std::shared_ptr<_SerialConfigs<int, QString>> &serialConfigs;
    configs_container_t                            localSelections{ { "Port", PORT_UNDEFINED },
                                         { "Baudrate", { 115200, _T("115200" ) } },
                                         { "Databits", { SERIAL_DATABITS_8, _T("8" ) } },
                                         { "Stopbits", { SERIAL_STOPBITS_1, _T("1" ) } },
                                         { "Parity", { SERIAL_PARITY_NONE, _T("NONE" ) } },
                                         { "DeviceMsgLen", { 4, _T( "4" ) } } };
    };
