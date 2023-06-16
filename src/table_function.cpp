
#include <python_function.hpp>
#include <table_function.hpp>
#include <config.h>
#include <pyconvert.hpp>
#include <string>
#include <log.hpp>

namespace pyudf {

TableFunction::TableFunction(const std::string &function_specifier) : PythonFunction(function_specifier) {
	auto wrapped = wrap_function(function);
	if (wrapped) {
		debug("Successfully wrapped the function");
		function = wrapped;
	} else {
		debug("Failed to find function wrapper, this may be ok?");
	}
}

TableFunction::TableFunction(const std::string &module_name, const std::string &function_name)
    : PythonFunction(module_name, function_name) {
	auto wrapped = wrap_function(function);
	if (wrapped) {
		debug("Successfully wrapped the function");
		function = wrapped;
	} else {
		debug("Failed to find function wrapper, this may be ok?");
	}
}

PyObject *TableFunction::wrap_function(PyObject *function) {
	debug("About to import the decorator");
	PyObject *decorator = import_decorator();
	if (!decorator) {
		// todo: raise error?
		debug("Not able to find importable verison either, exitting");
		return nullptr;
	}
	if (!decorator) {
		debug("Our decorator is null?!?!");
		return nullptr;
	}
	if (!PyCallable_Check(decorator)) {
		debug("Our decorator isn't callable?!?!");
		return nullptr;
	}

	// TODO TODO TODO
	// check if the function is already wrapped using isinstance(), return it if so
	// TODO TODO TODO

	// Otherwise, go ahead and wrappe it
	debug("Creating a tuple for arguments");
	PyObject *args = PyTuple_New(1);
	debug("Setting our function as hte only argument");
	PyTuple_SetItem(args, 0, function);

	if (!function) {
		debug("function is nullptr");
		return nullptr;
	} else if (function == Py_None) {
		debug("function is None");
		return nullptr;
	} else if (!PyCallable_Check(function)) {
		debug("function is not callable");
		return nullptr;
	}

	if (!args) {
		debug("args is nullptr\n");
		return nullptr;
	} else if (args == Py_None) {
		debug("args is None\n");
		return nullptr;
	} else if (!PyTuple_Check(args)) {
		debug("args is not a tuple\n");
		return nullptr;
	}

	if (PyErr_Occurred()) {
		PyErr_Print();
		return nullptr;
	}

	debug("Calling the function to wrap our function");
	PyObject *wrapped_function = PyObject_CallObject(decorator, args);
	debug("Completed calling the function to wrapp");
	if (!wrapped_function) {
		debug("Error wrapping the function");
		PyErr_Print();
		return nullptr;
	}
	debug("Succeeded in wrapping the function?");
	// todo: handle exception from wrapping?
	return wrapped_function;
}

PyObject *TableFunction::import_decorator() {
	debug("Calling import module");
	PyObject *module_obj = PyImport_ImportModule("ducktables");
	debug("Completed calling import module");
	if (!module_obj) {
		debug("Module not found returning null");
		// todo: Maybe we want to report this error if it isn't just import error?
		PyErr_Print();
		return nullptr;
	}
	debug("Getting the attribute...");
	PyObject *function_obj = PyObject_GetAttrString(module_obj, "ducktable");
	if (!function_obj) {
		// todo: Maybe we want to report this error if it isn't just import error?
		debug("Attribute not found returning null");
		PyErr_Print();
		return nullptr;
	}
	debug("Got the function, now returning");
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
			debug("Number of column names returned by Python:  " + std::to_string(listSize));
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
	debug("Number of column names in our C++ vector:  " + std::to_string(columnNames.size()));
	return columnNames;
}

} // namespace pyudf
