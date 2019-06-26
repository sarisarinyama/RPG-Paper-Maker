/*
    RPG Paper Maker Copyright (C) 2017-2019 Marie Laporte

    RPG Paper Maker engine is under LGPL-3 license.

    Commercial license for commercial use of your games:
        https://creativecommons.org/licenses/by-nc/4.0/.

    See more information here: http://rpg-paper-maker.com/index.php/downloads.
*/

#include "dialogsystemcolor.h"
#include "systemcolor.h"

const QString SystemColor::JSON_R = "r";
const QString SystemColor::JSON_G = "g";
const QString SystemColor::JSON_B = "b";
const QString SystemColor::JSON_A = "a";

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

SystemColor::SystemColor() : SuperListItem() {

}

SystemColor::SystemColor(int i, QString n, int r, int g, int b, int a) :
    SuperListItem (i, n),
    m_color(r, g, b, a)
{

}

SystemColor::~SystemColor() {

}

QColor SystemColor::color() const {
    return m_color;
}

void SystemColor::setColor(QColor& color) {
    m_color = color;
}


// -------------------------------------------------------
//
//  VIRTUAL METHODS
//
// -------------------------------------------------------

bool SystemColor::openDialog() {
    SystemColor color;
    color.setCopy(*this);
    DialogSystemColor dialog(color);
    if (dialog.exec() == QDialog::Accepted){
        setCopy(color);
        return true;
    }
    return false;
}

// -------------------------------------------------------

SuperListItem * SystemColor::createCopy() const {
    SystemColor *super = new SystemColor;
    super->setCopy(*this);
    return super;
}

// -------------------------------------------------------

void SystemColor::setCopy(const SystemColor &color) {
    SuperListItem::setCopy(color);
    m_color = color.m_color;
}

// -------------------------------------------------------

void SystemColor::read(const QJsonObject &json) {
    SuperListItem::read(json);

    m_color = QColor(json[JSON_R].toInt(), json[JSON_G].toInt(), json[JSON_B]
        .toInt(), json[JSON_A].toInt());
}

// -------------------------------------------------------

void SystemColor::write(QJsonObject &json) const{
    SuperListItem::write(json);

    json[JSON_R] = m_color.red();
    json[JSON_G] = m_color.green();
    json[JSON_B] = m_color.blue();
    json[JSON_A] = m_color.alpha();
}