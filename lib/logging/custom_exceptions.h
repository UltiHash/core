//
// Created by benjamin-elias on 09.05.22.
//


#ifndef INC_18_LOGGING_EXCEPTIONS_H
#define INC_18_LOGGING_EXCEPTIONS_H

#include <exception>

#define CUSTOM_THROW(ec, MESSAGE) throw std::runtime_error("\nThis error occurred in file \"" + std::string(__FILE__) + "\" with code "+std::to_string(ec)+"."+"\nIn function \"" + std::string(__FUNCTION__ ) + "\"."+"\nIn line \"" + std::to_string(__LINE__) + "\". Error Message: "+std::string(MESSAGE));

/*
    try{
        CUSTOM_THROW(EXIT_SUCCESS,"If that happens you must fail.");
    }
    catch(std::exception &e){
        FATAL << "This error happend, because of this reason" << e.what();
    }*/

#endif //INC_18_LOGGING_EXCEPTIONS_H
