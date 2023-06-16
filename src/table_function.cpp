
#include <python_function.hpp>
#include <table_function.hpp>
#include <config.h>
#include <pyconvert.hpp>

namespace pyudf {

// class A
// {
// public:
//   A(bool construct = true) {
//     if (!construct) return;
//     bar();
//   }
//   virtual void bar() { cout << "A::bar" << endl; }
// };

// class B : public A
// {
// public:
//   B() : A(false) { bar(); }
//   void bar() override { cout << "B::bar" << endl; }
// };
TableFunction::TableFunction(const std::string &function_specifier) : PythonFunction(function_specifier) {
	// std::string module_name;
	// std::string function_name;
	// std::tie(module_name, function_name) = parse_func_specifier(function_specifier);
	// init(module_name, function_name);
	// std::cerr << "Got the function, now going to wrap it..." << std::endl;

	auto wrapped = wrap_function(function);
	if (wrapped) {
		std::cerr << "Successfully wrapped the function" << std::endl;
		function = wrapped;
	} else {
		std::cerr << "Failed to find function wrapper, this may be ok?" << std::endl;
	}
}

TableFunction::TableFunction(const std::string &module_name, const std::string &function_name)
    : PythonFunction(module_name, function_name) {
	auto wrapped = wrap_function(function);
	if (wrapped) {
		std::cerr << "Successfully wrapped the function" << std::endl;
		function = wrapped;
	} else {
		std::cerr << "Failed to find function wrapper, this may be ok?" << std::endl;
	}
}

PyObject *TableFunction::wrap_function(PyObject *function) {
	std::cerr << "About to import the decorator" << std::endl;
	PyObject *decorator = import_decorator();
	if (!decorator) {
		// todo: raise error?
		std::cerr << "Not able to find importable verison either, exitting" << std::endl;
		return nullptr;
	}
	if (!decorator) {
		std::cerr << "Our decorator is null?!?!" << std::endl;
		return nullptr;
	}
	if (!PyCallable_Check(decorator)) {
		std::cerr << "Our decorator isn't callable?!?!" << std::endl;
		return nullptr;
	}

	// TODO TODO TODO
	// check if the function is already wrapped using isinstance(), return it if so
	// TODO TODO TODO

	// Otherwise, go ahead and wrappe it
	std::cerr << "Creating a tuple for arguments" << std::endl;
	PyObject *args = PyTuple_New(1);
	std::cerr << "Setting our function as hte only argument" << std::endl;
	PyTuple_SetItem(args, 0, function);

	if (!function) {
		printf("function is nullptr\n");
	} else if (function == Py_None) {
		printf("function is None\n");
	} else if (!PyCallable_Check(function)) {
		printf("function is not callable\n");
	}

	if (!args) {
		printf("args is nullptr\n");
	} else if (args == Py_None) {
		printf("args is None\n");
	} else if (!PyTuple_Check(args)) {
		printf("args is not a tuple\n");
	}

	if (PyErr_Occurred()) {
		PyErr_Print();
	}

	std::cerr << "Calling the function to wrap our function" << std::endl;
	PyObject *wrapped_function = PyObject_CallObject(decorator, args);
	std::cerr << "Completed calling the function to wrapp" << std::endl;
	if (!wrapped_function) {
		std::cerr << "Error wrapping the function" << std::endl;
		PyErr_Print();
		return nullptr;
	}
	std::cerr << "Succeeded in wrapping the function?" << std::endl;
	// todo: handle exception from wrapping
	return wrapped_function;
}

PyObject *TableFunction::import_decorator() {
	std::cerr << "Calling import module" << std::endl;
	std::cerr << "Calling import module2" << std::endl;
	PyObject *module_obj = PyImport_ImportModule("ducktables");
	std::cerr << "Completed calling import module" << std::endl;
	if (!module_obj) {
		std::cerr << "Module not found returning null" << std::endl;
		// todo: Maybe we want to report this error if it isn't just import error?
		PyErr_Print();
		return nullptr;
	}
	std::cerr << "Getting the attribute..." << std::endl;
	PyObject *function_obj = PyObject_GetAttrString(module_obj, "ducktable");
	if (!function_obj) {
		// todo: Maybe we want to report this error if it isn't just import error?
		std::cerr << "Attribute not found returning null" << std::endl;
		PyErr_Print();
		return nullptr;
	}
	std::cerr << "Got the function, now returning" << std::endl;
	return function_obj;
}

std::vector<PyObject *> TableFunction::pycolumn_types(PyObject *args, PyObject *kwargs) {
	std::vector<PyObject *> columnTypes;

	// Get the 'column_types' method
	PyObject *columnTypesMethod = PyObject_GetAttrString(function, "column_types");

	// Check if the 'column_types' method exists
	if (columnTypesMethod && PyCallable_Check(columnTypesMethod)) {
		// Invoke the 'column_types' method with args and kwargs
		PyObject *result = PyObject_Call(columnTypesMethod, args, kwargs);

		// Check if the return value is a list
		if (result && PyList_Check(result)) {
			Py_ssize_t listSize = PyList_Size(result);

			// Iterate over the list elements
			for (Py_ssize_t i = 0; i < listSize; ++i) {
				PyObject *listItem = PyList_GetItem(result, i);

				// Add the list item to the columnTypes vector
				Py_INCREF(listItem);
				columnTypes.push_back(listItem);
			}
		}

		// Release the reference to the result
		Py_XDECREF(result);
	}

	// Release the reference to the 'column_types' method
	Py_XDECREF(columnTypesMethod);

	return columnTypes;
}

std::vector<duckdb::LogicalType> TableFunction::column_types(PyObject *args, PyObject *kwargs) {
	auto python_types = pycolumn_types(args, kwargs);
	// todo: check for a Python error?
	auto ddb_types = PyTypesToLogicalTypes(python_types);
	return ddb_types;
}

std::vector<std::string> TableFunction::column_names(PyObject *args, PyObject *kwargs) {
	std::vector<std::string> columnNames;

	// Get the 'column_names' method
	PyObject *columnNamesMethod = PyObject_GetAttrString(function, "column_names");

	// Check if the 'column_names' method exists
	if (columnNamesMethod && PyCallable_Check(columnNamesMethod)) {
		// Invoke the 'column_names' method with args and kwargs
		PyObject *result = PyObject_Call(columnNamesMethod, args, kwargs);

		// Check if the return value is a list
		if (result && PyList_Check(result)) {
			Py_ssize_t listSize = PyList_Size(result);
			std::cerr << "Number of column names returned by Python:  " << listSize << std::endl;
			// Iterate over the list elements
			for (Py_ssize_t i = 0; i < listSize; ++i) {
				PyObject *listItem = PyList_GetItem(result, i);

				// Convert the list item to a C++ string
				if (PyUnicode_Check(listItem)) {
					const char *columnName = Unicode_AsUTF8(listItem);
					columnNames.emplace_back(columnName);
					// todo: free columnName?????
				} else {
					// todo: this will break something by leaving out a column name
				}
			}
		}

		// Release the reference to the result
		Py_XDECREF(result);
	}

	// Release the reference to the 'column_names' method
	Py_XDECREF(columnNamesMethod);
	std::cerr << "Number of column names in our C++ vector:  " << columnNames.size() << std::endl;
	return columnNames;
}

} // namespace pyudf
