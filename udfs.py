
# Scalar Functions
def reverse(input):
    return input[::-1]

def scalar_throws_exception(input):
    raise Exception("This is an expected error")

# Table Functions
def table(input):
    for char in "a very long string":
        yield [char]

def index_chars(input):
    for i, val in enumerate(input):
        yield [i, val]


def table2(one_str_input, two_str_input, three_int_input):
    for c in one_str_input:
        yield [c]
    for c in two_str_input:
        yield [c]
    yield [str(three_int_input)]


def table_throws_exception(input):
    raise Exception("This function raises an exception")


def iterator_throws_exception(input):
    yield ["foo"]
    yield ["bar"]
    raise Exception("Third record raises an exception")


def num_columns(val, num_rows, num_cols):
    """Genreates a table of rows x cols where each entry is 'val'"""
    # Explicitly coercing rows/columns arguments to ints because
    # an early version of the extension forced all arugments to
    # the same type as the first argument in the arg list. So
    # call like this:
    # python_table('foo', 'bar', {schema}, ['x', 1, 2])
    # would result in an arg list of ['x', '1', '2']
    for _ in range(int(num_rows)):
        row = []
        for r in range(int(num_cols)):
            row.append(val)
        yield row

import unittest

class TestUdfs(unittest.TestCase):

    def test_reverse(self):
        self.assertEqual("raboof", reverse("foobar"))

    def test_table(self):
        actual = [record[0] for record in table("")]
        expected = ["a", " ", "v", "e", "r", "y", " ", "l", "o", "n", "g", " ", "s", "t", "r", "i", "n", "g"]
        self.assertEqual(actual, expected)

    def test_table2(self):
        actual = [record[0] for record in table2('foo', 'bar', 16)]
        expected = ['f', 'o', 'o', 'b', 'a', 'r', '16']
        self.assertEqual(actual, expected)

    def test_table_throws_exception(self):
        try:
            table_throws_exception("")
        except Exception as e:
            self.assertEqual("This function raises an exception", str(e))

    def test_iterator_throws_exception(self):
        iterator = iterator_throws_exception("")
        first = next(iterator)
        self.assertEqual(first, ["foo"])
        second = next(iterator)
        self.assertEqual(second, ["bar"])
        try:
            next(iterator)
        except Exception as e:
            self.assertEqual("Third record raises an exception", str(e))

    def test_index_chars(self):
        actual = list(index_chars("foo"))
        expected = [[0, 'f'], [1, 'o'], [2, 'o']]
        self.assertEqual(actual, expected)

    def test_num_columns(self):
        actual = list(num_columns('x', 2, 3))
        expected = [
            ['x', 'x', 'x'],
            ['x', 'x', 'x'],
            ]
        self.assertEqual(actual, expected)

if __name__ == '__main__':
    unittest.main()
