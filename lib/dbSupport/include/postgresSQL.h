//
// Created by benjamin-elias on 28.06.21.
//

#ifndef INC_20_DATABASE_SUPPORT_POSTGRESQL_H
#define INC_20_DATABASE_SUPPORT_POSTGRESQL_H

//postgreSQL library
#include <pqxx/pqxx>
#include <pqxx/connection.hxx>

#include <utility>

#include "config_files.h"
#include "conceptTypes.h"

extern std::recursive_mutex postgresMutex;

/*
 * this property_tree saves the entire CPU informations available that are retrieved by CoreFreq
 */
extern std::shared_mutex infoGlobalMutex;

extern boost::property_tree::ptree *cpuInfoGlobal;//filtered CPU info all

/*
 * this property_tree saves the entire GPU informations available that are retrieved by nvidia-smi
 */
extern boost::property_tree::ptree *gpuInfoGlobal;//filtered GPU info all

class postgresSQL {
protected:
    /*
     * this module supports the concept of a static and stored connection to a database to be used over a long period of
     * a_var
     */
    std::string addr, p, user, pass, datab;
    std::recursive_mutex sqlMutex_single{};
public:
    //check the name on the database we are connected to
    [[maybe_unused]] virtual std::string currentDatabase();
    //check if the requested schema exists
    virtual bool checkSchemaExists(const std::string &schema);

    //create that schema name if it does not exist
    virtual bool createSchema(const std::string &schema);

    //delete a Schema with it s contents
    virtual bool deleteSchema(const std::string &schema);

    //check if a table within a schema exists
    virtual bool checkTableExists(const std::string &schema,const std::string &table);

    std::map<std::string, pqxx::connection> connections;
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
    //this function creates a table and will also create the schema if it does not exist
    template<typename... Args>
    bool createTable(const std::string &schema, const std::string &table, const std::string &pre_options,
                                  const std::string &options, Args &&... args) {
        const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
        const std::lock_guard<std::recursive_mutex> lock2(postgresMutex);
        if(!checkSchemaExists(schema))[[unlikely]]{
            if(!createSchema(schema))[[unlikely]]{
                FATAL << "The following schema could not be created: " << schema;
                exit(EXIT_FAILURE);
            }
        }
        if(checkTableExists(schema,table))[[likely]]{return false;}
        std::string sql = "CREATE "+pre_options+" TABLE "+options+" "+schema+"."+table+" ( "+createTableRecursiveVariadicDefinition(args...)+" ) ;";
        while(boost::algorithm::contains(sql,"  ")){
            boost::replace_all(sql,"  "," ");
        }

        try{
            pqxx::work n(connections.at(datab));
            n.exec(sql);
            n.commit();
        }
        catch (std::exception &e){
            FATAL << "createTable on (SQL command: " << sql << ") failed for this reason: " << e.what();
            exit(EXIT_FAILURE);
        }

        return checkTableExists(schema,table);
    }

    //this function reads the properties of a certain table and what are the column names
    virtual std::vector<boost::tuple<std::string,std::string>> table_info(const std::string &schema,const std::string &table);

    //this function deltes tables completely
    virtual bool deleteTable(const std::string &schema, const std::string &table);

    //run and stop the postgreSQL service
    static void run();

    static void stop();

    /*
     * fully automatic connection to a database with fallback reconnection
     */
    bool connect(const std::string &database);

    /*
     * disconnect the saved database
     */
    void disconnect();

    /*
     * at construction read configuration file or later connect to management server to connect to the right database
     */
    postgresSQL();

    ~postgresSQL();
};


#endif //INC_20_DATABASE_SUPPORT_POSTGRESQL_H
