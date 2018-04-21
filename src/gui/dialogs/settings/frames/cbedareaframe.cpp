#include <QtGui/QRegExpValidator>
#include <utils/stringutils.h>
#include "cbedareaframe.h"
#include "ui_cbedareaframe.h"

CbedAreaFrame::CbedAreaFrame(QWidget *parent, CbedPosition pos) :
    QWidget(parent), Position(pos),
    ui(new Ui::CbedAreaFrame)
{
    ui->setupUi(this);

    QRegExpValidator* pmValidator = new QRegExpValidator(QRegExp("[+-]?(\\d*(?:\\.\\d*)?(?:[eE]([+\\-]?\\d+)?)>)*"));
    QRegExpValidator* pValidator = new QRegExpValidator(QRegExp("[+]?(\\d*(?:\\.\\d*)?(?:[eE]([+\\-]?\\d+)?)>)*"));

    ui->edtPosX->setValidator(pmValidator);
    ui->edtPosY->setValidator(pmValidator);

    ui->edtPadding->setValidator(pValidator);

    connect(ui->edtPadding, SIGNAL(textChanged(QString)), this, SLOT(valuesChanged(QString)));

    connect(ui->edtPosX, SIGNAL(editingFinished()), this, SLOT(editing_finished()));
    connect(ui->edtPosY, SIGNAL(editingFinished()), this, SLOT(editing_finished()));
    connect(ui->edtPadding, SIGNAL(editingFinished()), this, SLOT(editing_finished()));

    // this just resets the values to the currently stored ones
    on_btnReset_clicked();
}

CbedAreaFrame::~CbedAreaFrame()
{
    delete ui;
}

void CbedAreaFrame::valuesChanged(QString dud) {
    emit areaChanged();
}

CbedPosition CbedAreaFrame::getCbedPos() {
    float xp = ui->edtPosX->text().toFloat();
    float yp = ui->edtPosY->text().toFloat();
    float padding = ui->edtPadding->text().toFloat();
    return CbedPosition(xp, yp, padding);
}

void CbedAreaFrame::on_btnReset_clicked()
{
    ui->edtPosX->setText(Utils_Qt::numToQString( Position.getXPos()));
    ui->edtPosY->setText(Utils_Qt::numToQString( Position.getYPos()));
    ui->edtPadding->setText(Utils_Qt::numToQString( Position.getPadding()));
    emit areaChanged();
}

void CbedAreaFrame::editing_finished() {
    QLineEdit* sndr = (QLineEdit*) sender();

    auto val = sndr->text().toFloat();
    sndr->setText(Utils_Qt::numToQString( val ));
}
