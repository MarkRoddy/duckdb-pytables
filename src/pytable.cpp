
#include <Python.h>
#include <iostream>
#include <stdexcept>
#include <duckdb.hpp>
#include <duckdb/parser/expression/constant_expression.hpp>
#include <duckdb/parser/expression/function_expression.hpp>
#include <pytable.hpp>
#include "python_function.hpp"

using namespace duckdb;


#ifndef DEBUG
#define DEBUG false
#endif

namespace pyudf {


  void py_collect_garbage() {
    // enable garbage collection
    if (DEBUG) {
      std::cerr << "Performing Garbage Colleciton Run" << std::endl;
    PyObject *gc_module = PyImport_ImportModule("gc");
    PyObject *gc_dict = PyModule_GetDict(gc_module);
    PyObject *gc_enable = PyDict_GetItemString(gc_dict, "enable");
    PyObject *gc_result = PyObject_CallObject(gc_enable, nullptr);

    // collect garbage
    PyObject *gc_collect = PyDict_GetItemString(gc_dict, "collect");
    gc_result = PyObject_CallObject(gc_collect, nullptr);

    // disable garbage collection
    PyObject *gc_disable = PyDict_GetItemString(gc_dict, "disable");
    gc_result = PyObject_CallObject(gc_disable, nullptr);

    // release objects that are no longer needed
    Py_XDECREF(gc_result);
    Py_XDECREF(gc_collect);
    Py_XDECREF(gc_disable);
    Py_XDECREF(gc_enable);
    Py_XDECREF(gc_dict);
    Py_XDECREF(gc_module);
    std::cerr << "Garbage Colleciton Complete" << std::endl;
    }
  }
  





    
struct PyScanBindData : public TableFunctionData {
	std::vector<LogicalType> return_types;

        // Return value of the function specified
	PyObject *function_result_iterable;
};

struct PyScanLocalState : public LocalTableFunctionState {
	bool done = false;
};

struct PyScanGlobalState : public GlobalTableFunctionState {
	PyScanGlobalState() : GlobalTableFunctionState() {
	}
};

std::pair<std::string, std::string> parse_func_specifier(std::string specifier) {
	auto delim_location = specifier.find(":");
	if (delim_location == std::string::npos) {
		throw InvalidInputException("Function specifier lacks a ':' to delineate module and function");
	} else {
		auto module = specifier.substr(0, delim_location);
		auto function = specifier.substr(delim_location + 1, (specifier.length() - delim_location));
		return {module, function};
	}
}

void ConvertPyObjectsToDuckDBValues(PyObject *py_iterator, std::vector<duckdb::LogicalType> logical_types, std::vector<duckdb::Value>& result) {

	if (!PyIter_Check(py_iterator)) {
          throw InvalidInputException("First argument must be an iterator");
	}

	PyObject *py_item;
	size_t index = 0;
        std::string error_message;
	while ((py_item = PyIter_Next(py_iterator))) {
		if (index >= logical_types.size()) {
			Py_DECREF(py_item);
			error_message = "A row with " + std::to_string(index + 1) + " values was detected though " +
			                std::to_string(logical_types.size()) + " columns were expected",
                          throw InvalidInputException(error_message);
		}

		duckdb::Value value;
		PyObject *py_value;
		duckdb::LogicalType logical_type = logical_types[index];
		bool conversion_failed = false;

		switch (logical_type.id()) {
		case duckdb::LogicalTypeId::BOOLEAN:
			if (!PyBool_Check(py_item)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value(Py_True == py_item);
			}
			break;
		case duckdb::LogicalTypeId::TINYINT:
		case duckdb::LogicalTypeId::SMALLINT:
		case duckdb::LogicalTypeId::INTEGER:
			if (!PyLong_Check(py_item)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value((int32_t)PyLong_AsLong(py_item));
			}
			break;
		// case duckdb::LogicalTypeId::BIGINT:
		//   if (!PyLong_Check(py_item)) {
		//     conversion_failed = true;
		//   } else {
		//     value = duckdb::Value(PyLong_AsLongLong(py_item));
		//   }
		//   break;
		case duckdb::LogicalTypeId::FLOAT:
		case duckdb::LogicalTypeId::DOUBLE:
			if (!PyFloat_Check(py_item)) {
				conversion_failed = true;
			} else {
				value = duckdb::Value(PyFloat_AsDouble(py_item));
			}
			break;
		case duckdb::LogicalTypeId::VARCHAR:
			if (!PyUnicode_Check(py_item)) {
				conversion_failed = true;
			} else {
				py_value = PyUnicode_AsUTF8String(py_item);
				value = duckdb::Value(PyBytes_AsString(py_value));
				Py_DECREF(py_value);
			}
			break;
			// Add more cases for other LogicalTypes here
		default:
			conversion_failed = true;
		}

		if (conversion_failed) {
			// DUCKDB_API Value(std::nullptr_t val); // NOLINT: Allow implicit conversion from `nullptr_t`
			value = duckdb::Value((std::nullptr_t)NULL);
		}

		result.push_back(value);
		Py_DECREF(py_item);
		index++;
	}

	if (PyErr_Occurred()) {
		// todo: use our Python exception wrapper
		error_message = "Python runtime error occurred during iteration";
		PyErr_Clear();
                throw std::runtime_error(error_message);
	}

	if (index != logical_types.size()) {
		error_message = "A row with " + std::to_string(index) + " values was detected though " +
		                std::to_string(logical_types.size()) + " columns were expected";
                throw InvalidInputException(error_message);
	}
}

PyObject *duckdb_to_py(std::vector<Value> &values) {
	PyObject *py_tuple = PyTuple_New(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		PyObject *py_value = nullptr;

		switch (values[i].type().id()) {
		case LogicalTypeId::BOOLEAN:
			py_value = PyBool_FromLong(values[i].GetValue<bool>());
			break;
		case LogicalTypeId::TINYINT:
			py_value = PyLong_FromLong(values[i].GetValue<int8_t>());
			break;
		case LogicalTypeId::SMALLINT:
			py_value = PyLong_FromLong(values[i].GetValue<int16_t>());
			break;
		case LogicalTypeId::INTEGER:
			py_value = PyLong_FromLong(values[i].GetValue<int32_t>());
			break;
		case LogicalTypeId::BIGINT:
			py_value = PyLong_FromLongLong(values[i].GetValue<int64_t>());
			break;
		case LogicalTypeId::FLOAT:
			py_value = PyFloat_FromDouble(values[i].GetValue<float>());
			break;
		case LogicalTypeId::DOUBLE:
			py_value = PyFloat_FromDouble(values[i].GetValue<double>());
			break;
		case LogicalTypeId::VARCHAR:
			py_value = PyUnicode_FromString(values[i].GetValue<std::string>().c_str());
			break;
		default:
			Py_INCREF(Py_None);
			py_value = Py_None;
		}

		PyTuple_SetItem(py_tuple, i, py_value);
	}

	return py_tuple;
}

PyObject *pyObjectToIterable(PyObject *py_object) {
	PyObject *collections_module = PyImport_ImportModule("collections");
	if (!collections_module) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to import collections module");
		return nullptr;
	}

	PyObject *iterable_class = PyObject_GetAttrString(collections_module, "Iterable");
	if (!iterable_class) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to get Iterable class from collections module");
		Py_DECREF(collections_module);
		return nullptr;
	}

	int is_iterable = PyObject_IsInstance(py_object, iterable_class);
	Py_DECREF(iterable_class);
	Py_DECREF(collections_module);

	if (!is_iterable) {
		PyErr_SetString(PyExc_TypeError, "Input must be an iterable or an object that can be iterated upon");
		return nullptr;
	}

	PyObject *py_iter = PyObject_GetIter(py_object);
	if (!py_iter) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to get iterator from the input object");
	}

	return py_iter;
}

void PyScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
  std::cerr << "Starting Scan..." << std::endl;
	auto &bind_data = (PyScanBindData &)*data.bind_data;

	auto &local_state = (PyScanLocalState &)*data.local_state;

	if (local_state.done) {
          std::cerr << "Scan marked complete, exitting" << std::endl;
		return;
	} else {
          std::cerr << "Scan NOT marked complete" << std::endl;
        }

	PyObject *result = bind_data.function_result_iterable;
        if (nullptr == result) {
          std::cerr << "Iterable is null, throwing error" << std::endl;
          throw std::runtime_error("Where did our iterator go?");
        } else {
          std::cerr << "Iterable evaluated as non-null" << std::endl;
        }
        
	PyObject *row;
	int read_records = 0;
	while ((read_records < STANDARD_VECTOR_SIZE) && (row = PyIter_Next(result))) {
          std::cerr << "Processing next value from iterable" << std::endl;
          
		auto iter_row = pyObjectToIterable(row);
		if (!iter_row) {
			// todo: cleanup?
			throw std::runtime_error("Error: Row record not iterable as expected");
		} else {
                        std::vector<duckdb::Value> duck_row = {};
                        ConvertPyObjectsToDuckDBValues(iter_row, bind_data.return_types, duck_row);
                        for (long unsigned int i = 0; i < duck_row.size(); i++) {
                          auto v = duck_row.at(i);
                          // todo: Am I doing this correctly? I have no idea.
                          output.SetValue(i, output.size(), v);
                        }
			Py_DECREF(row);
			output.SetCardinality(output.size() + 1);
			read_records++;
		}
	}

	// PyIter_Next will return null if the iterator is exhausted or if an
	// exception has occurred during resumption of the underlying function,
	// so at this point we need to check which of these is the case.
	if (PyErr_Occurred()) {
          std::cerr << "Error occurred getting our next value" << std::endl;
		PythonException *error;
		error = new PythonException();
		std::string err = error->message;
		error->~PythonException();
		Py_DECREF(result);
                bind_data.function_result_iterable = nullptr;
                // Is this necessary? I would guess not?
                local_state.done = true;
		throw std::runtime_error(err);
	}
	if (!row) {
          std::cerr << "Row is null, iterable is exhausted" << std::endl;
		// We've exhausted our iterator
		local_state.done = true;
		Py_DECREF(result);
		// Py_DECREF(bind_data.function_result_iterable);
                bind_data.function_result_iterable = nullptr;
                std::cerr << "Decref'd iterable, and now collecting garbage" << std::endl;
                // py_collect_garbage();
                return;
	}
        std::cerr << "Row is not null. We hit max rows w/o exhausting our iterator" << std::endl;
}

unique_ptr<FunctionData> PyBind(ClientContext &context, TableFunctionBindInput &input,
                                std::vector<LogicalType> &return_types, std::vector<std::string> &names) {
	auto result = make_uniq<PyScanBindData>();
	auto names_and_types = input.named_parameters["columns"];

	auto params = input.named_parameters;
	std::string module_name;
	std::string function_name;
	std::vector<Value> arguments;
	if ((0 < params.count("module")) && (0 < params.count("func"))) {
		module_name = input.named_parameters["module"].GetValue<std::string>();
		function_name = input.named_parameters["func"].GetValue<std::string>();
		arguments = input.inputs;
	} else if ((0 == params.count("module")) && (0 == params.count("func"))) {
		// Neither module nor func specified, infer this from arg list.
		auto func_spec_value = input.inputs[0];
		if (func_spec_value.type().id() != LogicalTypeId::VARCHAR) {
			throw InvalidInputException(
			    "First argument must be string specifying 'module:func' if name parameters not supplied");
		}
		auto func_specifier = func_spec_value.GetValue<std::string>();
		std::tie(module_name, function_name) = parse_func_specifier(func_specifier);

		/* Since we had to infer our functions specifier from the argument list, we treat
		   everything after the first argument as input to the python function. */
		arguments = std::vector<Value>(input.inputs.begin() + 1, input.inputs.end());
	} else if (0 < params.count("module")) {
		throw InvalidInputException("Module specified w/o a corresponding function");
	} else if (0 < params.count("func")) {
		throw InvalidInputException("Function specified w/o a corresponding module");
	} else {
		throw InvalidInputException("I don't know how logic works");
	}

	auto &child_type = names_and_types.type();
	if (child_type.id() != LogicalTypeId::STRUCT) {
		throw InvalidInputException("columns requires a struct as input");
	}
	auto &struct_children = StructValue::GetChildren(names_and_types);
	D_ASSERT(StructType::GetChildCount(child_type) == struct_children.size());
	for (idx_t i = 0; i < struct_children.size(); i++) {
		auto &name = StructType::GetChildName(child_type, i);
		auto &val = struct_children[i];
		names.push_back(name);
		if (val.type().id() != LogicalTypeId::VARCHAR) {
			throw BinderException("we require a type specification as string");
		}
		return_types.emplace_back(TransformStringToLogicalType(StringValue::Get(val), context));
	}
	if (names.empty()) {
		throw BinderException("require at least a single column as input!");
	}

	result->return_types = std::vector<LogicalType>(return_types);

        std::cerr << "Binding Function: " + module_name + ":" + function_name << std::endl;
        PythonFunction func = PythonFunction(module_name, function_name);

        std::cerr << "Converting function arguments" << std::endl;
	auto pyarguments = duckdb_to_py(arguments);
	if (NULL == pyarguments) {
		throw IOException("Failed coerce function arguments");
	}

	// Invoke the function and grab a copy of the iterable it returns.
	PyObject *iter;
	PythonException *error;
        std::cerr << "Invoking function" << std::endl;
	std::tie(iter, error) = func.call(pyarguments);
        std::cerr << "Function invoked" << std::endl;
	if (!iter) {
		Py_XDECREF(iter);
		std::string err = error->message;
		error->~PythonException();
		throw std::runtime_error(err);
	} else if (!PyIter_Check(iter)) {
		Py_XDECREF(iter);
		throw std::runtime_error("Error: function '" + func.function_name() +
		                         "' did not return an iterator\n");
	}

        // Free each entry of the arguments tuple and the tuple itself
        std::cerr << "Freeing arguments" << std::endl;
        Py_ssize_t size = PyTuple_Size(pyarguments);
        for (Py_ssize_t i = 0; i < size; i++) {
          PyObject* arg = PyTuple_GetItem(pyarguments, i);
          // Py_XDECREF(arg);
        }
        std::cerr << "Freeing the argument tuple" << std::endl;
        // Py_XDECREF(pyarguments);

        std::cerr << "Saving result iterable" << std::endl;
	result->function_result_iterable = iter;
	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> PyInitGlobalState(ClientContext &context, TableFunctionInitInput &input) {
	auto result = make_uniq<PyScanGlobalState>();
	// result.function = get_python_function()
	return std::move(result);
}

unique_ptr<LocalTableFunctionState> PyInitLocalState(ExecutionContext &context, TableFunctionInitInput &input,
                                                     GlobalTableFunctionState *global_state) {
	// Leaving tehese commented out in case we need them in the future.
	// auto bind_data = (const PyScanBindData *)input.bind_data;
	// auto &gstate = (PyScanGlobalState &)*global_state;
	auto local_state = make_uniq<PyScanLocalState>();

	return std::move(local_state);
}

unique_ptr<CreateTableFunctionInfo> GetPythonTableFunction() {
	auto py_table_function =
	    TableFunction("python_table", {}, PyScan, (table_function_bind_t)PyBind, PyInitGlobalState, PyInitLocalState);

	py_table_function.varargs = LogicalType::ANY;
	py_table_function.named_parameters["module"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["func"] = LogicalType::VARCHAR;
	py_table_function.named_parameters["columns"] = LogicalType::ANY;
	CreateTableFunctionInfo py_table_function_info(py_table_function);
	return make_uniq<CreateTableFunctionInfo>(py_table_function_info);
}

} // namespace pyudf
