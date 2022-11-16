//
// Created by benjamin-elias on 29.06.21.
//

#ifndef INC_20_DATABASE_SUPPORT_CONTAINERSQL_H
#define INC_20_DATABASE_SUPPORT_CONTAINERSQL_H

#include "postgresSQL.h"


class containerSQL:public virtual postgresSQL{
public:
    std::string schem,tab;
    bool open=false;

    [[maybe_unused]] virtual std::string currentSchema();

    [[maybe_unused]] virtual std::string currentTableName();

    template<typename... Args>
    bool openTable(const std::string &database, const std::string &schema, const std::string &table,Args &&...args) {
        const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
        if(!connect(database))[[unlikely]]{
            FATAL << "Could not connect to database and clName: " << database << " at " << schema << "." << table << " !!!";
            std::exit(EXIT_FAILURE);
        }
        schem=schema;
        tab=table;
        if (checkTableExists(schema,table))[[likely]]{
            open = true;
            return open;
        }
        else{
            open=this->createTable(schema,table,args...);
            if (!open){
                ERROR << "Cannot create tabel for open request (database: "<< database << ", schema: "
                      << schema << ", table: "<< table << ")";
                return open;
            }
            return open;
        }
    }

    virtual bool isOpen(const std::string &table);

    containerSQL():postgresSQL(){}
};

#endif //ULTIHASH_CONTAINERSQL_H

