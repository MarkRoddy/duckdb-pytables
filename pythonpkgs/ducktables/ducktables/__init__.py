
import inspect
from typing import Any, Optional, Dict, Iterable

class DuckTableSchemaWrapper:

    def __init__(self, func):
        self.func = func

    def columns(self, *args, **kwargs):
        sig = inspect.signature(self.func)
        return_type = sig.return_annotation
        if sig.return_annotation == inspect.Signature.empty:
            return None

        # First level which represents the table
        if hasattr(return_type, '__origin__') and issubclass(return_type.__origin__, Iterable):
            row_type = return_type.__args__[0]

            # Second level which represents a row
            if hasattr(row_type, '__origin__') and issubclass(row_type.__origin__, Iterable):
                col_types = row_type.__args__  # Column types
                return {f'column{i + 1}': typ for i, typ in enumerate(col_types)}
        return None

    def __call__(self, *args, **kwargs):
        return self.func(*args, **kwargs)

def ducktable(func):
    return DuckTableSchemaWrapper(func)

