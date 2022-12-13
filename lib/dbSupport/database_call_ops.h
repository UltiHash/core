//
// Created by benjamin-elias on 09.05.22.
//

#ifndef INC_20_DATABASE_SUPPORT_DATABASE_CALL_OPS_H
#define INC_20_DATABASE_SUPPORT_DATABASE_CALL_OPS_H

#include "config_files.h"
#include "global_custom_functions.h"
#include "postgresSQL.h"

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>


class database_call_ops {
    bool connection_set=false;
    std::recursive_mutex database_mutex{};
    pqxx::connection *connection;
protected:
    std::string addr, p, user, pass, datab;
    //TODO: set up connection and custom database call ops
public:
    std::string schem, tab;
protected:

    virtual std::string currentSchema() {
        const std::lock_guard lock(database_mutex);
        return schem;
    }

    virtual std::string currentTableName() {
        const std::lock_guard lock(database_mutex);
        return tab;
    }

    [[nodiscard]] inline bool isOpen(const std::string &table) const {
        //TODO: gcc bug at const std::lock_guard lock(database_mutex);
        return connection_set and connection->is_open() and tab==table;
    }

    std::string currentDatabase() {
        const std::lock_guard lock(database_mutex);
        return datab;
    }

    bool checkSchemaExists(const std::string &schema) {
        const std::lock_guard lock(database_mutex);
        try{
            std::string sql = "SELECT EXISTS(SELECT schema_name FROM information_schema.schemata WHERE schema_name = \'"+schema+"\');";
            while(boost::algorithm::contains(sql, "  ")){
                boost::replace_all(sql, "  ", " ");
            }
            pqxx::nontransaction n(*connection);
            pqxx::result r(n.exec(sql));
            n.commit();
            bool exists=r[0][0].as<bool>();
            return exists;
        }
        catch (std::exception &e){
            FATAL << "checkSchemaExists on " << schema << " failed for this reason: " << e.what();
        }
        return false;
    }

    //returns if schema exists now
    bool createSchema(const std::string &schema) {
        const std::lock_guard lock(database_mutex);
        if (checkSchemaExists(schema))[[likely]]return true;
        try{
            std::string sql = "CREATE SCHEMA IF NOT EXISTS " + schema + " AUTHORIZATION " + user + " ; GRANT ALL ON SCHEMA " + schema + " TO " + datab + " ; GRANT ALL ON SCHEMA " + schema + " TO public;";
            while(boost::algorithm::contains(sql, "  ")){
                boost::replace_all(sql, "  ", " ");
            }
            pqxx::work n(*connection);
            n.exec(sql);
            n.commit();
        }
        catch (std::exception &e){
            FATAL << "createSchema on " << schema << " failed for this reason: " << e.what();
            exit(EXIT_FAILURE);
        }

        return checkSchemaExists(schema);
    }

    bool deleteSchema(const std::string &schema) {
        const std::lock_guard lock(database_mutex);
        if (!checkSchemaExists(schema))[[unlikely]]{return false;}
        try{
            std::string sql = "DROP SCHEMA IF EXISTS "+schema+" CASCADE;";
            while(boost::algorithm::contains(sql, "  ")){
                boost::replace_all(sql, "  ", " ");
            }
            pqxx::work n(*connection);
            n.exec(sql);
            n.commit();
        }
        catch (std::exception &e){
            DEBUG << "deleteSchema on " << schema << " failed for this reason: " << e.what();
        }

        return !checkSchemaExists(schema);
    }

    bool checkTableExists(const std::string &schema, const std::string &table) {
        const std::lock_guard lock(database_mutex);
        try{
            std::string sql = "SELECT EXISTS ( SELECT FROM information_schema.tables WHERE table_schema = \'"+schema+"\' AND table_name = \'"+table+"\' ) ;";
            while(boost::algorithm::contains(sql, "  ")){
                boost::replace_all(sql, "  ", " ");
            }
            pqxx::nontransaction n(*connection);
            pqxx::result r(n.exec(sql));
            n.commit();
            bool exists=r[0][0].as<bool>();
            return exists;
        }
        catch (std::exception &e){
            DEBUG << "checkTableExists on (Schema: " << schema << ", Table: " << table << ") failed for this reason: " << e.what();
        }
        return false;
    }

    std::vector<boost::tuple<std::string, std::string>>
    table_info(const std::string &schema, const std::string &table) {
        const std::lock_guard lock(database_mutex);
        std::vector<boost::tuple<std::string,std::string>> out;
        try{
            //table_name, column_name, data_type
            if(!checkTableExists(schema,table))[[unlikely]]{return out;}
            std::string sql = "SELECT column_name, data_type FROM information_schema.columns WHERE table_schema like '"+schema+"%' AND table_name like '"+table+"%' ;";
            while(boost::algorithm::contains(sql, "  ")){
                boost::replace_all(sql, "  ", " ");
            }
            pqxx::nontransaction n(*connection);
            pqxx::result r(n.exec(sql));
            n.commit();
            for (pqxx::result::const_iterator c = r.begin(); c != r.end();) {
                out.emplace_back(c[0].as<std::string>(),c[1].as<std::string>());
                if(++c == r.end())[[unlikely]]{return out;}
            }
        }
        catch (std::exception &e){
            DEBUG << "table_info on (Schema: " << schema << ", Table: " << table << ") failed for this reason: " << e.what();
            return out;
        }
        return out;
    }

    bool deleteTable(const std::string &schema, const std::string &table) {
        const std::lock_guard lock(database_mutex);
        if (!checkTableExists(schema,table))[[unlikely]]{return false;}
        try{
            std::string sql = "DROP TABLE IF EXISTS "+schema+"."+table+";";
            while(boost::algorithm::contains(sql, "  ")){
                boost::replace_all(sql, "  ", " ");
            }
            pqxx::work n(*connection);
            n.exec(sql);
            n.commit();
        }
        catch (std::exception &e){
            DEBUG << "deleteTable on (Schema: " << schema << ", Table: " << table << ") failed for this reason: " << e.what();
        }

        return !checkTableExists(schema,table);
    }
private:
    /*
     * this is a variadic function to transform requested types into string literals that tell the SQL database
     * to generate tables with certain data types
     */
    template<typename T1, typename T2, typename T3, typename... Args>
    std::string createTableRecursiveVariadicDefinition(T1 &&arg1, T2 &&arg2, T3 &&arg3, Args &&... args) {
        std::string out;
        if constexpr(String<T2>){
            if (boost::ifind_first(std::forward<T2>(arg2), std::string("character varying")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("varchar")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("character")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("char")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("text")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("bytea")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("smallint")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("integer")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("bigint")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("decimal")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("numeric")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("real")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("double precision")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("smallserial")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("serial")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("bigserial")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("money")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("timestamp")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("TIMESTAMPTZ")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("date")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("a_var")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("interval")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("boolean")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("point")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("line")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("lseg")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("box")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("path")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("polygon")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("circle")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("cidr")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("inet")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("macaddr")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("tsvector")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("tsquery")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("any")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("anyelement")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("anyarray")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("anynonarray")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("anyenum")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("anyrange")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("cstring")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("internal")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("language_handler")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("fdw_handler")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("record")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("trigger")) or
                boost::ifind_first(std::forward<T2>(arg2), std::string("void"))
                    ){
                out+=(std::string) std::forward<T1>(arg1)+" "+(std::string) std::forward<T2>(arg2)+" "+(std::string) std::forward<T3>(arg3);
            }
            else{
                if constexpr(sizeof...(args)==0){
                    out+=(std::string) std::forward<T1>(arg1)+" "+(std::string) std::forward<T2>(arg2)+" "+(std::string) std::forward<T3>(arg3);
                }
                else{
                    out+=(std::string) std::forward<T1>(arg1)+" text "+(std::string) std::forward<T3>(arg3);
                }
            }
        }
        else{
            if constexpr(std::is_same_v<typename std::decay<T2>::type,unsigned char> or
                         std::is_same_v<typename std::decay<T2>::type,unsigned short> or
                         std::is_same_v<typename std::decay<T2>::type,char> or
                         std::is_same_v<typename std::decay<T2>::type,short>
                    ){
                out+=(std::string) std::forward<T1>(arg1)+" smallint "+(std::string) std::forward<T3>(arg3);
            }
            if constexpr(std::is_same_v<typename std::decay<T2>::type,unsigned int> or
                         std::is_same_v<typename std::decay<T2>::type,int>
                    ){
                out+=(std::string) std::forward<T1>(arg1)+" integer "+(std::string) std::forward<T3>(arg3);
            }
            if constexpr(std::is_same_v<typename std::decay<T2>::type,unsigned long int> or
                         std::is_same_v<typename std::decay<T2>::type,unsigned long long int> or
                         std::is_same_v<typename std::decay<T2>::type,long int> or
                         std::is_same_v<typename std::decay<T2>::type,long long int>
                    ){
                out+=(std::string) std::forward<T1>(arg1)+" bigint "+(std::string) std::forward<T3>(arg3);
            }
            if constexpr(std::is_same_v<typename std::decay<T2>::type,float>){
                out+=(std::string) std::forward<T1>(arg1)+" real "+(std::string) std::forward<T3>(arg3);
            }
            if constexpr(std::is_same_v<typename std::decay<T2>::type,double> or
                         std::is_same_v<typename std::decay<T2>::type,long double>
                    ){
                out+=(std::string) std::forward<T1>(arg1)+" double precision "+(std::string) std::forward<T3>(arg3);
            }
            if constexpr(std::is_same_v<typename std::decay<T2>::type,bool>){
                out+=(std::string) std::forward<T1>(arg1)+" boolean "+(std::string) std::forward<T3>(arg3);
            }
        }
        if constexpr(sizeof...(args)==0){
            return out;
        }
        else{
            return out+", "+createTableRecursiveVariadicDefinition(args...);
        }
    }
public:

    bool connect(const std::string &database) {
        const std::lock_guard lock(database_mutex);
        if(isOpen(tab))[[likely]]{
            INFO << "Database " << database << " was already open.";
            return true;
        }
        INFO << "Connecting to Database " << database;
        for (char i = 0; i < 3; i++) {
            try {
                std::string out =
                        "user=" + user + " password=" + pass + " host=" + addr + " port=" + p + " dbname=postgres";// + " target_session_attrs=reader-write";
                connection = new pqxx::connection(out);
                connection_set=true;
                if (isOpen(tab))[[likely]]{
                    INFO << "Opened database successfully: " << this->datab;
                    datab = database;
                    return true;
                } else {
                    THROW(uh::util::exception, "Can't open database");
                }
            } catch (const std::exception &e) {
                ERROR << "connect on Database " << database << " failed for this reason: " << e.what();
                std::system("service postgresql restart");
                TRACE << "Restart of database done!";
            }
        }
        if(!isOpen(tab))[[likely]]{
            FATAL << "Database " << database << " could finally not be connected to!";
            exit(EXIT_FAILURE);
        }
        return false;
    }

    void disconnect() {
        const std::lock_guard lock(database_mutex);
        connection->close();
    }

    //this function creates a table and will also create the schema if it does not exist
    template<typename... Args>
    bool createTable(const std::string &schema, const std::string &table, const std::string &pre_options,
                     const std::string &options, Args &&... args) {
        const std::lock_guard lock(database_mutex);
        if(!checkSchemaExists(schema))[[unlikely]]{
            if(!createSchema(schema))[[unlikely]]{
                FATAL << "The following schema could not be created: " << schema;
                exit(EXIT_FAILURE);
            }
        }
        if(checkTableExists(schema,table))[[likely]]{return false;}
        std::string sql = "CREATE "+pre_options+" TABLE "+options+" "+schema+"."+table+" ( "+createTableRecursiveVariadicDefinition(args...)+" ) ;";
        while(boost::algorithm::contains(sql, "  ")){
            boost::replace_all(sql, "  ", " ");
        }

        try{
            pqxx::work n(*connection);
            n.exec(sql);
            n.commit();
        }
        catch (std::exception &e){
            FATAL << "createTable on (SQL command: " << sql << ") failed for this reason: " << e.what();
            exit(EXIT_FAILURE);
        }

        return checkTableExists(schema,table);
    }

    /*
     * automatically open table and create scheme or table if it does not exists
     */
    template<typename... Args>
    bool openTable(const std::string &database, const std::string &schema, const std::string &table,Args &&...args) {
        const std::lock_guard lock(database_mutex);
        if(!connect(database))[[unlikely]]{
            FATAL << "Could not connect to database and clName: " << database << " at " << schema << "." << table << " !!!";
            std::exit(EXIT_FAILURE);
        }
        schem=schema;
        tab=table;
        if (checkTableExists(schema,table))[[likely]]{
            return isOpen(table);
        }
        else{
            if (!this->createTable(schema,table,args...)){
                WARNING << "Cannot create table for open request (database: "<< database << ", schema: "
                          << schema << ", table: "<< table << ")";
                return isOpen(table);
            }
            return isOpen(table);
        }
    }

    //check if the block can be reader
    bool check_block_exists(const std::string& identifier) {
        const std::lock_guard lock(database_mutex);
        std::string sql = "SELECT exists ( SELECT identifier FROM " + schem + "." + tab + " WHERE identifier = \'"+ boost::algorithm::hex(identifier) + "\' ) ;";
        while(boost::algorithm::contains(sql, "  ")){
            boost::replace_all(sql, "  ", " ");
        }
        pqxx::nontransaction nf(*connection);
        pqxx::result rf(nf.exec(sql));
        nf.commit();
        if (rf.empty()) {
            return false;
        }
        if(rf.size()>1){
            DEBUG << "There were two blocks of " << boost::algorithm::hex(identifier)<<" found!";
        }
        return rf[0][0].as<bool>();
    }
    //get the value of the sha id, so basically retrieve/return block
    std::string read_block(const std::string& identifier) {
        const std::lock_guard lock(database_mutex);
        if(!check_block_exists(identifier))[[unlikely]]{
            ERROR << "The block "+boost::algorithm::hex(identifier)+" was not found on the database." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::string sql = "SELECT identifier,block FROM " + schem + "." + tab + " WHERE identifier = \'" + boost::algorithm::hex(identifier) + "\' ;";
        while(boost::algorithm::contains(sql, "  ")){
            boost::replace_all(sql, "  ", " ");
        }
        pqxx::nontransaction nf(*connection);
        pqxx::result rf(nf.exec(sql));
        nf.commit();
        if(rf.size()>1){
            DEBUG << "The block hash "+boost::algorithm::hex(identifier)+"was found multiple times on the database. Hash collision!" << std::endl;
        }
        std::string out=boost::algorithm::unhex(rf[0][1].as<std::string>());
        return out;
    }
    //write block by setting a string to the sha_id
    bool write_block(const std::string& identifier, const std::string& block) {
        const std::lock_guard lock(database_mutex);
        if(check_block_exists(identifier))[[unlikely]]{
            if(read_block(identifier)!=block)[[unlikely]] {
                FATAL << "Found rare Block hash overlap at identifier "+boost::algorithm::hex(identifier)+" ! Aborting!";
                return false;
            }
            else[[likely]]{
                //referencing duplicate block
                return true;
            }
        }
        std::string sql =
                "INSERT INTO " + schem + "." + tab + " (identifier,block) VALUES ( \'" + boost::algorithm::hex(identifier) +"\' , \'" + boost::algorithm::hex(block) +"\' ) ON CONFLICT (identifier) DO UPDATE SET block = excluded.block ;";
        while(boost::algorithm::contains(sql, "  ")){
            boost::replace_all(sql, "  ", " ");
        }
        pqxx::work n2(*connection);
        n2.exec(sql);
        n2.commit();

        return true;
    }
//auto create block table
    explicit database_call_ops(const std::string &db_config_file) {//: postgresSQL(db_config_file) {
        const std::lock_guard lock(database_mutex);
        try{
            std::filesystem::path config(db_config_file);
            config_files configuration = config_files(config);
            unsigned char count=0;
            for(auto &r:configuration.read()){
                switch(count){
                    case 0:addr=r;break;
                    case 1:p=r;break;
                    case 2:user=r;break;
                    case 3:pass=r;break;
                    case 4:datab=r;break;
                    case 5:schem=r;break;
                    default:tab=r;break;
                }
                count++;
            }
            INFO << "Successfully reader Database configuration file at postgreSQL \'" << config.c_str() << "\' module." ;
        }
        catch (std::exception &e){
            try{
                std::fstream file;
                file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                file.open(db_config_file, std::ios_base::binary);
                (void) file.write(std::string().c_str(), 0);
                file.close();
                WARNING<<"Could not configure Database, because there was a file error (check please) at postgreSQL \'config_database\' (address: "+addr+", port: "+p+", user: "+user+", password: "+pass+") error: " << e.what();
                INFO<<"Created a new database config file at the spot you chose. Hopefully this was not by accident so you should not forget to delete the file at the wrong spot.";
            }
            catch(std::exception& e){
                FATAL<<"Could not configure Database, because there was a file error (check please) at postgreSQL \'config_database\' (address: "+addr+", port: "+p+", user: "+user+", password: "+pass+") error: " << e.what();
                std::exit(EXIT_FAILURE);
            }
        }
        if (!this->openTable(datab, schem, tab, "", "IF NOT EXISTS",
                             "id", (unsigned long long int) 0, "GENERATED ALWAYS AS IDENTITY",
                             "identifier", "text", "UNIQUE NOT NULL",
                             "block", "text", "NOT NULL",
                             "", "", "PRIMARY KEY(id)"
        )) {
            FATAL << "Database table could not be created or opened!";
            std::exit(EXIT_FAILURE);
        }
    }
};


#endif //INTEGRATION_PROJECT_DATABASE_CALL_OPS_H
