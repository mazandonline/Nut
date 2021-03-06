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

#ifndef QUERY_H
#define QUERY_H

#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QScopedPointer>
#include <QtCore/QRegularExpression>

#include "query_p.h"
#include "database.h"
#include "databasemodel.h"
#include "tablesetbase_p.h"
#include "sqlgeneratorbase_p.h"
#include "querybase_p.h"
#include "wherephrase.h"

QT_BEGIN_NAMESPACE

template<class T>
class NUT_EXPORT Query : public QueryBase
{
    QueryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Query)

public:
    Query(Database *database, TableSetBase *tableSet);
    ~Query();

    QList<T *> toList(int count = -1);
    int remove();
    T *first();

    int count();

    QVariant max(FieldPhrase &f);
    QVariant min(FieldPhrase &f);
    QVariant average(FieldPhrase &f){
        //TODO: ...
        return QVariant();
    }

    Query<T> *join(const QString &tableName);
    Query<T> *setWhere(WherePhrase where);

    Query<T> *join(Table *c){
        join(c->metaObject()->className());
        return this;
    }

//    Query<T> *setWhere(const QString &where);
    Query<T> *orderBy(QString fieldName, QString type);
    Query<T> *orderBy(WherePhrase phrase);
};

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T>::Query(Database *database, TableSetBase *tableSet) : QueryBase(database),
    d_ptr(new QueryPrivate(this))
{
    Q_D(Query);

    d->database = database;
    d->tableSet = tableSet;
    d->tableName = d->database->tableName(T::staticMetaObject.className());
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T>::~Query()
{
    qDebug() << "Query::~Query()";
    Q_D(Query);
    delete d;
}

template<class T>
Q_OUTOFLINE_TEMPLATE QList<T *> Query<T>::toList(int count)
{
    Q_D(Query);
    QList<T*> result;
    d->select = "*";

//    QSqlQuery q = d->database->exec(d->database->sqlGenertor()->selectCommand(d->wheres, d->orders, d->tableName, d->joinClassName));
    QSqlQuery q = d->database->exec(d->database->sqlGenertor()->selectCommand(
                                        SqlGeneratorBase::SelectALl,
                                        "",
                                        d->wheres,
                                        d->orderPhrases,
                                        d->tableName,
                                        d->joinClassName));

    QString pk =d->database->model().model(d->tableName)->primaryKey();
    QVariant lastPkValue = QVariant();
    int childTypeId = 0;
    T *lastRow = 0;
    TableSetBase *childTableSet;
    QStringList masterFields = d->database->model().model(d->tableName)->fieldsNames();
    QStringList childFields;
    if(!d->joinClassName.isNull())
        if(d->database->model().modelByClass(d->joinClassName)){
            childFields = d->database->model().modelByClass(d->joinClassName)->fieldsNames();
            QString joinTableName = d->database->tableName(d->joinClassName);
            childTypeId = d->database->model().model(joinTableName)->typeId();
        }

    while (q.next()) {
        if(lastPkValue != q.value(pk)){
            T *t = new T();

            foreach (QString field, masterFields)
                t->setProperty(field.toLatin1().data(), q.value(field));

            t->setTableSet(d->tableSet);
            t->setStatus(Table::FeatchedFromDB);
            t->setParent(this);

            result.append(t);
            lastRow = t;

            if(childTypeId){
                QSet<TableSetBase*> tableSets = t->tableSets;
                foreach (TableSetBase *ts, tableSets)
                    if(ts->childClassName() == d->joinClassName)
                        childTableSet = ts;
            }
        }

        if(childTypeId){
            const QMetaObject *childMetaObject = QMetaType::metaObjectForType(childTypeId);
            Table *childTable = qobject_cast<Table*>(childMetaObject->newInstance());

            foreach (QString field, childFields)
                childTable->setProperty(field.toLatin1().data(), q.value(field));

            childTable->setParent(this);
            childTable->setParentTable(lastRow);
            childTable->setStatus(Table::FeatchedFromDB);
            childTable->setTableSet(childTableSet);
            childTableSet->add(childTable);
        }
        lastPkValue = q.value(pk);

        if(!--count)
            break;
    }

    deleteLater();
    return result;
}

template<class T>
Q_OUTOFLINE_TEMPLATE T *Query<T>::first()
{
    QList<T *> list = toList(1);

    if(list.count())
        return list.first();
    else
        return 0;
}

template<class T>
Q_OUTOFLINE_TEMPLATE int Query<T>::count()
{
    Q_D(Query);

    d->select = "COUNT(*)";
    QSqlQuery q = d->database->exec(d->database->sqlGenertor()->selectCommand("COUNT(*)", d->wheres, d->orders, d->tableName, d->joinClassName));

    if(q.next())
        return q.value(0).toInt();
    return 0;
}

template<class T>
Q_OUTOFLINE_TEMPLATE QVariant Query<T>::max(FieldPhrase &f){
    Q_D(Query);

    QSqlQuery q = d->database->exec(d->database->sqlGenertor()->selectCommand("MAX(" + f.data()->text + ")", d->wheres, d->orders, d->tableName, d->joinClassName));

    if(q.next())
        return q.value(0).toInt();
    return 0;
}

template<class T>
Q_OUTOFLINE_TEMPLATE QVariant Query<T>::min(FieldPhrase &f){
    Q_D(Query);

    QSqlQuery q = d->database->exec(d->database->sqlGenertor()->selectCommand("MIN(" + f.data()->text + ")", d->wheres, d->orders, d->tableName, d->joinClassName));

    if(q.next())
        return q.value(0).toInt();
    return 0;
}

template<class T>
Q_OUTOFLINE_TEMPLATE int Query<T>::remove()
{
    Q_D(Query);

    QString sql = d->database->sqlGenertor()->deleteCommand(d->wheres, d->tableName);
//            d->_database->sqlGenertor()->deleteRecords(_tableName, queryText());
//    sql = compileCommand(sql);
    QSqlQuery q = d->database->exec(sql);
    return q.numRowsAffected();
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::join(const QString &tableName)
{
    Q_D(Query);
    d->joinClassName = tableName;
    return this;
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::setWhere(WherePhrase where)
{
    Q_D(Query);
    d->wheres.append(where);
    return this;
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::orderBy(QString fieldName, QString type)
{
    Q_D(Query);
    d->orders.insert(fieldName, type);
    return this;
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::orderBy(WherePhrase phrase)
{
    Q_D(Query);
    d->orderPhrases.append(phrase);
    return this;
}

QT_END_NAMESPACE

#endif // QUERY_H
