

from ducktables import pytable
from unittest import TestCase

class TestDecorator(TestCase):

    def test_hardcoded_fields(self):
        @pytable(['col1', 'col2', 'col3'])
        def foo(x):
            pass
        expected = ['col1', 'col2', 'col3']
        actual = foo.fields_for()
        self.assertEqual(expected, actual)

    def test_field_factory_func(self):
        def fields_for_input(*args):
            fields = []
            for i, f in enumerate(args):
                fields.append("column" + str(i))
            return fields
        @pytable(fields_for_input)
        def repeat(num_rows, *args):
            for x in range(num_rows):
                yield [x] + list(args)

        # Check that we generate field names correctly
        expected = ['column0', 'column1', 'column2', 'column3']
        actual = repeat.fields_for(2, 'foo', 'bar', 'baz')
        self.assertEqual(expected, actual)

        # Check that we pass through return values correctly.
        expected = [
            [0, 'foo', 'bar', 'baz'],
            [1, 'foo', 'bar', 'baz'],
            ]
        actual = list(repeat(2, 'foo', 'bar', 'baz'))
        self.assertEqual(expected, actual)
            
