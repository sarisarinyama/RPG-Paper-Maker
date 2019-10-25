/*
    RPG Paper Maker Copyright (C) 2017-2019 Wano

    RPG Paper Maker engine is under proprietary license.
    This source code is also copyrighted.

    Use Commercial edition for commercial use of your games.
    See RPG Paper Maker EULA here:
        http://rpg-paper-maker.com/index.php/eula.
*/

#include "dialogcommandremoveapicture.h"
#include "ui_dialogcommandremoveapicture.h"

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

DialogCommandRemoveAPicture::DialogCommandRemoveAPicture(EventCommand *command,
    SystemCommonObject *object, QStandardItemModel *parameters, QWidget *parent) :
    DialogCommand(parent),
    m_object(object),
    m_parameters(parameters),
    ui(new Ui::DialogCommandRemoveAPicture)
{
    ui->setupUi(this);

    this->initializePrimitives();

    if (command != nullptr) {
        this->initialize(command);
    }
}

DialogCommandRemoveAPicture::~DialogCommandRemoveAPicture() {
    delete ui;
}

// -------------------------------------------------------
//
//  INTERMEDIARY FUNCTIONS
//
// -------------------------------------------------------

void DialogCommandRemoveAPicture::initializePrimitives() {
    QStandardItemModel *properties;

    properties = nullptr;
    if (m_object != nullptr){
        properties = m_object->modelProperties();
    }

    ui->panelPrimitiveIndex->initializeNumber(m_parameters, properties);
}

// -------------------------------------------------------

void DialogCommandRemoveAPicture::initialize(EventCommand *command) {
    QString checked;
    int i;

    i = 0;
    ui->panelPrimitiveIndex->initializeCommand(command, i);
}

// -------------------------------------------------------

EventCommand * DialogCommandRemoveAPicture::getCommand() const{
    QVector<QString> command;

    ui->panelPrimitiveIndex->getCommand(command);

    return new EventCommand(EventCommandKind::RemoveAPicture, command);
}