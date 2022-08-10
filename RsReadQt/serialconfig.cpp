#include "serialconfig.h"
#include <QtGlobal>
#include <type_traits>
#include <qsettings.h>
#include "global_configs.hpp"
#include <qfile.h>
#include <qfiledialog.h>
#include <qtoolbutton.h>
#include <qjsondocument.h>
#include <qjsonobject.h>

SerialConfig::SerialConfig(std::shared_ptr<void> &databridgeConfigs, QWidget *parent)
  : serialConfigs{ std::add_lvalue_reference_t<std::shared_ptr<SerialConfigs>>(databridgeConfigs) }
  , QDialog(parent)
{
    setupUi(this);

    if (serialConfigs.get() == nullptr) {
        serialConfigs = std::make_shared<SerialConfigs>();
    }

    connect(dropdownPORT, &QComboBox::currentTextChanged, this, &SerialConfig::OnSelportSelected);
    connect(dropdownBAUDRATE, &QComboBox::currentTextChanged, this, &SerialConfig::OnSelbaudrateSelected);
    connect(dropdownDATABITS, &QComboBox::currentTextChanged, this, &SerialConfig::OnSeldatabitsSelected);
    connect(dropdownSTOPBITS, &QComboBox::currentTextChanged, this, &SerialConfig::OnSelstopbitsSelected);
    connect(dropdownPARITY, &QComboBox::currentTextChanged, this, &SerialConfig::OnSelparitySelected);
    connect(buttonAPPLY, &QPushButton::clicked, this, &SerialConfig::OnApplyClick);
    connect(spinboxMsgLen, &QSpinBox::valueChanged, this, &SerialConfig::OnMsgLenValueChanged);
    connect(selectFileButton, &QToolButton::clicked, this, &SerialConfig::OnSpecifyFileWithSettings);
    connect(selectFileButton2, &QToolButton::clicked, this, &SerialConfig::OnSpecifyFileWithSettings2);

    Init();
}

SerialConfig::~SerialConfig() { }

void
SerialConfig::SaveSettingsToSystem()
{
    QSettings settings;
    // TODO: add all settings
    settings.beginGroup("SerialConfigs");
    {
        settings.setValue("Port", localSelections["Port"].first);
        settings.setValue("ConfigsFile", serialConfigs->GetConfigsFileName());
    }
    settings.endGroup();
}

void
SerialConfig::RetreiveSettingsFromSystem()
{
    QSettings settings;
    settings.beginGroup(SERIAL_SYSTEMCONFIGS_NAME);

    // TODO: add all settings
    if (settings.contains("Port")) {
        auto avlblPorts = serialConfigs->GetAvlblComPorts();

        if (avlblPorts.ContainsKey(settings.value("Port").toInt())) {
            assert(avlblPorts.ConvToPairByKeyIfAny(settings.value("Port").toInt(), localSelections["Port"]));
            // assert(avlblPorts.ConvToPairByKeyIfAny(5, localSelections["Port"]));
        }
        else {
            localSelections["Port"] = SerialConfigs::PORT_UNDEFINED;
        }
    }
    else {
        auto avlblPorts2 = serialConfigs->GetAvlblComPorts();
        if (!serialConfigs->PreviouslySelectedPortIsPresent()) {
            serialConfigs->SetPortToUndefined();
        }

        localSelections["Port"] = serialConfigs->GetSelectedPort();
    }

    if (settings.contains("ConfigsFile")) {
        serialConfigs->SetConfigsFileName(settings.value("ConfigsFile").toString());
    }

    localSelections["Baudrate"]     = serialConfigs->GetSelectedBaudrate();
    localSelections["Databits"]     = serialConfigs->GetSelectedDatabits();
    localSelections["Stopbits"]     = serialConfigs->GetSelectedStopbits();
    localSelections["Parity"]       = serialConfigs->GetSelectedParity();
    localSelections["DeviceMsgLen"] = { serialConfigs->GetSelectedMsgLen(),
                                        QString::number(serialConfigs->GetSelectedMsgLen()) };

    settings.endGroup();
}

void
SerialConfig::Init()
{
    serialConfigs->UpdateAvlblPorts();

    PutValuesToDropDown(dropdownPORT, serialConfigs->GetAvlblComPorts());
    PutValuesToDropDown(dropdownBAUDRATE, serialConfigs->GetAvlblBaudrates());
    PutValuesToDropDown(dropdownDATABITS, serialConfigs->GetAvlblDatabits());
    PutValuesToDropDown(dropdownSTOPBITS, serialConfigs->GetAvlblStopbits());
    PutValuesToDropDown(dropdownPARITY, serialConfigs->GetAvlblParities());

    // these should be before data retreival because of event generated when spinbox data changes see OnMsgLenValueChanged
    spinboxMsgLen->setMinimum(SerialConfigs::avlblMsgLen.first);
    spinboxMsgLen->setMaximum(SerialConfigs::avlblMsgLen.second);

    RetreiveSettingsFromSystem();

    dropdownPORT->setCurrentText(localSelections["Port"].second);
    dropdownBAUDRATE->setCurrentText(localSelections["Baudrate"].second);
    dropdownDATABITS->setCurrentText(localSelections["Databits"].second);
    dropdownSTOPBITS->setCurrentText(localSelections["Stopbits"].second);
    dropdownPARITY->setCurrentText(localSelections["Parity"].second);

    spinboxMsgLen->setValue(localSelections["DeviceMsgLen"].first);
}

void
SerialConfig::OnSelportSelected(const QString &newSelection)
{
    if (newSelection.isEmpty())
        return;

    if (newSelection == localSelections["Port"].second)
        return;

    assert_msg(serialConfigs->GetAvlblComPorts().ConvToPairByValueIfAny(newSelection, localSelections["Port"]),
               "Error while saving selection to configs container");
}

void
SerialConfig::OnSelbaudrateSelected(const QString &newSelection)
{
    assert_msg(serialConfigs->GetAvlblBaudrates().ConvToPairByValueIfAny(newSelection, localSelections["Baudrate"]),
               "Error while saving selection to configs container");
}

void
SerialConfig::OnSeldatabitsSelected(const QString &newSelection)
{
    assert_msg(serialConfigs->GetAvlblDatabits().ConvToPairByValueIfAny(newSelection, localSelections["Databits"]),
               "Error while saving selection to configs container");
}

void
SerialConfig::OnSelstopbitsSelected(const QString &newSelection)
{
    assert_msg(serialConfigs->GetAvlblStopbits().ConvToPairByValueIfAny(newSelection, localSelections["Stopbits"]),
               "Error while saving selection to configs container");
}

void
SerialConfig::OnSelparitySelected(const QString &newSelection)
{
    assert_msg(serialConfigs->GetAvlblParities().ConvToPairByValueIfAny(newSelection, localSelections["Parity"]),
               "Error while saving selection to configs container");
}

void
SerialConfig::OnApplyClick()
{
    assert_msg(serialConfigs->SetPortByName(localSelections["Port"].second), "Error occured while saving configuration: Port");

    assert_msg(serialConfigs->SetBaudrateByValue(localSelections["Baudrate"].second),
               "Error occured while saving configuration: Baudrate");

    assert_msg(serialConfigs->SetDatabitsByValue(localSelections["Databits"].second),
               "Error occured while saving configuration: Databits");

    assert_msg(serialConfigs->SetStopbitsByValue(localSelections["Stopbits"].second),
               "Error occured while saving configuration: Stopbits");

    assert_msg(serialConfigs->SetParityByValue(localSelections["Parity"].second),
               "Error occured while saving configuration: Parity");

    assert_msg(serialConfigs->SetMsgLen(spinboxMsgLen->value()), "bytes in msg value are not in limit (1 - 8)");

    SaveSettingsToSystem();

    this->done(0);
}

void
SerialConfig::OnCancelClick()
{ }

void
SerialConfig::OnMsgLenValueChanged(int value)
{
    localSelections["DeviceMsgLen"] = { value, QString::number(value) };
}

void
SerialConfig::OnSpecifyFileWithSettings()
{
    serialConfigs->SetConfigsFileName(QFileDialog::getOpenFileName(this, tr("Open configurations file"), "", tr("*.json")));

    // TEST
}

void
SerialConfig::OnSpecifyFileWithSettings2()
{
    serialConfigs->SetConfigsFileName2(QFileDialog::getOpenFileName(this, tr("Open configurations file"), "", tr("*.json")));
}