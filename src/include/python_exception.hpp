#ifndef PYTHONEXCEPTION_H
#define PYTHONEXCEPTION_H

#include <string>
#include <iostream>


class PythonException {
public:
	std::string message;
	std::string traceback;
        void print_error();
        PythonException(std::string message, std::string traceback);
        PythonException();
};

#endif // PYTHONEXCEPTION
