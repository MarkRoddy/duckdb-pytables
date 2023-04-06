
# Scalar Functions
def reverse(input):
    return input[::-1]

def scalar_throws_exception(input):
    raise Exception("This is an expected error")

# Table Functions
def table(input):
    for char in "a very long string":
        yield [char]

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

import unittest

class TestUdfs(unittest.TestCase):

    def test_reverse(self):
        self.assertEquals("raboof", reverse("foobar"))

    def test_table(self):
        actual = [record[0] for record in table("")]
        expected = ["a", " ", "v", "e", "r", "y", " ", "l", "o", "n", "g", " ", "s", "t", "r", "i", "n", "g"]
        self.assertEquals(actual, expected)

    def test_table2(self):
        actual = [record[0] for record in table2('foo', 'bar', 16)]
        expected = ['f', 'o', 'o', 'b', 'a', 'r', '16']
        self.assertEquals(actual, expected)

    def test_table_throws_exception(self):
        try:
            table_throws_exception("")
        except Exception as e:
            self.assertEquals("This function raises an exception", str(e))

    def test_iterator_throws_exception(self):
        iterator = iterator_throws_exception("")
        first = next(iterator)
        self.assertEquals(first, ["foo"])
        second = next(iterator)
        self.assertEquals(second, ["bar"])
        try:
            next(iterator)
        except Exception as e:
            self.assertEquals("Third record raises an exception", str(e))
                          
if __name__ == '__main__':
    unittest.main()
