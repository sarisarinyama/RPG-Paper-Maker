/*
    RPG Paper Maker Copyright (C) 2017-2019 Marie Laporte

    This file is part of RPG Paper Maker.

    RPG Paper Maker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPG Paper Maker is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "systemcurrency.h"
#include "dialogsystemcurrency.h"

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

SystemCurrency::SystemCurrency() : SystemCurrency(1, new LangsTranslation, -1)
{

}

SystemCurrency::SystemCurrency(int i, LangsTranslation* names, int pictureID) :
    SystemIcon(i, names, pictureID)
{

}

// -------------------------------------------------------
//
//  VIRTUAL FUNCTIONS
//
// -------------------------------------------------------

bool SystemCurrency::openDialog() {
    SystemCurrency currency;
    currency.setCopy(*this);
    DialogSystemCurrency dialog(currency);

    if (dialog.exec() == QDialog::Accepted) {
        setCopy(currency);
        return true;
    }

    return false;
}

// -------------------------------------------------------

SuperListItem* SystemCurrency::createCopy() const{
    SystemCurrency* super = new SystemCurrency;
    super->setCopy(*this);
    return super;
}

// -------------------------------------------------------

void SystemCurrency::setCopy(const SystemCurrency& super){
    SystemIcon::setCopy(super);
}

// -------------------------------------------------------

void SystemCurrency::read(const QJsonObject &json){
    SystemIcon::read(json);
}

// -------------------------------------------------------

void SystemCurrency::write(QJsonObject &json) const{
    SystemIcon::write(json);
}
