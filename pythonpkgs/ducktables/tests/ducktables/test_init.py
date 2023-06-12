
from unittest import TestCase
from ducktables import ducktable, DuckTableSchemaWrapper

from typing import Iterator, Tuple, List, Dict

# Table function that will be the basis for all of our tests. Strategy is to
# create other functions that delegate to this function so we have consistent
# return values. The 'other functions' will each wrap themselvse in different
# ways to provide differetn testing scenarios.
def index_chars(input):
    for i, val in enumerate(input):
        yield (i, val)


class TestDuckTableSchemaWrapper(TestCase):

    def test_nothing_to_go_on(self):
        """If we don't have any info on what the func returns, we return None"""
        @ducktable
        def no_types(input):
            return index_chars(input)

        columns = no_types.columns()
        self.assertIsNone(columns)

        rows = list(no_types('foo'))
        expected_rows = [
            (0, 'f'),
            (1, 'o'),
            (2, 'o'),
            ]
        self.assertEqual(rows, expected_rows)

    def test_return_type_annotation_iterators(self):
        """If we have return type annotation return that with generated column names"""
        @ducktable
        def some_types(input) -> Iterator[Tuple[int, str]]:
            return index_chars(input)

        actual_columns = some_types.columns()
        expected_columns = {
            'column1': int,
            'column2': str,
            }
        self.assertEqual(expected_columns, actual_columns)

        rows = list(some_types('foo'))
        expected_rows = [
            (0, 'f'),
            (1, 'o'),
            (2, 'o'),
            ]
        self.assertEqual(rows, expected_rows)

    def test_return_type_annotation_lists(self):
        """If we have return type annotation return that with generated column names"""
        # Fun fact, you can't mix types in a typed list.
        @ducktable
        def some_types(input) -> List[Tuple[int, str]]:
            return index_chars(input)
        actual_columns = some_types.columns()
        expected_columns = {
            'column1': int,
            'column2': str,
            }
        self.assertEqual(expected_columns, actual_columns)

    def test_return_type_annotation_tuple(self):
        """If we have return type annotation return that with generated column names"""
        @ducktable
        def some_types(input) -> Tuple[Tuple[int, str]]:
            return index_chars(input)
        actual_columns = some_types.columns()
        expected_columns = {
            'column1': int,
            'column2': str,
            }
        self.assertEqual(expected_columns, actual_columns)

    def test_return_type_annotation_invalid(self):
        """Do we handle invalid return annotations"""
        # Outer most is not Iterator/List/Tuple
        @ducktable
        def some_types(input) -> Dict[int, str]:
            return index_chars(input)
        actual_columns = some_types.columns()
        expected_columns = None
        self.assertEqual(expected_columns, actual_columns)

        
        # Row is not Iterator/Tuple
        @ducktable
        def some_types(input) -> Iterator[int]:
            return index_chars(input)
        actual_columns = some_types.columns()
        expected_columns = None
        self.assertEqual(expected_columns, actual_columns)
