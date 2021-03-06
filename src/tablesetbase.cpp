/**************************************************************************
**
** This file is part of Nut project.
** https://github.com/HamedMasafi/Nut
**
** Nut is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Nut is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with Nut.  If not, see <http://www.gnu.org/licenses/>.
**
**************************************************************************/

#include "table.h"
#include "database.h"
#include "tablesetbase_p.h"
#include "databasemodel.h"

TableSetBase::TableSetBase(Database *parent) : QObject(parent), _database(parent), _table(0)
{
    parent->add(this);
}

TableSetBase::TableSetBase(Table *parent) : QObject(parent), _database(0), _table(parent)
{
    parent->add(this);
}

void TableSetBase::save(Database *db)
{
    foreach (Table *t, _tablesList) {
        if(_table)
            t->setParentTable(_table);

        if(t->status() == Table::Added
                || t->status() == Table::Modified
                || t->status() == Table::Deleted){
            t->save(db);
        }
    }
}

void TableSetBase::clearChilds()
{
    foreach (Table *t, _tablesList)
        delete t;
}

void TableSetBase::add(Table *t)
{
    if(!_tables.contains(t)){
        _tables.insert(t);
        _tablesList.append(t);
    }
}

QString TableSetBase::childClassName() const
{
    return _childClassName;
}

Database *TableSetBase::database() const
{
    return _database;
}

void TableSetBase::setDatabase(Database *database)
{
    _database = database;
}
