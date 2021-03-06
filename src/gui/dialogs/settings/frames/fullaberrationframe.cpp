#include "fullaberrationframe.h"
#include "ui_fullaberrationframe.h"

#include <QtGui/QRegExpValidator>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <utility>
#include <utilities/stringutils.h>
#include <dialogs/settings/settingsdialog.h>
#include <utils/stringutils.h>
#include <controls/editunitsbox.h>

#include "utilities/commonstructs.h"

FullAberrationFrame::FullAberrationFrame(QWidget *parent, std::shared_ptr<SimulationManager> params) :
    QWidget(parent),
    ui(new Ui::FullAberrationFrame)
{
    ui->setupUi(this);

    Manager = std::move(params);

    connect(ui->edtConAp, &QLineEdit::textChanged, this, &FullAberrationFrame::checkEditZero);
    connect(ui->edtObjAp, &QLineEdit::textChanged, this, &FullAberrationFrame::checkEditZero);
    connect(ui->edtVoltage, &QLineEdit::textChanged, this, &FullAberrationFrame::checkEditZero);

    setValidators();
    setBackgroundStyles();
    setUnits();
    setValues();

    auto parent_dlg = dynamic_cast<AberrationsDialog*>(parentWidget());
    connect(parent_dlg, &AberrationsDialog::okSignal, this, &FullAberrationFrame::dlgOk_clicked);
    connect(parent_dlg, &AberrationsDialog::cancelSignal, this, &FullAberrationFrame::dlgCancel_clicked);
    connect(parent_dlg, &AberrationsDialog::applySignal, this, &FullAberrationFrame::dlgApply_clicked);
}

void FullAberrationFrame::setValidators()
{
    QRegExpValidator* pValidator = new QRegExpValidator(QRegExp(R"([+]?(\d*(?:\.\d*)?(?:[eE]([+\-]?\d+)?)>)*)"));
    QRegExpValidator* pmValidator = new QRegExpValidator(QRegExp(R"([+-]?(\d*(?:\.\d*)?(?:[eE]([+\-]?\d+)?)>)*)"));

    ui->edtVoltage->setValidator(pValidator);
    ui->edtDefocusSpread->setValidator(pValidator);
    ui->edtConverge->setValidator(pValidator);

    ui->edtBeamTilt->setValidator(pmValidator);
    ui->edtBeamAzimuth->setValidator(pmValidator);

    ui->edtConAp->setValidator(pValidator);
    ui->edtConApSmooth->setValidator(pValidator);

    ui->edtObjAp->setValidator(pValidator);
    ui->edtObjApSmooth->setValidator(pValidator);

    ui->edtC10->setValidator(pmValidator);
    ui->edtC12Mag->setValidator(pmValidator);
    ui->edtC12Ang->setValidator(pmValidator);

    ui->edtC21Mag->setValidator(pmValidator);
    ui->edtC21Ang->setValidator(pmValidator);
    ui->edtC23Mag->setValidator(pmValidator);
    ui->edtC23Ang->setValidator(pmValidator);

    ui->edtC30->setValidator(pmValidator);
    ui->edtC32Mag->setValidator(pmValidator);
    ui->edtC32Ang->setValidator(pmValidator);
    ui->edtC34Mag->setValidator(pmValidator);
    ui->edtC34Ang->setValidator(pmValidator);

    ui->edtC41Mag->setValidator(pmValidator);
    ui->edtC41Ang->setValidator(pmValidator);
    ui->edtC43Mag->setValidator(pmValidator);
    ui->edtC43Ang->setValidator(pmValidator);
    ui->edtC45Mag->setValidator(pmValidator);
    ui->edtC45Ang->setValidator(pmValidator);

    ui->edtC50->setValidator(pmValidator);
    ui->edtC52Mag->setValidator(pmValidator);
    ui->edtC52Ang->setValidator(pmValidator);
    ui->edtC54Mag->setValidator(pmValidator);
    ui->edtC54Ang->setValidator(pmValidator);
    ui->edtC56Mag->setValidator(pmValidator);
    ui->edtC56Ang->setValidator(pmValidator);
}

void FullAberrationFrame::setValues()
{
    auto MicroParams = Manager->microscopeParams();
    // this is fun, right?
    ui->edtVoltage->setText(Utils_Qt::numToQString(MicroParams->Voltage)); // kV

    ui->edtConAp->setText(Utils_Qt::numToQString(MicroParams->CondenserAperture)); // mrad
    ui->edtConApSmooth->setText(Utils_Qt::numToQString(MicroParams->CondenserApertureSmoothing)); // mrad

    ui->edtObjAp->setText(Utils_Qt::numToQString(MicroParams->ObjectiveAperture)); // mrad
    ui->edtObjApSmooth->setText(Utils_Qt::numToQString(MicroParams->ObjectiveApertureSmoothing)); // mrad

    ui->edtDefocusSpread->setText(Utils_Qt::numToQString(MicroParams->Delta / 10)); // nm
    ui->edtConverge->setText(Utils_Qt::numToQString(MicroParams->Alpha)); // mrad

    ui->edtBeamTilt->setText(Utils_Qt::numToQString(MicroParams->BeamTilt)); // mrad
    ui->edtBeamAzimuth->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->BeamAzimuth)); // mrad

    ui->edtC10->setText(Utils_Qt::numToQString(MicroParams->C10 / 10)); // nm
    ui->edtC12Mag->setText(Utils_Qt::numToQString(MicroParams->C12.Mag / 10)); // nm
    ui->edtC12Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C12.Ang)); // degrees

    ui->edtC21Mag->setText(Utils_Qt::numToQString(MicroParams->C21.Mag / 10)); // nm
    ui->edtC21Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C21.Ang)); // degrees
    ui->edtC23Mag->setText(Utils_Qt::numToQString(MicroParams->C23.Mag / 10)); // nm
    ui->edtC23Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C23.Ang)); // degrees

    ui->edtC30->setText(Utils_Qt::numToQString(MicroParams->C30 / 10000)); // um
    ui->edtC32Mag->setText(Utils_Qt::numToQString(MicroParams->C32.Mag / 10000)); // um
    ui->edtC32Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C32.Ang)); // degrees
    ui->edtC34Mag->setText(Utils_Qt::numToQString(MicroParams->C34.Mag / 10000)); // um
    ui->edtC34Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C34.Ang)); // degrees

    ui->edtC41Mag->setText(Utils_Qt::numToQString(MicroParams->C41.Mag / 10000)); // um
    ui->edtC41Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C41.Ang)); // degrees
    ui->edtC43Mag->setText(Utils_Qt::numToQString(MicroParams->C43.Mag / 10000)); // um
    ui->edtC43Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C43.Ang)); // degrees
    ui->edtC45Mag->setText(Utils_Qt::numToQString(MicroParams->C45.Mag / 10000)); // um
    ui->edtC45Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C45.Ang)); // degrees

    ui->edtC50->setText(Utils_Qt::numToQString(MicroParams->C50 / 10000)); // um
    ui->edtC52Mag->setText(Utils_Qt::numToQString(MicroParams->C52.Mag / 10000)); // um
    ui->edtC52Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C52.Ang)); // degrees
    ui->edtC54Mag->setText(Utils_Qt::numToQString(MicroParams->C54.Mag / 10000)); // um
    ui->edtC54Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C52.Ang)); // degrees
    ui->edtC56Mag->setText(Utils_Qt::numToQString(MicroParams->C56.Mag / 10000)); // um
    ui->edtC56Ang->setText(Utils_Qt::numToQString((180 / Constants::Pi) * MicroParams->C52.Ang)); // degrees
}

FullAberrationFrame::~FullAberrationFrame()
{
    delete ui;
}

void FullAberrationFrame::checkEditZero(QString dud)
{
    (void)dud; // make it explicit that this is not used

    auto * edt = dynamic_cast<EditUnitsBox*>(sender());

    if(edt == nullptr)
        return;

    double val = edt->text().toDouble();

    if (val <= 0)
        edt->setForegroundStyle("color: #FF8C00"); // I just chose orange, mgiht want to be a better colour
    else
        edt->setForegroundStyle("");
}

void FullAberrationFrame::dlgCancel_clicked()
{
    // don't need to do anything, just return
    auto * dlg = dynamic_cast<AberrationsDialog*>(parentWidget());
    dlg->reject();
}

void FullAberrationFrame::dlgOk_clicked()
{
    // same as clicking apply then closing the dialog
    if(dlgApply_clicked()) {
        auto * dlg = dynamic_cast<AberrationsDialog*>(parentWidget());
        dlg->accept();
    }
}

bool FullAberrationFrame::dlgApply_clicked()
{
    // this is a pain, just need to test to see if our aberrations are valid (only the 4 that need to be non-zero)
    // then just copy all the aberrations to a new class and send it off...
    // woo!

    double voltage = ui->edtVoltage->text().toDouble();
    double dfSpread = ui->edtDefocusSpread->text().toDouble() * 10;
    double converge = ui->edtConverge->text().toDouble();

    double con_ap = ui->edtConAp->text().toDouble();
    double con_ap_sig = ui->edtConApSmooth->text().toDouble();

    double obj_ap = ui->edtObjAp->text().toDouble();
    double obj_ap_sig = ui->edtObjApSmooth->text().toDouble();

    double beam_tilt = ui->edtBeamTilt->text().toDouble();
    double beam_azimuth = ui->edtBeamAzimuth->text().toDouble() * Constants::Pi / 180;

    double C10 = ui->edtC10->text().toDouble() * 10;
    double C12m = ui->edtC12Mag->text().toDouble() * 10;
    double C12a = ui->edtC12Ang->text().toDouble() * Constants::Pi / 180;

    double C21m = ui->edtC21Mag->text().toDouble() * 10;
    double C21a = ui->edtC21Ang->text().toDouble() * Constants::Pi / 180;
    double C23m = ui->edtC23Mag->text().toDouble() * 10;
    double C23a = ui->edtC23Ang->text().toDouble() * Constants::Pi / 180;

    double C30 = ui->edtC30->text().toDouble() * 10000;
    double C32m = ui->edtC32Mag->text().toDouble() * 10000;
    double C32a = ui->edtC32Ang->text().toDouble() * Constants::Pi / 180;
    double C34m = ui->edtC34Mag->text().toDouble() * 10000;
    double C34a = ui->edtC34Ang->text().toDouble() * Constants::Pi / 180;

    double C41m = ui->edtC41Mag->text().toDouble() * 10000;
    double C41a = ui->edtC41Ang->text().toDouble() * Constants::Pi / 180;
    double C43m = ui->edtC43Mag->text().toDouble() * 10000;
    double C43a = ui->edtC43Ang->text().toDouble() * Constants::Pi / 180;
    double C45m = ui->edtC45Mag->text().toDouble() * 10000;
    double C45a = ui->edtC45Ang->text().toDouble() * Constants::Pi / 180;

    double C50 = ui->edtC50->text().toDouble() * 10000;
    double C52m = ui->edtC52Mag->text().toDouble() * 10000;
    double C52a = ui->edtC52Ang->text().toDouble() * Constants::Pi / 180;
    double C54m = ui->edtC54Mag->text().toDouble() * 10000;
    double C54a = ui->edtC54Ang->text().toDouble() * Constants::Pi / 180;
    double C56m = ui->edtC56Mag->text().toDouble() * 10000;
    double C56a = ui->edtC56Ang->text().toDouble() * Constants::Pi / 180;

    // now we have all the data, assign it to our class storing everything

    auto MicroParams = Manager->microscopeParams();

    MicroParams->Voltage = voltage;
    MicroParams->CondenserAperture = con_ap;
    MicroParams->CondenserApertureSmoothing = con_ap_sig;

    MicroParams->ObjectiveAperture = obj_ap;
    MicroParams->ObjectiveApertureSmoothing = obj_ap_sig;

    MicroParams->Delta = dfSpread;
    MicroParams->Alpha = converge;

    MicroParams->BeamTilt = beam_tilt;
    MicroParams->BeamAzimuth = beam_azimuth;

    MicroParams->C10 = C10;
    MicroParams->C12 = ComplexAberration(C12m, C12a);

    MicroParams->C21 = ComplexAberration(C21m, C21a);
    MicroParams->C23 = ComplexAberration(C23m, C23a);

    MicroParams->C30 = C30;
    MicroParams->C32 = ComplexAberration(C32m, C32a);
    MicroParams->C34 = ComplexAberration(C34m, C34a);

    MicroParams->C41 = ComplexAberration(C41m, C41a);
    MicroParams->C43 = ComplexAberration(C43m, C43a);
    MicroParams->C45 = ComplexAberration(C45m, C45a);

    MicroParams->C50 = C50;
    MicroParams->C52 = ComplexAberration(C52m, C52a);
    MicroParams->C54 = ComplexAberration(C54m, C54a);
    MicroParams->C56 = ComplexAberration(C56m, C56a);

    emit dynamic_cast<AberrationsDialog*>(parentWidget())->appliedSignal();

    return true;
}

void FullAberrationFrame::setUnits() {
    ui->edtVoltage->setUnits("kV");
    ui->edtDefocusSpread->setUnits("nm");
    ui->edtConverge->setUnits("mrad");

    ui->edtBeamTilt->setUnits("mrad");
    ui->edtBeamAzimuth->setUnits("°");

    ui->edtConAp->setUnits("mrad");
    ui->edtConApSmooth->setUnits("mrad");

    ui->edtObjAp->setUnits("mrad");
    ui->edtObjApSmooth->setUnits("mrad");

    ui->edtC10->setUnits("nm");
    ui->edtC12Mag->setUnits("nm");
    ui->edtC12Ang->setUnits("°");

    ui->edtC21Mag->setUnits("nm");
    ui->edtC21Ang->setUnits("°");
    ui->edtC23Mag->setUnits("nm");
    ui->edtC23Ang->setUnits("°");

    ui->edtC30->setUnits("μm");
    ui->edtC32Mag->setUnits("μm");
    ui->edtC32Ang->setUnits("°");
    ui->edtC34Mag->setUnits("μm");
    ui->edtC34Ang->setUnits("°");

    ui->edtC41Mag->setUnits("μm");
    ui->edtC41Ang->setUnits("°");
    ui->edtC43Mag->setUnits("μm");
    ui->edtC43Ang->setUnits("°");
    ui->edtC45Mag->setUnits("μm");
    ui->edtC45Ang->setUnits("°");

    ui->edtC50->setUnits("μm");
    ui->edtC52Mag->setUnits("μm");
    ui->edtC52Ang->setUnits("°");
    ui->edtC54Mag->setUnits("μm");
    ui->edtC54Ang->setUnits("°");
    ui->edtC56Mag->setUnits("μm");
    ui->edtC56Ang->setUnits("°");

}

void FullAberrationFrame::setBackgroundStyles() {
    
    auto md = Manager->mode();
    bool do_im = Manager->ctemImageEnabled();

    QColor disabled_col = qApp->palette().color(QPalette::Disabled, QPalette::Base);
    std::string disabled_hex = disabled_col.name().toStdString();
    std::string disabled_Default = "background-color: " + disabled_hex;

    std::string ab_style = "";
    std::string ctem_image_style = "";
    std::string condens_style = "";

    if (md == SimulationMode::CTEM and !do_im)
        ab_style = disabled_Default;
    
    if (md != SimulationMode::CTEM || !do_im)
        ctem_image_style = disabled_Default;

    if (md == SimulationMode::CTEM)
        condens_style = disabled_Default;

    ui->edtConAp->setBackgroundStyle(condens_style);
    ui->edtConApSmooth->setBackgroundStyle(condens_style);

    //
    ui->edtDefocusSpread->setBackgroundStyle(ctem_image_style);
    ui->edtConverge->setBackgroundStyle(ctem_image_style);

    ui->edtObjAp->setBackgroundStyle(ctem_image_style);
    ui->edtObjApSmooth->setBackgroundStyle(ctem_image_style);

    //
    ui->edtC10->setBackgroundStyle(ab_style);
    ui->edtC12Mag->setBackgroundStyle(ab_style);
    ui->edtC12Ang->setBackgroundStyle(ab_style);

    ui->edtC21Mag->setBackgroundStyle(ab_style);
    ui->edtC21Ang->setBackgroundStyle(ab_style);
    ui->edtC23Mag->setBackgroundStyle(ab_style);
    ui->edtC23Ang->setBackgroundStyle(ab_style);

    ui->edtC30->setBackgroundStyle(ab_style);
    ui->edtC32Mag->setBackgroundStyle(ab_style);
    ui->edtC32Ang->setBackgroundStyle(ab_style);
    ui->edtC34Mag->setBackgroundStyle(ab_style);
    ui->edtC34Ang->setBackgroundStyle(ab_style);

    ui->edtC41Mag->setBackgroundStyle(ab_style);
    ui->edtC41Ang->setBackgroundStyle(ab_style);
    ui->edtC43Mag->setBackgroundStyle(ab_style);
    ui->edtC43Ang->setBackgroundStyle(ab_style);
    ui->edtC45Mag->setBackgroundStyle(ab_style);
    ui->edtC45Ang->setBackgroundStyle(ab_style);

    ui->edtC50->setBackgroundStyle(ab_style);
    ui->edtC52Mag->setBackgroundStyle(ab_style);
    ui->edtC52Ang->setBackgroundStyle(ab_style);
    ui->edtC54Mag->setBackgroundStyle(ab_style);
    ui->edtC54Ang->setBackgroundStyle(ab_style);
    ui->edtC56Mag->setBackgroundStyle(ab_style);
    ui->edtC56Ang->setBackgroundStyle(ab_style);
}













