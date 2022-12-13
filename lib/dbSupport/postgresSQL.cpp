//
// Created by benjamin-elias on 28.06.21.
//

#include "postgresSQL.h"

std::recursive_mutex postgresMutex{};

std::shared_mutex infoGlobalMutex{};
boost::property_tree::ptree *cpuInfoGlobal= nullptr;//filtered CPU info all
boost::property_tree::ptree *gpuInfoGlobal= nullptr;//filtered CPU info all

std::string postgresSQL::currentDatabase() {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    return datab;
}

bool postgresSQL::checkSchemaExists(const std::string &schema) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    try{
        std::string sql = "SELECT EXISTS(SELECT schema_name FROM information_schema.schemata WHERE schema_name = '"+schema+"');";
        boost::replace_all(sql,"  "," ");
        pqxx::nontransaction n(connections.at(datab));
        pqxx::result r(n.exec(sql));
        n.commit();
        for (pqxx::result::const_iterator c = r.begin(); c != r.end();) {
            auto it = c;
            if(++c == r.end())[[likely]]{ return it[0].as<bool>();}
            else break;
        }
    }
    catch (std::exception &e){
        ERROR << "checkSchemaExists on " << schema << " failed for this reason: " << e.what();
    }
    return false;
}

//returns if schema exists now
bool postgresSQL::createSchema(const std::string &schema) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    if (checkSchemaExists(schema))[[likely]]return true;
    try{
        std::string sql = "CREATE SCHEMA IF NOT EXISTS " + schema + " AUTHORIZATION " + user + " ; GRANT ALL ON SCHEMA " + schema + " TO " + datab + " ; GRANT ALL ON SCHEMA " + schema + " TO public;";
        boost::replace_all(sql,"  "," ");
        pqxx::work n(connections.at(datab));
        n.exec(sql);
        n.commit();
    }
    catch (std::exception &e){
        FATAL << "createSchema on " << schema << " failed for this reason: " << e.what();
        exit(EXIT_FAILURE);
    }

    return checkSchemaExists(schema);
}

bool postgresSQL::deleteSchema(const std::string &schema) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    if (!checkSchemaExists(schema))[[unlikely]]{return false;}
    try{
        std::string sql = "DROP SCHEMA IF EXISTS "+schema+" CASCADE;";
        boost::replace_all(sql,"  "," ");
        pqxx::work n(connections.at(datab));
        n.exec(sql);
        n.commit();
    }
    catch (std::exception &e){
        ERROR << "deleteSchema on " << schema << " failed for this reason: " << e.what();
    }

    return !checkSchemaExists(schema);
}

bool postgresSQL::checkTableExists(const std::string &schema, const std::string &table) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    if (!checkSchemaExists(schema))[[unlikely]]{return false;}
    try{
        std::string sql = "SELECT EXISTS (SELECT FROM pg_catalog.pg_class c JOIN pg_catalog.pg_namespace n"
                          " ON n.oid = c.relnamespace WHERE n.nspname = '"+schema+"' AND c.relname = '"+table+
                          "' AND c.relkind = 'r' );";
        boost::replace_all(sql,"  "," ");
        pqxx::nontransaction n(connections.at(datab));
        pqxx::result r(n.exec(sql));
        n.commit();
        for (pqxx::result::const_iterator c = r.begin(); c != r.end();) {
            auto it = c;
            if(++c == r.end())[[likely]]{return it[0].as<bool>();}
            else break;
        }
    }
    catch (std::exception &e){
        ERROR << "checkTableExists on (Schema: " << schema << ", Table: " << table << ") failed for this reason: " << e.what();
    }
    return false;
}

std::vector<boost::tuple<std::string, std::string>>
postgresSQL::table_info(const std::string &schema, const std::string &table) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    std::vector<boost::tuple<std::string,std::string>> out;
    try{
        //table_name, column_name, data_type
        if(!checkTableExists(schema,table))[[unlikely]]{return out;}
        std::string sql = "SELECT column_name, data_type FROM information_schema.columns WHERE table_schema like '"+schema+"%' AND table_name like '"+table+"%' ;";
        boost::replace_all(sql,"  "," ");
        pqxx::nontransaction n(connections.at(datab));
        pqxx::result r(n.exec(sql));
        n.commit();
        for (pqxx::result::const_iterator c = r.begin(); c != r.end();) {
            out.emplace_back(c[0].as<std::string>(),c[1].as<std::string>());
            if(++c == r.end())[[unlikely]]{return out;}
        }
    }
    catch (std::exception &e){
        ERROR << "table_info on (Schema: " << schema << ", Table: " << table << ") failed for this reason: " << e.what();
        return out;
    }
    return out;
}

bool postgresSQL::deleteTable(const std::string &schema, const std::string &table) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    if (!checkTableExists(schema,table))[[unlikely]]{return false;}
    try{
        std::string sql = "DROP TABLE IF EXISTS "+schema+"."+table+";";
        boost::replace_all(sql,"  "," ");
        pqxx::work n(connections.at(datab));
        n.exec(sql);
        n.commit();
    }
    catch (std::exception &e){
        ERROR << "deleteTable on (Schema: " << schema << ", Table: " << table << ") failed for this reason: " << e.what();
    }

    return !checkTableExists(schema,table);
}

void postgresSQL::run() {
    std::system("service postgresql start");
}

void postgresSQL::stop() {
    std::system("service postgresql stop");
}

bool postgresSQL::connect(const std::string &database) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    if(connections.find(database) != connections.end() and connections.find(database)->second.is_open())[[likely]]{
        INFO << "Database " << database << " was already open.";
        return true;
    }
    INFO << "Connecting on Database " << database;
    for (char i = 0; i < 3; i++) {
        try {
            std::string out =
                    "dbname = " + database + " user = " + user + " password = " + pass + " hostaddr = " + addr +
                    " port = " + p;
            if (connections.find(database) == connections.end())[[likely]]{
                connections.insert({database, pqxx::connection(out)});
            }
            if (connections.find(database)->second.is_open())[[likely]]{
                INFO << "Opened database successfully: " << connections.find(database)->second.dbname();
                datab = database;
                return true;
            } else {
                THROW(uh::util::exception, "Can't open database");
            }
        } catch (const std::exception &e) {
            ERROR << "connect on Database " << database << " failed for this reason: " << e.what();
            std::system("service postgresql restart");
            INFO << "Restart of database done!";
        }
    }
    if(!connections.find(database)->second.is_open())[[likely]]{
        FATAL << "Database " << database << " could finally not be connected to!";
        exit(EXIT_FAILURE);
    }
    return false;
}

void postgresSQL::disconnect() {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
    for (auto &i:connections) {
        i.second.close();
    }
}

postgresSQL::postgresSQL() {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    try{
        std::filesystem::path config;
        //TODO:this did not compile -> where does root_work_path come from?
        // config.append(root_work_path->c_str());
        config.append("config_database");
        config_files configuration = config_files(config);
        auto reader=configuration.read();
        unsigned char count=0;
        for(auto &r:reader){
            switch(count){
                case 0:addr=r;break;
                case 1:p=r;break;
                case 2:user=r;break;
                default:pass=r;break;
            }
            count++;
        }
        INFO << "Successfully read Database configuration file at postgreSQL \'" << config.c_str() << "\' module." ;
    }
    catch (std::exception &e){
        FATAL<<"Could not configure Database, because there was a file error (check please) at postgreSQL \'config_database\' (address: "+addr+", port: "+p+", user: "+user+", password: "+pass+") error: " << e.what();
        exit(EXIT_FAILURE);
    }
}

postgresSQL::~postgresSQL() {
    disconnect();
    stop();
}
