

class PyTable:

    _fields = None
    _fieldfactory = None
    _types = None
    _typeFactory = None

    def __init__(self, func, fields, types):
        
        self._func = func

        if isinstance(fields, (list, tuple)):
            self._fields = fields
        elif callable(fields):
            self._fieldfactory = fields
        else:
            msg = "Argument must be either list of fields, or callable to produce them from arguments"
            raise Exception(msg)

    def __call__(self, *args, **kwargs):
        return self._func(*args, **kwargs)

    def fields_for(self, *args, **kwargs):
        if self._fieldfactory:
            return self._fieldfactory(*args, **kwargs)
        else:
            return self._fields

    def typtes_for(self, *args, **kwargs):
        

def pytable(fields = None, types = None):
    def decorator(func):
        pt = PyTable(func, fields)
        return pt
    return decorator

